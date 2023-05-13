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

#include <FFDecDXVA2.hpp>

#include <DeintHWPrepareFilter.hpp>
#include <GPUInstance.hpp>
#include <StreamInfo.hpp>
#ifdef USE_OPENGL
#   include <DXVA2OpenGL.hpp>
#endif

#include <QDebug>

#include <d3d9.h>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavutil/hwcontext.h>
    #include <libavutil/hwcontext_dxva2.h>
}

using namespace std;

static AVPixelFormat dxva2GetFormat(AVCodecContext *codecCtx, const AVPixelFormat *pixFmt)
{
    Q_UNUSED(codecCtx)
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
    av_buffer_unref(&m_hwDeviceBufferRef);
}

bool FFDecDXVA2::set()
{
    return sets().getBool("DecoderDXVA2Enabled");
}

QString FFDecDXVA2::name() const
{
    return "FFmpeg/DXVA2";
}

shared_ptr<VideoFilter> FFDecDXVA2::hwAccelFilter() const
{
    return m_filter;
}

bool FFDecDXVA2::open(StreamInfo &streamInfo)
{
    if (streamInfo.params->codec_type != AVMEDIA_TYPE_VIDEO)
        return false;

    m_pixFmt = Frame::convert3PlaneTo2Plane(streamInfo.pixelFormat());
    if (m_pixFmt != AV_PIX_FMT_NV12 && (m_pixFmt != AV_PIX_FMT_P016 || streamInfo.params->codec_id == AV_CODEC_ID_H264))
        return false;

    AVCodec *codec = init(streamInfo);
    if (!codec || !hasHWAccel("dxva2"))
        return false;

#ifdef USE_OPENGL
    shared_ptr<DXVA2OpenGL> dxva2OpenGL;

    if (QMPlay2Core.renderer() == QMPlay2CoreClass::Renderer::OpenGL)
    {
        dxva2OpenGL = QMPlay2Core.gpuInstance()->getHWDecContext<DXVA2OpenGL>();
        if (dxva2OpenGL)
            m_hwDeviceBufferRef = av_buffer_ref(dxva2OpenGL->m_hwDeviceBufferRef);
    }
#endif

    if (!m_hwDeviceBufferRef && av_hwdevice_ctx_create(&m_hwDeviceBufferRef, AV_HWDEVICE_TYPE_DXVA2, nullptr, nullptr, 0) != 0)
        return false;

#ifdef USE_OPENGL
    if (QMPlay2Core.renderer() == QMPlay2CoreClass::Renderer::OpenGL)
    {
        if (!dxva2OpenGL)
        {
            dxva2OpenGL = make_shared<DXVA2OpenGL>(m_hwDeviceBufferRef);
            if (!dxva2OpenGL->initVideoProcessor())
                return false;
            if (!QMPlay2Core.gpuInstance()->setHWDecContextForVideoOutput(dxva2OpenGL))
                return false;
        }
        if (!dxva2OpenGL->checkCodec(streamInfo.codec_name, m_pixFmt == AV_PIX_FMT_YUV420P10))
            return false;
    }

    if (dxva2OpenGL)
    {
        m_filter = make_shared<DeintHWPrepareFilter>();
        m_hasHWDecContext = true;
    }
#endif

    codec_ctx->hw_device_ctx = av_buffer_ref(m_hwDeviceBufferRef);
    codec_ctx->get_format = dxva2GetFormat;
    codec_ctx->thread_count = 1;
    if (!openCodec(codec))
        return false;

    m_timeBase = streamInfo.time_base;
    return true;
}
