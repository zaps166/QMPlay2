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

#include <FFDecVTB.hpp>

#include <GPUInstance.hpp>
#include <VideoWriter.hpp>
#include <StreamInfo.hpp>
#include <FFCommon.hpp>
#ifdef USE_OPENGL
#   include <VTBOpenGL.hpp>
#endif

#include <QDebug>

extern "C"
{
    #include <libavutil/pixdesc.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavutil/hwcontext.h>
    #include <libavutil/hwcontext_videotoolbox.h>
}

using namespace std;

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
        decoded.setVideoData(dstBuffer, dstLinesize);
    }
}

bool FFDecVTB::open(StreamInfo &streamInfo)
{
    if (streamInfo.codec_type != AVMEDIA_TYPE_VIDEO)
        return false;

    const AVPixelFormat pix_fmt = streamInfo.pixelFormat();
    if (pix_fmt != AV_PIX_FMT_YUV420P)
        return false;

    AVCodec *codec = init(streamInfo);
    if (!codec || !hasHWAccel("videotoolbox"))
        return false;

#ifdef USE_OPENGL
    shared_ptr<VTBOpenGL> vtbOpenGL;

    if (QMPlay2Core.renderer() == QMPlay2CoreClass::Renderer::OpenGL)
    {
        vtbOpenGL = QMPlay2Core.gpuInstance()->getHWDecContext<VTBOpenGL>();
        if (vtbOpenGL)
            m_hwDeviceBufferRef = av_buffer_ref(vtbOpenGL->m_hwDeviceBufferRef);
    }
#endif

    if (!m_hwDeviceBufferRef && av_hwdevice_ctx_create(&m_hwDeviceBufferRef, AV_HWDEVICE_TYPE_VIDEOTOOLBOX, nullptr, nullptr, 0) != 0)
        return false;

#ifdef USE_OPENGL
    if (QMPlay2Core.renderer() == QMPlay2CoreClass::Renderer::OpenGL && !vtbOpenGL)
    {
        vtbOpenGL = make_shared<VTBOpenGL>(m_hwDeviceBufferRef);
        if (!QMPlay2Core.gpuInstance()->setHWDecContextForVideoOutput(vtbOpenGL))
            return false;
    }

    if (vtbOpenGL)
        m_hasHWDecContext = true;
#endif

    codec_ctx->hw_device_ctx = av_buffer_ref(m_hwDeviceBufferRef);
    codec_ctx->get_format = vtbGetFormat;
    codec_ctx->thread_count = 1;
    if (!openCodec(codec))
        return false;

    m_timeBase = streamInfo.time_base;
    return true;
}
