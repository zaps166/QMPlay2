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

#include <FFDecVTB.hpp>

#include <HWAccelInterface.hpp>
#include <VideoWriter.hpp>
#include <StreamInfo.hpp>
#include <ImgScaler.hpp>
#include <FFCommon.hpp>

#include <QDebug>

extern "C"
{
    #include <libavutil/pixdesc.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavutil/hwcontext.h>
    #include <libavutil/hwcontext_videotoolbox.h>
}

#include <gl.h>

/**/

static AVPixelFormat vtbGetFormat(AVCodecContext *codecCtx, const AVPixelFormat *pixFmt)
{
    Q_UNUSED(codecCtx)
    while (*pixFmt != AV_PIX_FMT_NONE)
    {
        if (*pixFmt == AV_PIX_FMT_VIDEOTOOLBOX)
            return *pixFmt;
        ++pixFmt;
    }
    return AV_PIX_FMT_NONE;
}

/**/

class VTBOpenGL final : public HWAccelInterface
{
public:
    VTBOpenGL(AVBufferRef *hwDeviceBufferRef)
        : m_hwDeviceBufferRef(av_buffer_ref(hwDeviceBufferRef))
    {}
    ~VTBOpenGL()
    {
        av_buffer_unref(&m_hwDeviceBufferRef);
    }

    QString name() const override
    {
        return "VideoToolBox";
    }

    Format getFormat() const override
    {
        return NV12;
    }
    bool isTextureRectangle() const override
    {
        return true;
    }
    bool isCopy() const override
    {
        return false;
    }

    bool init(const int *widths, const int *heights, const SetTextureParamsFn &setTextureParamsFn) override
    {
        for (int p = 0; p < 2; ++p)
        {
            if (m_widths[p] != widths[p] || m_heights[p] != heights[p])
            {
                clear();
                for (int p = 0; p < 2; ++p)
                {
                    m_widths[p] = widths[p];
                    m_heights[p] = heights[p];
                }
                glGenTextures(2, m_textures);
                break;
            }
        }
        for (int p = 0; p < 2; ++p)
            setTextureParamsFn(m_textures[p]);
        return true;
    }
    void clear() override
    {
        glDeleteTextures(2, m_textures);
        memset(m_textures, 0, sizeof(m_textures));
        memset(m_widths, 0, sizeof(m_widths));
        memset(m_heights, 0, sizeof(m_heights));
    }

    MapResult mapFrame(const Frame &videoFrame, Field field) override
    {
        Q_UNUSED(field)

        CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)videoFrame.hwSurface();
        CGLContextObj glCtx = CGLGetCurrentContext();

        IOSurfaceRef surface = CVPixelBufferGetIOSurface(pixelBuffer);

        const OSType pixelFormat = IOSurfaceGetPixelFormat(surface);
        if (pixelFormat != kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange)
            return MapError;

        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_textures[0]);
        if (CGLTexImageIOSurface2D(glCtx, GL_TEXTURE_RECTANGLE_ARB, GL_R8, videoFrame.width(0), videoFrame.height(0), GL_RED, GL_UNSIGNED_BYTE, surface, 0) != kCGLNoError)
            return MapError;

        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_textures[1]);
        if (CGLTexImageIOSurface2D(glCtx, GL_TEXTURE_RECTANGLE_ARB, GL_RG8, videoFrame.width(1), videoFrame.height(1), GL_RG, GL_UNSIGNED_BYTE, surface, 1) != kCGLNoError)
            return MapError;

        return MapOk;
    }
    quint32 getTexture(int plane) override
    {
        return m_textures[plane];
    }

    bool getImage(const Frame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) override
    {
        CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)videoFrame.hwSurface();
        if (CVPixelBufferGetPixelFormatType(pixelBuffer) == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange)
        {
            CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

            const quint8 *srcData[2] = {
                (const quint8 *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0),
                (const quint8 *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1)
            };
            const qint32 srcLinesize[2] = {
                (qint32)CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0),
                (qint32)CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1)
            };

            nv12ToRGB32->scale((const void **)srcData, srcLinesize, dest);

            CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

            return true;
        }

        return false;
    }

