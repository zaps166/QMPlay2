/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2025  Błażej Szczygieł

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

#include <FFDecVkVideo.hpp>
#include <VkVideoVulkan.hpp>
#include <FFCommon.hpp>
#include <StreamInfo.hpp>
#include <GPUInstance.hpp>

#include "../../qmvk/PhysicalDevice.hpp"
#include "../../qmvk/Device.hpp"
#include "../../qmvk/Queue.hpp"
#include "../../qmvk/CommandBuffer.hpp"

#include <vulkan/VulkanInstance.hpp>

#include <QVulkanInstance>
#include <QThread>
#include <QDebug>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/pixdesc.h>
    #include <libavutil/hwcontext.h>
    #include <libavutil/hwcontext_vulkan.h>
}

using namespace std;

static inline uint64_t makeU32Pair(uint32_t hi, uint32_t lo)
{
    uint64_t val = hi;
    val <<= 32;
    val |= lo;
    return val;
}

static AVPixelFormat vkGetFormat(AVCodecContext *codecCtx, const AVPixelFormat *pixFmt)
{
    while (*pixFmt != AV_PIX_FMT_NONE)
    {
        if (*pixFmt == AV_PIX_FMT_VULKAN)
            return *pixFmt;
        ++pixFmt;
    }
    return AV_PIX_FMT_NONE;
}

FFDecVkVideo::FFDecVkVideo(Module &module, bool hwDownload)
    : m_hwDownload(hwDownload)
{
    SetModule(module);
}
FFDecVkVideo::~FFDecVkVideo()
{
    destroyHw();
    destroyDecoder(); // Decoder must be destroyed before closing the device
}

bool FFDecVkVideo::set()
{
    return sets().getBool("DecoderVkVideoEnabled");
}

QString FFDecVkVideo::name() const
{
    return "FFmpeg/" VkVideoWriterName;
}

int FFDecVkVideo::decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurryUp)
{
    if (m_hwDownload)
    {
        return FFDecHWAccel::decodeVideo(encodedPacket, decoded, newPixFmt, flush, hurryUp);
    }

    if (flush)
        m_vkVideoVulkan->clear();
    int ret = FFDecHWAccel::decodeVideo(encodedPacket, decoded, newPixFmt, flush, hurryUp);
    if (ret > -1)
        m_vkVideoVulkan->insertAvailableAvVkFrames(decoded.hwData(0), codec_ctx->coded_height);
    if (m_hasCriticalError && m_libError)
    {
        try
        {
            // Test whether we have device lost
            auto cmdBuf = QmVk::CommandBuffer::create(m_device->firstQueue());
            cmdBuf->resetAndBegin();
            cmdBuf->endSubmitAndWait();
        }
        catch (const vk::DeviceLostError &e)
        {
            // Try to recover from device lost error

            Q_UNUSED(e)

            if (m_recovering)
            {
                qDebug() << "VkVideo :: Another device lost, ignoring";
                return ret;
            }

            m_recovering = true;

            auto paramsBackup = avcodec_parameters_alloc();
            avcodec_parameters_from_context(paramsBackup, codec_ctx);

            destroyHw();
            destroyDecoder();
            m_vkInstance->resetDevice(m_device);
            m_device.reset();

            QThread::msleep(1000);

            codec_ctx = avcodec_alloc_context3(m_codec);
            if (codec_ctx)
                avcodec_parameters_to_context(codec_ctx, paramsBackup);

            avcodec_parameters_free(&paramsBackup);

            if (codec_ctx && initHw())
            {
                m_hasCriticalError = false;
                m_libError = false;
                qDebug() << "VkVideo :: Recovered from device lost error";
                ret = decodeVideo(encodedPacket, decoded, newPixFmt, flush, hurryUp);
            }
            else
            {
                qDebug() << "VkVideo :: Unable to recover from device lost error";
            }

            m_recovering = false;
        }
        catch (const vk::SystemError &e)
        {
            Q_UNUSED(e)
        }
    }
    return ret;
}

