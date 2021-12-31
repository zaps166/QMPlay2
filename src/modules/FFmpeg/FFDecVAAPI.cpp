/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2021  Błażej Szczygieł

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
    #include <libswscale/swscale.h>
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
    if (m_swsCtx)
        sws_freeContext(m_swsCtx);
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
void FFDecVAAPI::downloadVideoFrame(Frame &decoded)
{
    VAImage image;
    quint8 *vaData = m_vaapi->getNV12Image(image, (quintptr)frame->data[3]);
    if (vaData)
    {
        AVBufferRef *dstBuffer[3] = {
            av_buffer_alloc(image.pitches[0] * frame->height),
            av_buffer_alloc((image.pitches[1] / 2) * ((frame->height + 1) / 2)),
            av_buffer_alloc((image.pitches[1] / 2) * ((frame->height + 1) / 2))
        };

        quint8 *srcData[2] = {
            vaData + image.offsets[0],
            vaData + image.offsets[1]
        };
        qint32 srcLinesize[2] = {
            (qint32)image.pitches[0],
            (qint32)image.pitches[1]
        };

        uint8_t *dstData[3] = {
            dstBuffer[0]->data,
            dstBuffer[1]->data,
            dstBuffer[2]->data
        };
        qint32 dstLinesize[3] = {
            (qint32)image.pitches[0],
            (qint32)image.pitches[1] / 2,
            (qint32)image.pitches[1] / 2
        };

        m_swsCtx = sws_getCachedContext(m_swsCtx, frame->width, frame->height, AV_PIX_FMT_NV12, frame->width, frame->height, AV_PIX_FMT_YUV420P, SWS_POINT, nullptr, nullptr, nullptr);
        sws_scale(m_swsCtx, srcData, srcLinesize, 0, frame->height, dstData, dstLinesize);

        decoded = Frame::createEmpty(frame, false, AV_PIX_FMT_YUV420P);
        decoded.setVideoData(dstBuffer, dstLinesize);

        vaUnmapBuffer(m_vaapi->VADisp, image.buf);
        vaDestroyImage(m_vaapi->VADisp, image.image_id);
    }
}

bool FFDecVAAPI::open(StreamInfo &streamInfo)
{
    if (streamInfo.codec_type != AVMEDIA_TYPE_VIDEO)
        return false;

    const AVPixelFormat pix_fmt = streamInfo.pixelFormat();
    if (pix_fmt == AV_PIX_FMT_YUV420P10 && QMPlay2Core.isVulkanRenderer())
    {
#ifdef USE_VULKAN
        auto vkInstance = static_pointer_cast<QmVk::Instance>(QMPlay2Core.gpuInstance());
        if (!vkInstance->supportedPixelFormats().contains(pix_fmt))
            return false;
#endif
    }
    else if (pix_fmt != AV_PIX_FMT_YUV420P && pix_fmt != AV_PIX_FMT_YUVJ420P)
    {
        return false;
    }

    AVCodec *codec = init(streamInfo);
    if (!codec || !hasHWAccel("vaapi"))
        return false;

#ifdef USE_OPENGL
    shared_ptr<VAAPIOpenGL> vaapiOpenGL;
    if (QMPlay2Core.renderer() == QMPlay2CoreClass::Renderer::OpenGL)
    {
        vaapiOpenGL = QMPlay2Core.gpuInstance()->getHWDecContext<VAAPIOpenGL>();
        if (vaapiOpenGL)
            m_vaapi = vaapiOpenGL->getVAAPI();
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
        }
    }
#endif

    if (!m_vaapi)
    {
        m_vaapi = make_shared<VAAPI>();
        if (!m_vaapi->open(QMPlay2Core.renderer() == QMPlay2CoreClass::Renderer::Legacy))
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
    if (QMPlay2Core.renderer() == QMPlay2CoreClass::Renderer::OpenGL && !vaapiOpenGL)
    {
        vaapiOpenGL = make_shared<VAAPIOpenGL>(m_vaapi);
        if (!QMPlay2Core.gpuInstance()->setHWDecContextForVideoOutput(vaapiOpenGL))
            return false;
        m_vaapi->vpp_deint_type = m_vppDeintType;
    }
    if (vaapiOpenGL)
    {
        m_filter = make_shared<DeintHWPrepareFilter>();
        m_hasHWDecContext = true;
    }
#endif
#ifdef USE_VULKAN
    if (QMPlay2Core.isVulkanRenderer() && !m_vaapiVulkan)
    {
        m_vaapiVulkan = make_shared<VAAPIVulkan>(m_vaapi);
        if (!QMPlay2Core.gpuInstance()->setHWDecContextForVideoOutput(m_vaapiVulkan))
            return false;
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
