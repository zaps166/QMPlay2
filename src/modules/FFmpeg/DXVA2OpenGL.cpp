/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <DXVA2OpenGL.hpp>

#include <QMPlay2Core.hpp>
#include <ImgScaler.hpp>
#include <Frame.hpp>

#include <QLibrary>

extern "C"
{
    #include <libavutil/hwcontext.h>
    #include <libavutil/hwcontext_dxva2.h>
}

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

DXVA2OpenGL::DXVA2OpenGL(AVBufferRef *hwDeviceBufferRef)
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
DXVA2OpenGL::~DXVA2OpenGL()
{
    if (m_dontReleaseRenderTarget)
    {
        for (int i = 0; i < s_numRenderTargets; ++i)
        {
            for (auto &&renderTarget : qAsConst(m_renderTargets[i].surfaces))
                renderTarget->Release();
        }
    }
    if (m_videoProcessor)
        m_videoProcessor->Release();
    av_buffer_unref(&m_hwDeviceBufferRef);
}

QString DXVA2OpenGL::name() const
{
    return "DXVA2";
}

DXVA2OpenGL::Format DXVA2OpenGL::getFormat() const
{
    return RGB32;
}

bool DXVA2OpenGL::init(const int *widths, const int *heights, const SetTextureParamsFn &setTextureParamsFn)
{
    if (m_width != widths[0] || m_height != heights[0])
    {
        clearTextures();

        m_width = widths[0];
        m_height = heights[0];

        glGenTextures(2, m_textures);
    }

    for (int i = 0; i < 2; ++i)
        setTextureParamsFn(m_textures[i]);

    if (m_glHandleD3D)
    {
        const bool ret = ensureRenderTargets();
        if (!ret)
            m_error = true;
        return ret;
    }

    auto context = QOpenGLContext::currentContext();
    if (!context)
    {
        QMPlay2Core.logError("DXVA2 :: Unable to get OpenGL context");
        m_error = true;
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
        m_error = true;
        return false;
    }

    if (auto d3dDev9 = getD3dDevice9())
    {
        m_glHandleD3D = wglDXOpenDeviceNV(d3dDev9.get());

        IDirect3D9 *d3d9 = nullptr;
        if (SUCCEEDED(d3dDev9->GetDirect3D(&d3d9)))
        {
            D3DADAPTER_IDENTIFIER9 adapterIdentifier = {};
            if (SUCCEEDED(d3d9->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &adapterIdentifier)))
            {
                // Don't release render target on old Intel drivers, bacause "wglDXSetResourceShareHandleNV()" can fail.
                // Render target must be recreated on Radeon drivers, otherwise "wglDXRegisterObjectNV()" will fail.
                m_dontReleaseRenderTarget = (strstr(adapterIdentifier.Description, "Intel") != nullptr);
            }
            d3d9->Release();
        }
    }
    if (!m_glHandleD3D)
    {
        QMPlay2Core.logError("DXVA2 :: Unable to initialize DXVA2 <-> GL interop");
        m_error = true;
        return false;
    }

    if (!ensureRenderTargets())
    {
        m_error = true;
        return false;
    }

    return true;
}
void DXVA2OpenGL::clear()
{
    clearTextures();
    if (m_glHandleD3D)
    {
        wglDXCloseDeviceNV(m_glHandleD3D);
        m_glHandleD3D = nullptr;
    }
}

