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

#include <VkVideoVulkan.hpp>

#include "../qmvk/Image.hpp"

#include <vulkan/VulkanInstance.hpp>

#include <QMPlay2Core.hpp>
#include <FFCommon.hpp>
#include <Frame.hpp>

#include <QDebug>

extern "C"
{
    #include <libavutil/hwcontext_vulkan.h>
}

using namespace QmVk;

class VkVideoSyncData : public HWInterop::SyncData
{
public:
    vk::TimelineSemaphoreSubmitInfo timelineSemaphoreSubmitInfo;
    vector<vk::Semaphore> waitSemaphores;
    vector<vk::PipelineStageFlags> waitDstStageMask;
    vector<uint64_t> waitSemaphoreValues;
    vector<uint64_t> signalSemaphoreValues;
};

VkVideoVulkan::VkVideoVulkan(const shared_ptr<QmVk::Instance> &instance)
    : m_vkInstance(instance)
{
}
VkVideoVulkan::~VkVideoVulkan()
{}

QString VkVideoVulkan::name() const
{
    return VkVideoWriterName;
}

void VkVideoVulkan::map(Frame &frame)
{
    if (m_error || frame.vulkanImage())
        return;

    auto avVkFramePtr = frame.hwData(0);
    if (avVkFramePtr == Frame::s_invalidCustomData)
        return;

    auto avVkFrame = reinterpret_cast<AVVkFrame *>(avVkFramePtr);
    if (!avVkFrame)
        return;

    lock_guard<mutex> locker(m_mutex);

    const auto format = (frame.depth() > 8)
        ? vk::Format::eG16B16R162Plane420Unorm
        : vk::Format::eG8B8R82Plane420Unorm
    ;

    auto device = m_vkInstance->device();

    if (!m_images.empty())
    {
        auto image = m_images.begin()->second.get();
        if (image->device() != device || image->format() != format)
            m_images.clear();
    }

    if (!device || !frame.isHW())
        return;

    if (m_availableAvVkFrames.count(avVkFramePtr) == 0)
        return;

    auto &image = m_images[avVkFramePtr];
    if (!image)
    {
        try
        {
            image = Image::createFromImage(
                device,
                {avVkFrame->img[0]},
                vk::Extent2D(frame.width(), m_codedHeight),
                format,
                (avVkFrame->tiling == VK_IMAGE_TILING_LINEAR)
            );
        }
        catch (const vk::Error &e)
        {
            QMPlay2Core.logError(QString("VkVideo :: %1").arg(e.what()));
            m_images.erase(avVkFramePtr);
            m_error = true;
        }
    }

    if (!m_error)
    {
        image->imageLayout() = static_cast<vk::ImageLayout>(avVkFrame->layout[0]);
        image->accessFlags() = static_cast<vk::AccessFlagBits>(avVkFrame->access[0]);

        frame.setHasBorders(true);
        frame.setVulkanImage(image);
    }
}
void VkVideoVulkan::clear()
{
    lock_guard<mutex> locker(m_mutex);
    m_availableAvVkFrames.clear();
    m_images.clear();
}

HWInterop::SyncDataPtr VkVideoVulkan::sync(const vector<Frame> &frames, vk::SubmitInfo *submitInfo)
{
    unique_ptr<VkVideoSyncData> syncData;
    unique_ptr<vk::SubmitInfo> localSubmitInfo;

    auto initSyncData = [&] {
        syncData = make_unique<VkVideoSyncData>();
        if (submitInfo)
        {
            for (uint32_t i = 0; i < submitInfo->waitSemaphoreCount; ++i)
            {
                syncData->waitSemaphores.push_back(submitInfo->pWaitSemaphores[i]);
                syncData->waitDstStageMask.push_back(submitInfo->pWaitDstStageMask[i]);
                syncData->waitSemaphoreValues.push_back(0);
                syncData->signalSemaphoreValues.push_back(0);
            }
        }
        else
        {
            localSubmitInfo = make_unique<vk::SubmitInfo>();
            submitInfo = localSubmitInfo.get();
        }
    };

    for (auto &&frame : frames)
    {
        auto avVkFramePtr = frame.hwData(0);
        if (avVkFramePtr == Frame::s_invalidCustomData)
            continue;

        auto avVkFrame = reinterpret_cast<AVVkFrame *>(avVkFramePtr);
        if (!avVkFrame)
            continue;

        {
            lock_guard<mutex> locker(m_mutex);
            auto it = m_availableAvVkFrames.find(avVkFramePtr);
            if (it != m_availableAvVkFrames.end() && !it->second)
                it->second = true; // Set as synchronized
            else
                continue; // Not found or already synchronized
        }

        if (!syncData)
            initSyncData();

        syncData->waitSemaphores.push_back(avVkFrame->sem[0]);
        syncData->waitDstStageMask.push_back(vk::PipelineStageFlagBits::eAllCommands);
        syncData->waitSemaphoreValues.push_back(avVkFrame->sem_value[0]);
        syncData->signalSemaphoreValues.push_back(avVkFrame->sem_value[0] + 1);
    }

    if (!syncData)
        return nullptr;

    submitInfo->pNext = &syncData->timelineSemaphoreSubmitInfo;

    submitInfo->waitSemaphoreCount = syncData->waitSemaphores.size();
    submitInfo->pWaitSemaphores = syncData->waitSemaphores.data();
    submitInfo->pWaitDstStageMask = syncData->waitDstStageMask.data();
    syncData->timelineSemaphoreSubmitInfo.waitSemaphoreValueCount = syncData->waitSemaphoreValues.size();
    syncData->timelineSemaphoreSubmitInfo.pWaitSemaphoreValues = syncData->waitSemaphoreValues.data();
    syncData->timelineSemaphoreSubmitInfo.signalSemaphoreValueCount = syncData->signalSemaphoreValues.size();
    syncData->timelineSemaphoreSubmitInfo.pSignalSemaphoreValues = syncData->signalSemaphoreValues.data();

    if (!localSubmitInfo)
        return syncData;

    if (!syncNow(*submitInfo))
        m_error = true;

    return nullptr;
}

void VkVideoVulkan::updateInfo(const std::vector<Frame> &frames)
{
    for (auto &&frame : frames)
    {
        auto avVkFramePtr = frame.hwData(0);
        if (avVkFramePtr == Frame::s_invalidCustomData)
            continue;

        auto avVkFrame = reinterpret_cast<AVVkFrame *>(avVkFramePtr);
        if (!avVkFrame)
            continue;

        {
            lock_guard<mutex> locker(m_mutex);
            if (m_availableAvVkFrames.count(avVkFramePtr) == 0)
                continue;
        }

        avVkFrame->layout[0] = static_cast<VkImageLayout>(frame.vulkanImage()->imageLayout());
        avVkFrame->access[0] = static_cast<VkAccessFlagBits>(frame.vulkanImage()->accessFlags().operator unsigned int());
    }
}

void VkVideoVulkan::insertAvailableAvVkFrames(uintptr_t avVkFramePtr, int codedHeight)
{
    lock_guard<mutex> locker(m_mutex);
    m_availableAvVkFrames[avVkFramePtr] = false; // Set as not synchronized
    m_codedHeight = codedHeight;
}
