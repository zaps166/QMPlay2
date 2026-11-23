// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <VideoFilter.hpp>

#ifdef USE_VULKAN
#   include <QMPlay2Core.hpp>

#   include <vulkan/VulkanInstance.hpp>
#   include <vulkan/VulkanImagePool.hpp>
#   include <vulkan/VulkanHWInterop.hpp>
#endif

VideoFilter::VideoFilter(bool fillDefaultSupportedPixelFormats)
{
#ifdef USE_VULKAN
    if (QMPlay2Core.isVulkanRenderer())
        m_vkImagePool = std::static_pointer_cast<QmVk::Instance>(QMPlay2Core.gpuInstance())->createImagePool();
#endif

    if (!fillDefaultSupportedPixelFormats)
        return;

    m_supportedPixelFormats = {
        AV_PIX_FMT_YUV420P,
        AV_PIX_FMT_YUVJ420P,
        AV_PIX_FMT_YUV422P,
        AV_PIX_FMT_YUVJ422P,
        AV_PIX_FMT_YUV444P,
        AV_PIX_FMT_YUVJ444P,
        AV_PIX_FMT_YUV410P,
        AV_PIX_FMT_YUV411P,
        AV_PIX_FMT_YUVJ411P,
        AV_PIX_FMT_YUV440P,
        AV_PIX_FMT_YUVJ440P,
    };
}
VideoFilter::~VideoFilter()
{
}

void VideoFilter::clearBuffer()
{
    m_internalQueue.clear();

    m_secondFrame = false;
    m_lastTS = qQNaN();
}

bool VideoFilter::removeLastFromInternalBuffer()
{
    if (!m_internalQueue.isEmpty())
    {
        m_internalQueue.removeLast();
        return true;
    }
    return false;
}

void VideoFilter::processParamsDeint()
{
    m_secondFrame = false;
    m_lastTS = qQNaN();

    m_deintFlags = getParam("DeinterlaceFlags").toInt();
}

void VideoFilter::addFramesToInternalQueue(QQueue<Frame> &framesQueue)
{
    while (!framesQueue.isEmpty())
    {
        const Frame &videoFrame = framesQueue.constFirst();
        if (videoFrame.isEmpty() || (!m_supportedPixelFormats.isEmpty() && !m_supportedPixelFormats.contains(videoFrame.pixelFormat())))
            break;
        m_internalQueue.enqueue(framesQueue.dequeue());
    }
}
void VideoFilter::addFramesToDeinterlace(QQueue<Frame> &framesQueue)
{
    while (!framesQueue.isEmpty())
    {
        const Frame &videoFrame = framesQueue.constFirst();
        if (videoFrame.isEmpty() || (!m_supportedPixelFormats.isEmpty() && !m_supportedPixelFormats.contains(videoFrame.pixelFormat())))
            break;
        if ((m_deintFlags & AutoDeinterlace) && !videoFrame.isInterlaced())
            break;
        m_internalQueue.enqueue(framesQueue.dequeue());
    }
}

void VideoFilter::deinterlaceDoublerCommon(Frame &frame)
{
    const double initialTS = frame.ts();

    if (m_secondFrame)
    {
        frame.setTS(getMidFrameTS(frame.ts(), m_lastTS));
        frame.setIsSecondField(true);
        m_internalQueue.removeFirst();
    }

    if (m_secondFrame || qIsNaN(m_lastTS))
        m_lastTS = initialTS;

    m_secondFrame = !m_secondFrame;
}

bool VideoFilter::isTopFieldFirst(const Frame &videoFrame) const
{
    return ((m_deintFlags & AutoParity) && videoFrame.isInterlaced())
        ? videoFrame.isTopFieldFirst()
        : (m_deintFlags & TopFieldFirst)
    ;
}

double VideoFilter::getMidFrameTS(double ts1, double ts2) const
{
    return ts1 + qAbs(ts1 - ts2) / 2.0;
}

Frame VideoFilter::getNewFrame(const Frame &other)
{
#ifdef USE_VULKAN
    if (m_vkImagePool)
    {
        auto frame = m_vkImagePool->takeToFrame(other);
        if (!frame.isEmpty())
            return frame;
    }
#endif
    return Frame::createEmpty(other, true);
}

#ifdef USE_VULKAN
std::shared_ptr<QmVk::Queue> VideoFilter::getVulkanComputeQueue(
    const std::shared_ptr<QmVk::Device> &device)
{
    const uint32_t numQueueFamilies = device->numQueueFamilies();
    uint32_t queueFamilyIndex = ~0u;
    uint32_t numQueues = 0;
    for (uint32_t i = 0; i < numQueueFamilies; ++i)
    {
        const uint32_t queueFamilyIndexIt = device->queueFamilyIndex(i);
        const uint32_t numQueuesIt = device->numQueues(queueFamilyIndexIt);
        const auto &props = device->physicalDevice()->getQueueProps(queueFamilyIndexIt);
        if ((props.flags & vk::QueueFlagBits::eCompute) && (numQueues == 0 || (numQueues == 1 && numQueuesIt > 1)))
        {
            queueFamilyIndex = queueFamilyIndexIt;
            numQueues = numQueuesIt;
        }
    }
    return device->queue(queueFamilyIndex, numQueues - 1u);
}

std::shared_ptr<QmVk::Image> VideoFilter::vulkanImageFromFrame(
    Frame &frame,
    const std::shared_ptr<QmVk::Device> &device,
    CopyImageLinearToOptimalFn *copyFn)
{
    if (m_vkHwInterop && device)
    {
        m_vkHwInterop->map(frame);
        if (m_vkHwInterop->hasError() || !frame.vulkanImage())
            return nullptr;
    }

    auto image = frame.vulkanImage();
    if (!image)
    {
        auto oldFrame = std::move(frame);
        frame = m_vkImagePool->takeToFrame(oldFrame);
        if (frame.isEmpty())
        {
            frame = std::move(oldFrame);
            return nullptr;
        }

        image = frame.vulkanImage();
        oldFrame.copyToVulkanImage(image);
    }
    else if (device && image->device() != device)
    {
        return nullptr;
    }

    if (copyFn && image->isLinear() && !image->isSampled())
    {
        auto oldFrame = std::move(frame);
        frame = m_vkImagePool->takeOptimalToFrame(oldFrame);
        if (frame.isEmpty())
        {
            frame = std::move(oldFrame);
            return nullptr;
        }

        auto oldImage = std::move(image);
        image = frame.vulkanImage();
        *copyFn = [=](const std::shared_ptr<QmVk::CommandBuffer> &commandBuffer) {
            oldImage->copyTo(image, commandBuffer);
        };
    }

    return image;
}
#endif