bool DXVA2OpenGL::mapFrame(Frame &videoFrame)
{
    if (!unlockRenderTargets())
    {
        m_error = true;
        return false;
    }

    const RECT rect = {
        0,
        0,
        (LONG)m_width,
        (LONG)m_height,
    };

    m_videoSample.Start = 0;
    if (videoFrame.isInterlaced())
    {
        m_videoSample.End = 100;
        if (videoFrame.isTopFieldFirst() == videoFrame.isSecondField())
            m_videoSample.SampleFormat.SampleFormat = DXVA2_SampleFieldInterleavedOddFirst;
        else
            m_videoSample.SampleFormat.SampleFormat = DXVA2_SampleFieldInterleavedEvenFirst;
    }
    else
    {
        m_videoSample.End = 0;
        m_videoSample.SampleFormat.SampleFormat = DXVA2_SampleProgressiveFrame;
    }
    m_videoSample.SampleFormat.NominalRange = videoFrame.isLimited()
        ? DXVA2_NominalRange_16_235
        : DXVA2_NominalRange_0_255
    ;
    switch (videoFrame.colorSpace())
    {
        case AVCOL_SPC_BT709:
            m_videoSample.SampleFormat.VideoTransferMatrix = DXVA2_VideoTransferMatrix_BT709;
            break;
        case AVCOL_SPC_SMPTE240M:
            m_videoSample.SampleFormat.VideoTransferMatrix = DXVA2_VideoTransferMatrix_SMPTE240M;
            break;
        default:
            m_videoSample.SampleFormat.VideoTransferMatrix = DXVA2_VideoTransferMatrix_BT601;
            break;
    }
    m_videoSample.SrcSurface = reinterpret_cast<IDirect3DSurface9 *>(videoFrame.hwData());
    m_videoSample.SrcRect = rect;
    m_videoSample.DstRect = rect;

    m_bltParams.TargetFrame = m_videoSample.Start;
    m_bltParams.TargetRect = rect;
    m_bltParams.DestFormat.NominalRange = videoFrame.isLimited()
        ? DXVA2_NominalRange_0_255
        : DXVA2_NominalRange_16_235
    ;

    if (FAILED(m_videoProcessor->VideoProcessBlt(m_renderTargets[m_renderTargetIdx].surface, &m_bltParams, &m_videoSample, 1, nullptr)))
    {
        m_error = true;
        return false;
    }

    m_renderTargetIdx = (m_renderTargetIdx + 1) % s_numRenderTargets;

    if (!wglDXLockObjectsNV(m_glHandleD3D, 1, &m_renderTargets[m_renderTargetIdx].glSurface))
    {
        m_error = true;
        return false;
    }

    m_renderTargets[m_renderTargetIdx].locked = true;
    return true;
}
quint32 DXVA2OpenGL::getTexture(int plane)
{
    Q_UNUSED(plane)
    return m_textures[m_renderTargetIdx];
}

QImage DXVA2OpenGL::getImage(const Frame &videoFrame)
{
    D3DSURFACE_DESC desc;
    D3DLOCKED_RECT lockedRect;
    IDirect3DSurface9 *surface = (IDirect3DSurface9 *)videoFrame.hwData();
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

        QImage img;

        ImgScaler imgScaler;
        if (imgScaler.create(videoFrame))
        {
            img = QImage(videoFrame.width(), videoFrame.height(), QImage::Format_RGB32);
            imgScaler.scale(src, srcLinesize, img.bits());
        }

        surface->UnlockRect();
        return img;
    }
    return QImage();
}

void DXVA2OpenGL::getVideAdjustmentCap(VideoAdjustment &videoAdjustmentCap)
{
    videoAdjustmentCap.brightness = false;
    videoAdjustmentCap.contrast = false;
    videoAdjustmentCap.saturation = true;
    videoAdjustmentCap.hue = true;
    videoAdjustmentCap.sharpness = false;
}
void DXVA2OpenGL::setVideoAdjustment(const VideoAdjustment &videoAdjustment)
{
    const double saturation = videoAdjustment.saturation / 100.0 + 1.0;
    const double hue = videoAdjustment.hue * 1.8;

    m_bltParams.ProcAmpValues.Saturation.Value = saturation;
    m_bltParams.ProcAmpValues.Saturation.Fraction = (saturation - m_bltParams.ProcAmpValues.Saturation.Value) * 65535;

    m_bltParams.ProcAmpValues.Hue.Value = hue;
    m_bltParams.ProcAmpValues.Hue.Fraction = (hue - m_bltParams.ProcAmpValues.Hue.Value) * 65535;
}

