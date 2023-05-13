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

#include <FFDecVAAPI.hpp>
#include <DeintHWPrepareFilter.hpp>
#include <FFCommon.hpp>
#include <StreamInfo.hpp>
#include <GPUInstance.hpp>
#ifdef USE_OPENGL
#   include <VAAPIOpenGL.hpp>
#endif
#ifdef USE_VULKAN
#   include <vulkan/VulkanInstance.hpp>
#   include <VAAPIVulkan.hpp>
#endif

#include <QDebug>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/pixdesc.h>
    #include <libavutil/hwcontext.h>
    #include <libavutil/hwcontext_vaapi.h>
}

using namespace std;

static AVPixelFormat vaapiGetFormat(AVCodecContext *codecCtx, const AVPixelFormat *pixFmt)
{
    Q_UNUSED(codecCtx)
    while (*pixFmt != AV_PIX_FMT_NONE)
    {
        if (*pixFmt == AV_PIX_FMT_VAAPI)
            return *pixFmt;
        ++pixFmt;
    }
    return AV_PIX_FMT_NONE;
}

/**/

FFDecVAAPI::FFDecVAAPI(Module &module)
{
    SetModule(module);
}
FFDecVAAPI::~FFDecVAAPI()
{
    if (m_vaapi.use_count() == 1)
        destroyDecoder();
}

bool FFDecVAAPI::set()
{
    switch (sets().getInt("VAAPIDeintMethod"))
    {
        case 0:
            m_vppDeintType = VAProcDeinterlacingBob;
            break;
        case 2:
            m_vppDeintType = VAProcDeinterlacingMotionCompensated;
            break;
        default:
            m_vppDeintType = VAProcDeinterlacingMotionAdaptive;
    }
    if (m_vaapi)
    {
        const bool reloadVpp = m_vaapi->ok && m_vaapi->use_vpp && (m_vaapi->vpp_deint_type != m_vppDeintType);
        m_vaapi->vpp_deint_type = m_vppDeintType;
        if (reloadVpp)
        {
            m_vaapi->clearVPP(false);
#ifdef USE_VULKAN
            if (m_vaapiVulkan)
                m_vaapiVulkan->clear();
#endif
        }
    }
    return sets().getBool("DecoderVAAPIEnabled");
}

QString FFDecVAAPI::name() const
{
    return "FFmpeg/" VAAPIWriterName;
}

shared_ptr<VideoFilter> FFDecVAAPI::hwAccelFilter() const
{
    return m_filter;
}

int FFDecVAAPI::decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurryUp)
{
#ifdef USE_VULKAN
    if (flush && m_vaapiVulkan)
        m_vaapiVulkan->clear();
#endif
    int ret = FFDecHWAccel::decodeVideo(encodedPacket, decoded, newPixFmt, flush, hurryUp);
    if (m_hasHWDecContext && ret > -1)
    {
        decoded.setOnDestroyFn([vaapi = m_vaapi] {
        });
        m_vaapi->maybeInitVPP(codec_ctx->coded_width, codec_ctx->coded_height);
#ifdef USE_VULKAN
        if (m_vaapiVulkan)
            m_vaapiVulkan->insertAvailableSurface(decoded.hwData());
#endif
    }
    return ret;
}