bool FFDecVkVideo::open(StreamInfo &streamInfo)
{
    if (streamInfo.params->codec_type != AVMEDIA_TYPE_VIDEO || !hasHWAccel("vulkan"))
        return false;

    if (!m_hwDownload)
    {
        m_vkInstance = static_pointer_cast<QmVk::Instance>(QMPlay2Core.gpuInstance());
    }

    const AVPixelFormat pixFmt = streamInfo.pixelFormat();
    if (pixFmt == AV_PIX_FMT_YUV420P10 && streamInfo.params->codec_id != AV_CODEC_ID_H264)
    {
        if (!m_hwDownload)
        {
            if (!m_vkInstance->supportedPixelFormats().contains(pixFmt))
                return false;
        }
    }
    else if (pixFmt != AV_PIX_FMT_YUV420P && pixFmt != AV_PIX_FMT_YUVJ420P)
    {
        return false;
    }

    if (!m_hwDownload)
    {
        m_physicalDevice = m_vkInstance->physicalDevice();
        Q_ASSERT(m_physicalDevice);

        if (!m_physicalDevice->checkExtensions({
            VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
            VK_KHR_VIDEO_QUEUE_EXTENSION_NAME,
            VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME,
        })) {
            return false;
        }

        auto getVideoCodecOperations = [&]()->vk::VideoCodecOperationFlagsKHR {
            using ChainType = vk::StructureChain<vk::QueueFamilyProperties2, vk::QueueFamilyVideoPropertiesKHR>;
            const auto queueFamilyProps2 = m_physicalDevice->getQueueFamilyProperties2<
                ChainType,
                allocator<ChainType>
            >(m_vkInstance->dld());

            const auto decodeQueueFamily = m_physicalDevice->getQueuesFamily(vk::QueueFlagBits::eVideoDecodeKHR, false, true);
            if (decodeQueueFamily.empty())
                return vk::VideoCodecOperationFlagBitsKHR::eNone;

            const auto decodeQueueFamilyIndex = decodeQueueFamily[0].first;
            if (decodeQueueFamilyIndex >= queueFamilyProps2.size())
                return vk::VideoCodecOperationFlagBitsKHR::eNone;
            return queueFamilyProps2[decodeQueueFamilyIndex].get<vk::QueueFamilyVideoPropertiesKHR>().videoCodecOperations;
        };
        switch (streamInfo.params->codec_id)
        {
            case AV_CODEC_ID_H264:
                if (!m_physicalDevice->checkExtension(VK_KHR_VIDEO_DECODE_H264_EXTENSION_NAME))
                    return false;
                if (!(getVideoCodecOperations() & vk::VideoCodecOperationFlagBitsKHR::eDecodeH264))
                    return false;
                break;
            case AV_CODEC_ID_H265:
                if (!m_physicalDevice->checkExtension(VK_KHR_VIDEO_DECODE_H265_EXTENSION_NAME))
                    return false;
                if (!(getVideoCodecOperations() & vk::VideoCodecOperationFlagBitsKHR::eDecodeH265))
                    return false;
                break;
            case AV_CODEC_ID_VP9:
                if (avcodec_version() >= AV_VERSION_INT(62, 11, 100))
                {
                    if (!m_physicalDevice->checkExtension(VK_KHR_VIDEO_DECODE_VP9_EXTENSION_NAME))
                        return false;
                    if (!(getVideoCodecOperations() & vk::VideoCodecOperationFlagBitsKHR::eDecodeVp9))
                        return false;
                }
                else
                {
                    return false;
                }
                break;
            case AV_CODEC_ID_AV1:
                if (avcodec_version() >= AV_VERSION_INT(61, 2, 100))
                {
                    if (!m_physicalDevice->checkExtension(VK_KHR_VIDEO_DECODE_AV1_EXTENSION_NAME))
                        return false;
                    if (!(getVideoCodecOperations() & vk::VideoCodecOperationFlagBitsKHR::eDecodeAv1))
                        return false;
                }
                else
                {
                    return false;
                }
                break;
            default:
                return false;
        }
    }

    m_codec = init(streamInfo);
    if (!m_codec)
        return false;

    m_timeBase = streamInfo.time_base;

    return initHw();
}

