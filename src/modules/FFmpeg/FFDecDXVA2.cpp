/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <FFDecDXVA2.hpp>

#include <HWAccelInterface.hpp>
#include <VideoWriter.hpp>
#include <StreamInfo.hpp>
#include <ImgScaler.hpp>

#include <QOpenGLContext>
#include <QDebug>

#include <functional>
#include <memory>

#include <d3d9.h>

#include <GL/gl.h>
#include <GL/wglext.h>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavutil/pixdesc.h>
    #include <libavutil/hwcontext.h>
    #include <libavutil/hwcontext_dxva2.h>
}

using QMPlay2D3dDev9 = std::unique_ptr<IDirect3DDevice9, std::function<void(void *)>>;

DEFINE_GUID(QMPlay2_DXVA2_ModeMPEG2_VLD,      0xee27417f, 0x5e28,0x4e65,0xbe,0xea,0x1d,0x26,0xb5,0x08,0xad,0xc9);
DEFINE_GUID(QMPlay2_DXVA2_ModeMPEG2and1_VLD,  0x86695f12, 0x340e,0x4f04,0x9f,0xd3,0x92,0x53,0xdd,0x32,0x74,0x60);
DEFINE_GUID(QMPlay2_DXVA2_ModeH264_E,         0x1b81be68, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(QMPlay2_DXVA2_ModeH264_F,         0x1b81be69, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(QMPlay2_DXVADDI_Intel_ModeH264_E, 0x604F8E68, 0x4951,0x4C54,0x88,0xFE,0xAB,0xD2,0x5C,0x15,0xB3,0xD6);
DEFINE_GUID(QMPlay2_DXVA2_ModeVC1_D,          0x1b81beA3, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(QMPlay2_DXVA2_ModeVC1_D2010,      0x1b81beA4, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
DEFINE_GUID(QMPlay2_DXVA2_ModeHEVC_VLD_Main,  0x5b11d51b, 0x2f4c,0x4452,0xbc,0xc3,0x09,0xf2,0xa1,0x16,0x0c,0xc0);
DEFINE_GUID(QMPlay2_DXVA2_ModeHEVC_VLD_Main10,0x107af0e0, 0xef1a,0x4d19,0xab,0xa8,0x67,0xa1,0x63,0x07,0x3d,0x13);
DEFINE_GUID(QMPlay2_DXVA2_ModeVP9_VLD_Profile0,0x463707f8,0xa1d0,0x4585,0x87,0x6d,0x83,0xaa,0x6d,0x60,0xb8,0x9e);
DEFINE_GUID(QMPlay2_DXVA2_ModeVP9_VLD_10bit_Profile2,0xa4c749ef,0x6ecf,0x48aa,0x84,0x48,0x50,0xa7,0xa1,0x16,0x5f,0xf7);

DEFINE_GUID(QMPlay2_DXVA2_VideoProcProgressiveDevice, 0x5a54a0c9,0xc7ec,0x4bd9,0x8e,0xde,0xf3,0xc7,0x5d,0xc4,0x39,0x3b);

class DXVA2OpenGL : public HWAccelInterface
{
public:
    DXVA2OpenGL(AVBufferRef *hwDeviceBufferRef)
        : m_hwDeviceBufferRef(av_buffer_ref(hwDeviceBufferRef))
    {
        m_videoSample.PlanarAlpha.ll = 0x10000;
        m_videoSample.SampleFormat.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_MPEG2;

        m_bltParams.Alpha.ll = 0x10000;
        m_bltParams.BackgroundColor.Y = 0x1000;
        m_bltParams.BackgroundColor.Cb = 0x8000;
        m_bltParams.BackgroundColor.Cr = 0x8000;
        m_bltParams.BackgroundColor.Alpha = 0xffff;
        m_bltParams.ProcAmpValues.Contrast.Value = 1;
        m_bltParams.ProcAmpValues.Saturation.Value = 1;
    }
    ~DXVA2OpenGL() final
    {
        if (m_videoProcessor)
            m_videoProcessor->Release();
        if (m_videoProcessorService)
            m_videoProcessorService->Release();
        av_buffer_unref(&m_hwDeviceBufferRef);
    }

    QString name() const override
    {
        return "DXVA2";
    }

    Format getFormat() const override
    {
        return RGB32;
    }

    bool init(quint32 *textures) override
    {
        if (m_glHandleD3D)
        {
            m_textures = textures;
            return true;
        }

        auto context = QOpenGLContext::currentContext();
        if (!context)
        {
            QMPlay2Core.logError("DXVA2 :: Unable to get OpenGL context");
            return false;
        }

        wglDXOpenDeviceNV = (PFNWGLDXOPENDEVICENVPROC)context->getProcAddress("wglDXOpenDeviceNV");
        wglDXSetResourceShareHandleNV = (PFNWGLDXSETRESOURCESHAREHANDLENVPROC)context->getProcAddress("wglDXSetResourceShareHandleNV");
        wglDXRegisterObjectNV = (PFNWGLDXREGISTEROBJECTNVPROC)context->getProcAddress("wglDXRegisterObjectNV");
        wglDXLockObjectsNV = (PFNWGLDXLOCKOBJECTSNVPROC)context->getProcAddress("wglDXLockObjectsNV");
        wglDXUnlockObjectsNV = (PFNWGLDXUNLOCKOBJECTSNVPROC)context->getProcAddress("wglDXUnlockObjectsNV");
        wglDXUnregisterObjectNV = (PFNWGLDXUNREGISTEROBJECTNVPROC)context->getProcAddress("wglDXUnregisterObjectNV");
        wglDXCloseDeviceNV = (PFNWGLDXCLOSEDEVICENVPROC)context->getProcAddress("wglDXCloseDeviceNV");
        if (!wglDXOpenDeviceNV || !wglDXSetResourceShareHandleNV || !wglDXRegisterObjectNV || !wglDXLockObjectsNV || !wglDXUnlockObjectsNV || !wglDXUnregisterObjectNV || !wglDXCloseDeviceNV)
        {
            QMPlay2Core.logError("DXVA2 :: Unable to get DXVA2 interop function pointers");
            return false;
        }

        if (auto d3dDev9 = getD3dDevice9())
            m_glHandleD3D = wglDXOpenDeviceNV(d3dDev9.get());
        if (!m_glHandleD3D)
        {
            QMPlay2Core.logError("DXVA2 :: Unable to initialize DXVA2 <-> GL interop");
            return false;
        }

        m_textures = textures;
        return true;
    }
    void clear(bool contextChange) override
    {
        releaseRenderTarget();
        if (m_glHandleD3D && contextChange)
        {
            wglDXCloseDeviceNV(m_glHandleD3D);
            m_glHandleD3D = nullptr;
        }
        m_textures = nullptr;
    }

    MapResult mapFrame(const VideoFrame &videoFrame, Field field) override
    {
        Q_UNUSED(field)

        if (!unlockRenderTarget())
            return MapError;

        const int width = videoFrame.size.width;
        const int height = videoFrame.size.height;
        if (m_width != width || m_height != height)
        {
            releaseRenderTarget();
            m_width = width;
            m_height = height;
        }

        if (!ensureRenderTarget())
            return MapError;

        const RECT rect = {
            0,
            0,
            (LONG)m_width,
            (LONG)m_height,
        };

        m_videoSample.Start = 0;
        m_videoSample.End = (field == Field::FullFrame) ? 0 : 100;
        switch (field)
        {
            case Field::TopField:
                m_videoSample.SampleFormat.SampleFormat = DXVA2_SampleFieldInterleavedEvenFirst;
                break;
            case Field::BottomField:
                m_videoSample.SampleFormat.SampleFormat = DXVA2_SampleFieldInterleavedOddFirst;
                break;
            default:
                m_videoSample.SampleFormat.SampleFormat = DXVA2_SampleProgressiveFrame;
                break;
        }
        m_videoSample.SampleFormat.NominalRange = videoFrame.limited
            ? DXVA2_NominalRange_16_235
            : DXVA2_NominalRange_0_255
        ;
        switch (videoFrame.colorSpace)
        {
            case QMPlay2ColorSpace::BT709:
                m_videoSample.SampleFormat.VideoTransferMatrix = DXVA2_VideoTransferMatrix_BT709;
                break;
            case QMPlay2ColorSpace::SMPTE240M:
                m_videoSample.SampleFormat.VideoTransferMatrix = DXVA2_VideoTransferMatrix_SMPTE240M;
                break;
            default:
                m_videoSample.SampleFormat.VideoTransferMatrix = DXVA2_VideoTransferMatrix_BT601;
                break;
        }
        m_videoSample.SrcSurface = reinterpret_cast<IDirect3DSurface9 *>(videoFrame.surfaceId);
        m_videoSample.SrcRect = rect;
        m_videoSample.DstRect = rect;

        m_bltParams.TargetFrame = m_videoSample.Start;
        m_bltParams.TargetRect = rect;
        m_bltParams.DestFormat.NominalRange = videoFrame.limited
            ? DXVA2_NominalRange_0_255
            : DXVA2_NominalRange_16_235
        ;

        if (FAILED(m_videoProcessor->VideoProcessBlt(m_renderTarget, &m_bltParams, &m_videoSample, 1, nullptr)))
            return MapError;

        if (!wglDXLockObjectsNV(m_glHandleD3D, 1, &m_glHandleSurface))
            return MapError;

        m_surfaceLocked = true;
        return MapOk;
    }

    bool getImage(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) override
    {
        D3DSURFACE_DESC desc;
        D3DLOCKED_RECT lockedRect;
        IDirect3DSurface9 *surface = (IDirect3DSurface9 *)videoFrame.surfaceId;
        if (SUCCEEDED(surface->GetDesc(&desc)) && SUCCEEDED(surface->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY)))
        {
            const void *src[2] = {
                lockedRect.pBits,
                (quint8 *)lockedRect.pBits + (lockedRect.Pitch * desc.Height)
            };
            const int srcLinesize[2] = {
                lockedRect.Pitch,
                lockedRect.Pitch
            };
            nv12ToRGB32->scale(src, srcLinesize, dest);
            surface->UnlockRect();
            return true;
        }
        return false;
    }

    void getVideAdjustmentCap(VideoAdjustment &videoAdjustmentCap) override
    {
        videoAdjustmentCap.brightness = false;
        videoAdjustmentCap.contrast = false;
        videoAdjustmentCap.saturation = true;
        videoAdjustmentCap.hue = true;
        videoAdjustmentCap.sharpness = false;
    }
    void setVideoAdjustment(const VideoAdjustment &videoAdjustment) override
    {
        const double saturation = videoAdjustment.saturation / 100.0 + 1.0;
        const double hue = videoAdjustment.hue * 1.8;

        m_bltParams.ProcAmpValues.Saturation.Value = saturation;
        m_bltParams.ProcAmpValues.Saturation.Fraction = (saturation - m_bltParams.ProcAmpValues.Saturation.Value) * 65535;

        m_bltParams.ProcAmpValues.Hue.Value = hue;
        m_bltParams.ProcAmpValues.Hue.Fraction = (hue - m_bltParams.ProcAmpValues.Hue.Value) * 65535;
    }

    /**/

    bool checkCodec(const QByteArray &codecName, bool b10)
    {
        auto d3dDev9 = getD3dDevice9();
        if (!d3dDev9)
            return false;

        IDirectXVideoDecoderService *videoDecoderService = nullptr;
        if (FAILED(DXVA2CreateVideoService(d3dDev9.get(), IID_IDirectXVideoDecoderService, (void **)&videoDecoderService)))
            return false;

        bool ok = false;

        UINT count = 0;
        GUID *codecGUIDs = nullptr;
        if (SUCCEEDED(videoDecoderService->GetDecoderDeviceGuids(&count, &codecGUIDs)))
        {
            QVector<const GUID *> neededCodecGUIDs;
            if (codecName == "h264")
            {
                neededCodecGUIDs.append({
                    &QMPlay2_DXVA2_ModeH264_E,
                    &QMPlay2_DXVA2_ModeH264_F,
                    &QMPlay2_DXVADDI_Intel_ModeH264_E,
                });
            }
            else if (codecName == "hevc")
            {
                if (b10)
                    neededCodecGUIDs.append(&QMPlay2_DXVA2_ModeHEVC_VLD_Main10);
                else
                    neededCodecGUIDs.append(&QMPlay2_DXVA2_ModeHEVC_VLD_Main);
            }
            else if (codecName == "vp9")
            {
                if (b10)
                    neededCodecGUIDs.append(&QMPlay2_DXVA2_ModeVP9_VLD_10bit_Profile2);
                else
                    neededCodecGUIDs.append(&QMPlay2_DXVA2_ModeVP9_VLD_Profile0);
            }
            else if (codecName == "vc1")
            {
                neededCodecGUIDs.append({
                    &QMPlay2_DXVA2_ModeVC1_D,
                    &QMPlay2_DXVA2_ModeVC1_D2010,
                });
            }
            else if (codecName == "mpeg2video")
            {
                neededCodecGUIDs.append({
                    &QMPlay2_DXVA2_ModeMPEG2_VLD,
                    &QMPlay2_DXVA2_ModeMPEG2and1_VLD,
                });
            }
            else if (codecName == "mpeg1video")
            {
                neededCodecGUIDs.append({
                    &QMPlay2_DXVA2_ModeMPEG2and1_VLD,
                });
            }

            for (UINT i = 0; i < count; ++i)
            {
                for (const GUID *guid : asConst(neededCodecGUIDs))
                {
                    if (IsEqualGUID(codecGUIDs[i], *guid))
                    {
                        ok = true;
                        break;
                    }
                }
                if (ok)
                    break;
            }

            CoTaskMemFree(codecGUIDs);
        }

        videoDecoderService->Release();

        return ok;
    }

    bool initVideoProcessor()
    {
        auto d3dDev9 = getD3dDevice9();
        if (!d3dDev9)
            return false;

        if (FAILED(DXVA2CreateVideoService(d3dDev9.get(), IID_IDirectXVideoProcessorService, (void **)&m_videoProcessorService)))
        {
            QMPlay2Core.logError("DXVA2 :: Unable to create video processor service");
            return false;
        }

        DXVA2_VideoDesc videoDesc = {};
        if (FAILED(m_videoProcessorService->CreateVideoProcessor(QMPlay2_DXVA2_VideoProcProgressiveDevice, &videoDesc, D3DFMT_X8R8G8B8, 0, &m_videoProcessor)))
        {
            QMPlay2Core.logError("DXVA2 :: Unable to create video processor");
            return false;
        }

        return true;
    }

private:
    QMPlay2D3dDev9 getD3dDevice9()
    {
        const auto d3dDevMgr9 = ((AVDXVA2DeviceContext *)((AVHWDeviceContext *)m_hwDeviceBufferRef->data)->hwctx)->devmgr;

        HANDLE devHandle = nullptr;
        if (FAILED(d3dDevMgr9->OpenDeviceHandle(&devHandle)))
            return nullptr;

        IDirect3DDevice9 *d3dDev9 = nullptr;
        if (FAILED(d3dDevMgr9->LockDevice(devHandle, &d3dDev9, true)))
        {
            d3dDevMgr9->CloseDeviceHandle(devHandle);
            return nullptr;
        }

        return QMPlay2D3dDev9(d3dDev9, [=](void *) {
            d3dDevMgr9->UnlockDevice(devHandle, false);
            d3dDevMgr9->CloseDeviceHandle(devHandle);
        });
    }

    bool ensureRenderTarget()
    {
        if (m_renderTarget)
            return true;

        Q_ASSERT(!m_glHandleSurface);

        HANDLE sharedHandle = nullptr;

#if 1
        auto d3dDev9 = getD3dDevice9();
        if (!d3dDev9)
            return false;

        HRESULT hr = d3dDev9->CreateRenderTarget(
            m_width,
            m_height,
            D3DFMT_X8R8G8B8,
            D3DMULTISAMPLE_NONE,
            0,
            false,
            &m_renderTarget,
            &sharedHandle
        );
#else
        HRESULT hr = m_videoProcessorService->CreateSurface(
            m_width,
            m_height,
            0,
            D3DFMT_X8R8G8B8,
            D3DPOOL_DEFAULT,
            0,
            DXVA2_VideoProcessorRenderTarget,
            &m_renderTarget,
            &sharedHandle
        );
#endif
        if (FAILED(hr))
            return false;

        if (!wglDXSetResourceShareHandleNV(m_renderTarget, sharedHandle))
            return false;

        m_glHandleSurface = wglDXRegisterObjectNV(m_glHandleD3D, m_renderTarget, m_textures[0], GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV);
        if (!m_glHandleSurface)
            return false;

        return true;
    }
    bool unlockRenderTarget()
    {
        if (!m_surfaceLocked)
            return true;

        Q_ASSERT(m_glHandleSurface);
        if (wglDXUnlockObjectsNV(m_glHandleD3D, 1, &m_glHandleSurface))
        {
            m_surfaceLocked = false;
            return true;
        }

        return false;
    }
    void releaseRenderTarget()
    {
        unlockRenderTarget();
        if (m_glHandleSurface)
        {
            wglDXUnregisterObjectNV(m_glHandleD3D, m_glHandleSurface);
            m_glHandleSurface = nullptr;
        }
        if (m_renderTarget)
        {
            m_renderTarget->Release();
            m_renderTarget = nullptr;
        }
        m_width = m_height = 0;
    }

public:
    AVBufferRef *m_hwDeviceBufferRef = nullptr;

private:
    PFNWGLDXOPENDEVICENVPROC wglDXOpenDeviceNV = nullptr;
    PFNWGLDXSETRESOURCESHAREHANDLENVPROC wglDXSetResourceShareHandleNV = nullptr;
    PFNWGLDXREGISTEROBJECTNVPROC wglDXRegisterObjectNV = nullptr;
    PFNWGLDXLOCKOBJECTSNVPROC wglDXLockObjectsNV = nullptr;
    PFNWGLDXUNLOCKOBJECTSNVPROC wglDXUnlockObjectsNV = nullptr;
    PFNWGLDXUNREGISTEROBJECTNVPROC wglDXUnregisterObjectNV = nullptr;
    PFNWGLDXCLOSEDEVICENVPROC wglDXCloseDeviceNV = nullptr;

    IDirectXVideoProcessorService *m_videoProcessorService = nullptr;
    IDirectXVideoProcessor *m_videoProcessor = nullptr;
    DXVA2_VideoSample m_videoSample = {};
    DXVA2_VideoProcessBltParams m_bltParams = {};

    quint32 *m_textures = nullptr;

    HANDLE m_glHandleD3D = nullptr;

    IDirect3DSurface9 *m_renderTarget = nullptr;
    HANDLE m_glHandleSurface = nullptr;
    bool m_surfaceLocked = false;
    int m_width = 0;
    int m_height = 0;
};

static AVPixelFormat dxva2GetFormat(AVCodecContext *codecCtx, const AVPixelFormat *pixFmt)
{
    while (*pixFmt != AV_PIX_FMT_NONE)
    {
        if (*pixFmt == AV_PIX_FMT_DXVA2_VLD)
            return *pixFmt;
        ++pixFmt;
    }
    return AV_PIX_FMT_NONE;
}

/**/

FFDecDXVA2::FFDecDXVA2(Module &module)
{
    SetModule(module);
}
FFDecDXVA2::~FFDecDXVA2()
{
    if (m_swsCtx)
        sws_freeContext(m_swsCtx);
    av_buffer_unref(&m_hwDeviceBufferRef);
}

bool FFDecDXVA2::set()
{
    const bool copyVideo = sets().getBool("CopyVideoDXVA2");
    if (copyVideo != m_copyVideo)
    {
        m_copyVideo = copyVideo;
        return false;
    }
    return sets().getBool("DecoderDXVA2Enabled");
}

QString FFDecDXVA2::name() const
{
    return "FFmpeg/DXVA2";
}

int FFDecDXVA2::decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurryUp)
{
    int ret = FFDecHWAccel::decodeVideo(encodedPacket, decoded, newPixFmt, flush, hurryUp);
    if (m_hwAccelWriter && ret > -1)
    {
        decoded.setAVFrame(frame);
    }
    return ret;
}
void FFDecDXVA2::downloadVideoFrame(VideoFrame &decoded)
{
    D3DSURFACE_DESC desc;
    D3DLOCKED_RECT lockedRect;
    IDirect3DSurface9 *surface = (IDirect3DSurface9 *)frame->data[3];
    if (SUCCEEDED(surface->GetDesc(&desc)) && SUCCEEDED(surface->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY)))
    {
        AVBufferRef *dstBuffer[3] = {
            av_buffer_alloc(lockedRect.Pitch * frame->height),
            av_buffer_alloc((lockedRect.Pitch / 2) * ((frame->height + 1) / 2)),
            av_buffer_alloc((lockedRect.Pitch / 2) * ((frame->height + 1) / 2))
        };

        quint8 *srcData[2] = {
            (quint8 *)lockedRect.pBits,
            (quint8 *)lockedRect.pBits + (lockedRect.Pitch * desc.Height)
        };
        qint32 srcLinesize[2] = {
            lockedRect.Pitch,
            lockedRect.Pitch
        };

        uint8_t *dstData[3] = {
            dstBuffer[0]->data,
            dstBuffer[1]->data,
            dstBuffer[2]->data
        };
        qint32 dstLinesize[3] = {
            lockedRect.Pitch,
            lockedRect.Pitch / 2,
            lockedRect.Pitch / 2
        };

        m_swsCtx = sws_getCachedContext(m_swsCtx, frame->width, frame->height, AV_PIX_FMT_NV12, frame->width, frame->height, AV_PIX_FMT_YUV420P, SWS_POINT, nullptr, nullptr, nullptr);
        sws_scale(m_swsCtx, srcData, srcLinesize, 0, frame->height, dstData, dstLinesize);

        decoded = VideoFrame(VideoFrameSize(frame->width, frame->height), dstBuffer, dstLinesize, frame->interlaced_frame, frame->top_field_first);

        surface->UnlockRect();
    }
}

bool FFDecDXVA2::open(StreamInfo &streamInfo, VideoWriter *writer)
{
    const AVPixelFormat pixFmt = av_get_pix_fmt(streamInfo.format);
    if (pixFmt != AV_PIX_FMT_YUV420P && (pixFmt != AV_PIX_FMT_YUV420P10 || m_copyVideo))
        return false;

    AVCodec *codec = init(streamInfo);
    if (!codec || !hasHWAccel("dxva2"))
        return false;

    DXVA2OpenGL *dxva2OpenGL = nullptr;

    if (writer)
    {
        dxva2OpenGL = dynamic_cast<DXVA2OpenGL *>(writer->getHWAccelInterface());
        if (dxva2OpenGL)
        {
            m_hwDeviceBufferRef = av_buffer_ref(dxva2OpenGL->m_hwDeviceBufferRef);
            m_hwAccelWriter = writer;
        }
    }

    if (!dxva2OpenGL)
    {
        if (av_hwdevice_ctx_create(&m_hwDeviceBufferRef, AV_HWDEVICE_TYPE_DXVA2, nullptr, nullptr, 0) != 0)
            return false;

        dxva2OpenGL = new DXVA2OpenGL(m_hwDeviceBufferRef);
        if (!dxva2OpenGL->initVideoProcessor())
        {
            delete dxva2OpenGL;
            return false;
        }
    }

    if (!dxva2OpenGL->checkCodec(streamInfo.codec_name, pixFmt == AV_PIX_FMT_YUV420P10))
        return false;

    if (!m_hwAccelWriter && !m_copyVideo)
    {
        m_hwAccelWriter = VideoWriter::createOpenGL2(dxva2OpenGL);
        if (!m_hwAccelWriter)
            return false;
    }

    codec_ctx->hw_device_ctx = av_buffer_ref(m_hwDeviceBufferRef);
    codec_ctx->get_format = dxva2GetFormat;
    codec_ctx->thread_count = 1;
    if (!openCodec(codec))
        return false;

    time_base = streamInfo.getTimeBase();
    return true;
}