bool FFDecVAAPI::open(StreamInfo &streamInfo)
{
    if (streamInfo.params->codec_type != AVMEDIA_TYPE_VIDEO)
        return false;

    const AVPixelFormat pix_fmt = streamInfo.pixelFormat();
    if (pix_fmt == AV_PIX_FMT_YUV420P10 && streamInfo.params->codec_id != AV_CODEC_ID_H264)
    {
        if (QMPlay2Core.isVulkanRenderer())
        {
#ifdef USE_VULKAN
            auto vkInstance = static_pointer_cast<QmVk::Instance>(QMPlay2Core.gpuInstance());
            if (!vkInstance->supportedPixelFormats().contains(pix_fmt))
                return false;
#endif
        }
    }
    else if (pix_fmt != AV_PIX_FMT_YUV420P && pix_fmt != AV_PIX_FMT_YUVJ420P)
    {
        return false;
    }

    AVCodec *codec = init(streamInfo);
    if (!codec || !hasHWAccel("vaapi"))
        return false;

    auto isMesaRadeon = [](const QString &vendor) {
        return vendor.contains("Mesa Gallium") && vendor.contains("AMD Radeon");
    };

#ifdef USE_OPENGL
    shared_ptr<VAAPIOpenGL> vaapiOpenGL;
    if (QMPlay2Core.renderer() == QMPlay2CoreClass::Renderer::OpenGL)
    {
        vaapiOpenGL = QMPlay2Core.gpuInstance()->getHWDecContext<VAAPIOpenGL>();
        if (vaapiOpenGL)
        {
            m_vaapi = vaapiOpenGL->getVAAPI();
            if (m_vaapi && isMesaRadeon(m_vaapi->m_vendor))
                m_vaapi.reset();
        }
    }
#endif
#ifdef USE_VULKAN
    if (QMPlay2Core.isVulkanRenderer())
    {
        m_vaapiVulkan = QMPlay2Core.gpuInstance()->getHWDecContext<VAAPIVulkan>();
        if (m_vaapiVulkan)
        {
            m_vaapi = m_vaapiVulkan->getVAAPI();
            m_vaapiVulkan->clear();
            if (m_vaapi && isMesaRadeon(m_vaapi->m_vendor))
                m_vaapi.reset();
        }
    }
#endif

    if (!m_vaapi)
    {
        m_vaapi = make_shared<VAAPI>();
        if (!m_vaapi->open())
            return false;

        m_vaapi->m_hwDeviceBufferRef = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_VAAPI);
        if (!m_vaapi->m_hwDeviceBufferRef)
            return false;

        auto vaapiDevCtx = (AVVAAPIDeviceContext *)((AVHWDeviceContext *)m_vaapi->m_hwDeviceBufferRef->data)->hwctx;
        vaapiDevCtx->display = m_vaapi->VADisp;
        if (av_hwdevice_ctx_init(m_vaapi->m_hwDeviceBufferRef) != 0)
            return false;
    }

    if (!m_vaapi->checkCodec(avcodec_get_name(codec_ctx->codec_id)))
        return false;

#ifdef USE_OPENGL
    if (QMPlay2Core.renderer() == QMPlay2CoreClass::Renderer::OpenGL)
    {
        if (!vaapiOpenGL)
        {
            vaapiOpenGL = make_shared<VAAPIOpenGL>();
            if (!QMPlay2Core.gpuInstance()->setHWDecContextForVideoOutput(vaapiOpenGL))
                return false;
        }
        vaapiOpenGL->setVAAPI(m_vaapi);
        m_vaapi->vpp_deint_type = m_vppDeintType;
    }
    if (vaapiOpenGL)
    {
        m_filter = make_shared<DeintHWPrepareFilter>();
        m_hasHWDecContext = true;
    }
#endif
#ifdef USE_VULKAN
    if (QMPlay2Core.isVulkanRenderer())
    {
        if (!m_vaapiVulkan)
        {
            m_vaapiVulkan = make_shared<VAAPIVulkan>();
            if (!QMPlay2Core.gpuInstance()->setHWDecContextForVideoOutput(m_vaapiVulkan))
                return false;
        }
        m_vaapiVulkan->setVAAPI(m_vaapi);
        m_vaapi->vpp_deint_type = m_vppDeintType;
    }
    if (m_vaapiVulkan)
    {
        const bool vulkanFiltersSupported = QMPlay2Core.gpuInstance()->checkFiltersSupported();
        if (!vulkanFiltersSupported || (!QMPlay2Core.getSettings().getBool("Vulkan/ForceVulkanYadif") && m_vaapi->m_vendor.contains("intel", Qt::CaseInsensitive)))
        {
            // Use QMPlay2 Vulkan deinterlacing to workaround an Intel Media Driver bug
            // https://github.com/intel/media-driver/issues/804
            if (!vulkanFiltersSupported || !m_vaapi->m_vendor.contains("Intel iHD"))
                m_filter = make_shared<DeintHWPrepareFilter>();
        }
        m_hasHWDecContext = true;
    }
#endif

    m_vaapi->init(codec_ctx->width, codec_ctx->height, static_cast<bool>(m_filter));

    codec_ctx->hw_device_ctx = av_buffer_ref(m_vaapi->m_hwDeviceBufferRef);
    codec_ctx->get_format = vaapiGetFormat;
    codec_ctx->thread_count = 1;
    codec_ctx->extra_hw_frames = 4;
    if (!openCodec(codec))
        return false;

    m_timeBase = streamInfo.time_base;
    return true;
}