bool FFDecVkVideo::initHw()
{
    if (!m_hwDownload)
    {
        m_hwDeviceBufferRef = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_VULKAN);
        if (!m_hwDeviceBufferRef)
            return false;

        try
        {
            m_device = m_vkInstance->createOrGetDevice(m_physicalDevice);
        }
        catch (const vk::SystemError &e)
        {
            return false;
        }

        const auto &instanceExtensions = m_vkInstance->enabledExtensions();
        const auto &deviceExtensions = m_vkInstance->device()->enabledExtensions();
        int i = 0;

        auto hwDevCtx = (AVHWDeviceContext *)m_hwDeviceBufferRef->data;
        hwDevCtx->user_opaque = this;
        hwDevCtx->free = [](AVHWDeviceContext *ctx) {
            const auto vkDevCtx = reinterpret_cast<AVVulkanDeviceContext *>(ctx->hwctx);
            delete[] vkDevCtx->enabled_dev_extensions;
            delete[] vkDevCtx->enabled_inst_extensions;
        };

        auto vkDevCtx = (AVVulkanDeviceContext *)hwDevCtx->hwctx;
        vkDevCtx->get_proc_addr = m_vkInstance->getVkGetInstanceProcAddr();
        vkDevCtx->inst = *m_vkInstance;
        vkDevCtx->phys_dev = *m_physicalDevice;
        vkDevCtx->act_dev = *m_device;
        vkDevCtx->device_features = m_vkInstance->enabledDeviceFeatures();

        vkDevCtx->nb_enabled_inst_extensions = instanceExtensions.size();
        vkDevCtx->enabled_inst_extensions = new char *[vkDevCtx->nb_enabled_inst_extensions];
        i = 0;
        for (auto &&str : instanceExtensions)
            const_cast<const char *&>(vkDevCtx->enabled_inst_extensions[i++]) = str.c_str();

        vkDevCtx->nb_enabled_dev_extensions = deviceExtensions.size();
        vkDevCtx->enabled_dev_extensions = new char *[vkDevCtx->nb_enabled_dev_extensions];
        i = 0;
        for (auto &&str : deviceExtensions)
        {
            const char *cStr = str.c_str();
    #ifdef VK_USE_PLATFORM_WIN32_KHR
            if (qstrcmp(cStr, VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME) == 0)
    #else
            if (qstrcmp(cStr, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME) == 0)
    #endif
            {
                // Skip this extension, because it's not needed
                vkDevCtx->nb_enabled_dev_extensions -= 1;
                continue;
            }
            const_cast<const char *&>(vkDevCtx->enabled_dev_extensions[i++]) = cStr;
        }

        const uint32_t numQueueFamilies = m_device->numQueueFamilies();
        for (uint32_t i = 0; i < numQueueFamilies; ++i)
        {
            const uint32_t queueFamilyIndex = m_device->queueFamilyIndex(i);
            const uint32_t numQueues = m_device->numQueues(queueFamilyIndex);
            const auto &props = m_physicalDevice->getQueueProps(queueFamilyIndex);

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(59, 39, 100)
            auto setQueue = [&](int qfIdx, vk::QueueFlagBits flag) {
                auto &qf = vkDevCtx->qf[qfIdx];
                if ((props.flags & flag) && (qf.num == 0 || (m_physicalDevice->getQueueProps(qf.idx).flags & vk::QueueFlagBits::eGraphics)))
                {
                    qf.idx = queueFamilyIndex;
                    qf.num = numQueues;
                    qf.flags = static_cast<VkQueueFlagBits>(flag);
                    vkDevCtx->nb_qf = max(qfIdx + 1, vkDevCtx->nb_qf);
                }
            };

            setQueue(0, vk::QueueFlagBits::eGraphics);
            setQueue(1, vk::QueueFlagBits::eTransfer);
            setQueue(2, vk::QueueFlagBits::eCompute);
            setQueue(3, vk::QueueFlagBits::eVideoDecodeKHR);
#else
            auto setQueue = [&](int &idx, int &nb, vk::QueueFlagBits flag) {
                if ((props.flags & flag) && (nb == 0 || (m_physicalDevice->getQueueProps(idx).flags & vk::QueueFlagBits::eGraphics)))
                {
                    idx = queueFamilyIndex;
                    nb = numQueues;
                }
            };

            setQueue(vkDevCtx->queue_family_index, vkDevCtx->nb_graphics_queues, vk::QueueFlagBits::eGraphics);
            setQueue(vkDevCtx->queue_family_tx_index, vkDevCtx->nb_tx_queues, vk::QueueFlagBits::eTransfer);
            setQueue(vkDevCtx->queue_family_comp_index, vkDevCtx->nb_comp_queues, vk::QueueFlagBits::eCompute);
            setQueue(vkDevCtx->queue_family_decode_index, vkDevCtx->nb_decode_queues, vk::QueueFlagBits::eVideoDecodeKHR);
#endif
        }

        vkDevCtx->lock_queue = [](AVHWDeviceContext *ctx, uint32_t queueFamilyIndex, uint32_t index) {
            const auto self = reinterpret_cast<FFDecVkVideo *>(ctx->user_opaque);
            self->m_queueLocks[makeU32Pair(queueFamilyIndex, index)] = self->m_device->queue(queueFamilyIndex, index)->lock();
        };
        vkDevCtx->unlock_queue = [](AVHWDeviceContext *ctx, uint32_t queueFamilyIndex, uint32_t index) {
            const auto self = reinterpret_cast<FFDecVkVideo *>(ctx->user_opaque);
            self->m_queueLocks.erase(makeU32Pair(queueFamilyIndex, index));
        };

        if (av_hwdevice_ctx_init(m_hwDeviceBufferRef) != 0)
            return false;
    }
    else
    {
        QByteArray device;
        if (QMPlay2Core.isVulkanRenderer())
        {
            const auto vkInstance = static_pointer_cast<QmVk::Instance>(QMPlay2Core.gpuInstance());
            int i = -1;
            for (auto &&physicelDeviceIt : vkInstance->enumeratePhysicalDevices(false, false))
            {
                ++i;
                if (physicelDeviceIt == vkInstance->physicalDevice())
                    break;
            }
            if (i >= 0)
            {
                device = QByteArray::number(i);
            }
        }
        if (av_hwdevice_ctx_create(&m_hwDeviceBufferRef, AV_HWDEVICE_TYPE_VULKAN, device.constData(), nullptr, 0) < 0)
        {
            return false;
        }
    }

    codec_ctx->hw_device_ctx = av_buffer_ref(m_hwDeviceBufferRef);
    if (!m_hwDownload)
        codec_ctx->get_format = vkGetFormat;
    if (!openCodec(m_codec))
        return false;

    if (!m_hwDownload)
    {
        m_vkVideoVulkan = QMPlay2Core.gpuInstance()->getHWDecContext<VkVideoVulkan>();
        if (!m_vkVideoVulkan)
        {
            m_vkVideoVulkan = make_shared<VkVideoVulkan>(m_vkInstance);
            m_vkInstance->setHWDecContextForVideoOutput(m_vkVideoVulkan);
        }

        m_hasHWDecContext = true;
    }

    return true;
}
void FFDecVkVideo::destroyHw()
{
    if (m_vkVideoVulkan)
        m_vkVideoVulkan->clear();
    av_buffer_unref(&m_hwDeviceBufferRef);
}
