/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

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

#include <QOperatingSystemVersion>
#include <QDebug>

extern "C"
{
    #include <libavutil/pixdesc.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/hwcontext.h>
    #include <libavutil/hwcontext_videotoolbox.h>
    #include <libavcodec/videotoolbox.h>
}

using namespace std;

static bool isCodecSupported(const StreamInfo &streamInfo)
{
    switch (streamInfo.params->codec_id)
    {
        case AV_CODEC_ID_H263:
        case AV_CODEC_ID_H264:
        case AV_CODEC_ID_MPEG1VIDEO:
        case AV_CODEC_ID_MPEG2VIDEO:
        case AV_CODEC_ID_MPEG4:
            return true;
        case AV_CODEC_ID_HEVC:
        {
            const auto version = QOperatingSystemVersion::current();
            return (version.majorVersion() > 10 || (version.majorVersion() == 10 && version.minorVersion() >= 13));
        }
        case AV_CODEC_ID_VP9:
            return (QOperatingSystemVersion::current().majorVersion() >= 11);
        default:
            break;
    }
    return false;
}

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

bool FFDecVTB::open(StreamInfo &streamInfo)
{
    if (streamInfo.params->codec_type != AVMEDIA_TYPE_VIDEO || !hasHWAccel("videotoolbox"))
        return false;

    const AVPixelFormat pix_fmt = streamInfo.pixelFormat();
    if (pix_fmt == AV_PIX_FMT_YUV420P10)
    {
        if (streamInfo.params->codec_id == AV_CODEC_ID_H264)
            return false;
    }
    else if (pix_fmt != AV_PIX_FMT_YUV420P && pix_fmt != AV_PIX_FMT_YUVJ420P)
    {
        return false;
    }

    if (!isCodecSupported(streamInfo))
    {
        qWarning() << streamInfo.codec_name << "is not supported by VTB";
        return false;
    }

    AVCodec *codec = init(streamInfo);
    if (!codec)
    {
        qWarning() << "VTB: no or unsupported codec";
        return false;
    }

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
    {
        qWarning() << "VTB: failed to create hwdevice_ctx";
        return false;
    }

#ifdef USE_OPENGL
    if (QMPlay2Core.renderer() == QMPlay2CoreClass::Renderer::OpenGL && !vtbOpenGL)
    {
        vtbOpenGL = make_shared<VTBOpenGL>(m_hwDeviceBufferRef);
        if (!QMPlay2Core.gpuInstance()->setHWDecContextForVideoOutput(vtbOpenGL))
        {
            qWarning() << "VTB: failed to set VTB GPU context";
            return false;
        }
    }

    if (vtbOpenGL)
        m_hasHWDecContext = true;
#endif

    codec_ctx->hw_device_ctx = av_buffer_ref(m_hwDeviceBufferRef);
    codec_ctx->get_format = vtbGetFormat;
    codec_ctx->thread_count = 1;
    if (!openCodec(codec))
    {
        qWarning() << "VTB: failed to open codec";
        return false;
    }

    m_timeBase = streamInfo.time_base;
    return true;
}