bool DXVA2OpenGL::checkCodec(const QByteArray &codecName, bool b10)
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
            for (const GUID *guid : qAsConst(neededCodecGUIDs))
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

bool DXVA2OpenGL::initVideoProcessor()
{
    QLibrary dxva2("dxva2.dll");
    if (dxva2.load())
        DXVA2CreateVideoService = reinterpret_cast<DXVA2CreateVideoServiceFn>(dxva2.resolve("DXVA2CreateVideoService"));
    if (!DXVA2CreateVideoService)
        return false;

    auto d3dDev9 = getD3dDevice9();
    if (!d3dDev9)
        return false;

    IDirectXVideoProcessorService *videoProcessorService = nullptr;
    if (FAILED(DXVA2CreateVideoService(d3dDev9.get(), IID_IDirectXVideoProcessorService, (void **)&videoProcessorService)))
    {
        QMPlay2Core.logError("DXVA2 :: Unable to create video processor service");
        return false;
    }

    DXVA2_VideoDesc videoDesc = {};
    if (FAILED(videoProcessorService->CreateVideoProcessor(QMPlay2_DXVA2_VideoProcProgressiveDevice, &videoDesc, D3DFMT_X8R8G8B8, 0, &m_videoProcessor)))
    {
        QMPlay2Core.logError("DXVA2 :: Unable to create video processor");
        videoProcessorService->Release();
        return false;
    }

    videoProcessorService->Release();

    return true;
}

QMPlay2D3dDev9 DXVA2OpenGL::getD3dDevice9()
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

bool DXVA2OpenGL::ensureRenderTargets()
{
    for (int i = 0; i < s_numRenderTargets; ++i)
    {
        auto &surface = m_renderTargets[i].surface;
        if (surface)
            continue;

        auto &glSurface = m_renderTargets[i].glSurface;
        Q_ASSERT(!glSurface);

        const auto size = qMakePair(m_width, m_height);
        auto &surfaces = m_renderTargets[i].surfaces;

        if (m_dontReleaseRenderTarget)
            surface = surfaces.value(size);
        if (!surface)
        {
            HANDLE sharedHandle = nullptr;

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
                &surface,
                &sharedHandle
            );
            if (FAILED(hr))
                return false;

            if (m_dontReleaseRenderTarget)
                surfaces[size] = surface;

            if (!wglDXSetResourceShareHandleNV(surface, sharedHandle))
                return false;
        }

        glSurface = wglDXRegisterObjectNV(m_glHandleD3D, surface, m_textures[i], GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV);
        if (!glSurface)
            return false;
    }

    return true;
}
bool DXVA2OpenGL::unlockRenderTargets()
{
    bool ok = true;
    for (int i = 0; i < s_numRenderTargets; ++i)
    {
        if (!m_renderTargets[i].locked)
            continue;

        Q_ASSERT(m_renderTargets[i].glSurface);
        if (wglDXUnlockObjectsNV(m_glHandleD3D, 1, &m_renderTargets[i].glSurface))
            m_renderTargets[i].locked = false;
        else
            ok = false;
    }
    return ok;
}

void DXVA2OpenGL::clearTextures()
{
    unlockRenderTargets();
    for (int i = 0; i < s_numRenderTargets; ++i)
    {
        if (m_renderTargets[i].glSurface)
        {
            wglDXUnregisterObjectNV(m_glHandleD3D, m_renderTargets[i].glSurface);
            m_renderTargets[i].glSurface = nullptr;
        }
        if (m_renderTargets[i].surface)
        {
            if (!m_dontReleaseRenderTarget)
                m_renderTargets[i].surface->Release();
            m_renderTargets[i].surface = nullptr;
        }
    }

    glDeleteTextures(2, m_textures);
    memset(m_textures, 0, sizeof(m_textures));
    m_width = m_height = 0;
}