public:
    AVBufferRef *m_hwDeviceBufferRef = nullptr;

private:
    quint32 m_textures[2] = {};

    int m_widths[2] = {};
    int m_heights[2] = {};
};

/**/

FFDecVTB::FFDecVTB(Module &module)
{
    SetModule(module);
}
FFDecVTB::~FFDecVTB()
{
    if (m_swsCtx)
        sws_freeContext(m_swsCtx);
    av_buffer_unref(&m_hwDeviceBufferRef);
}

bool FFDecVTB::set()
{
    const bool copyVideo = sets().getBool("CopyVideoVTB");
    if (copyVideo != m_copyVideo)
    {
        m_copyVideo = copyVideo;
        return false;
    }
    return sets().getBool("DecoderVTBEnabled");
}

QString FFDecVTB::name() const
{
    return "FFmpeg/VideoToolBox";
}

void FFDecVTB::downloadVideoFrame(Frame &decoded)
{
    CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)frame->data[3];
    if (!pixelBuffer)
        return;

    if (CVPixelBufferGetPixelFormatType(pixelBuffer) == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange)
    {
        const size_t w = CVPixelBufferGetWidth(pixelBuffer);
        const size_t h = CVPixelBufferGetHeight(pixelBuffer);

        CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

        const quint8 *srcData[2] = {
            (const quint8 *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0),
            (const quint8 *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1)
        };
        const qint32 srcLinesize[2] = {
            (qint32)CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0),
            (qint32)CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1)
        };

        AVBufferRef *dstBuffer[3] = {
            av_buffer_alloc(srcLinesize[0] * h),
            av_buffer_alloc((srcLinesize[1] / 2) * ((h + 1) / 2)),
            av_buffer_alloc((srcLinesize[1] / 2) * ((h + 1) / 2))
        };

        quint8 *dstData[3] = {
            dstBuffer[0]->data,
            dstBuffer[1]->data,
            dstBuffer[2]->data
        };
        const qint32 dstLinesize[3] = {
            srcLinesize[0],
            srcLinesize[1] / 2,
            srcLinesize[1] / 2
        };

        m_swsCtx = sws_getCachedContext(m_swsCtx, w, h, AV_PIX_FMT_NV12, frame->width, frame->height, AV_PIX_FMT_YUV420P, SWS_POINT, nullptr, nullptr, nullptr);
        sws_scale(m_swsCtx, srcData, srcLinesize, 0, h, dstData, dstLinesize);

        CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

        decoded = Frame::createEmpty(frame, false, AV_PIX_FMT_YUV420P);
        decoded.setVideoData(dstBuffer, dstLinesize, false);
    }
}

bool FFDecVTB::open(StreamInfo &streamInfo, VideoWriter *writer)
{
    const AVPixelFormat pix_fmt = streamInfo.pixelFormat();
    if (pix_fmt != AV_PIX_FMT_YUV420P)
        return false;

    AVCodec *codec = init(streamInfo);
    if (!codec || !hasHWAccel("videotoolbox"))
        return false;

    if (writer)
    {
        if (auto vtbOpenGL = dynamic_cast<VTBOpenGL *>(writer->getHWAccelInterface()))
        {
            m_hwDeviceBufferRef = av_buffer_ref(vtbOpenGL->m_hwDeviceBufferRef);
            m_hwAccelWriter = writer;
        }
    }

    if (!m_hwDeviceBufferRef)
    {
        if (av_hwdevice_ctx_create(&m_hwDeviceBufferRef, AV_HWDEVICE_TYPE_VIDEOTOOLBOX, nullptr, nullptr, 0) != 0)
            return false;
    }

    if (!m_hwAccelWriter && !m_copyVideo)
    {
        m_hwAccelWriter = VideoWriter::createOpenGL2(new VTBOpenGL(m_hwDeviceBufferRef));
        if (!m_hwAccelWriter)
            return false;
    }

    codec_ctx->hw_device_ctx = av_buffer_ref(m_hwDeviceBufferRef);
    codec_ctx->get_format = vtbGetFormat;
    codec_ctx->thread_count = 1;
    if (!openCodec(codec))
        return false;

    time_base = streamInfo.getTimeBase();
    return true;
}
