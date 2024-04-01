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

#include <Settings.hpp>

#include "../../qmvk/PhysicalDevice.hpp"
#include "../../qmvk/Device.hpp"
#include "../../qmvk/Queue.hpp"
#include "../../qmvk/ComputePipeline.hpp"
#include "../../qmvk/CommandBuffer.hpp"
#include "../../qmvk/ShaderModule.hpp"
#include "../../qmvk/Sampler.hpp"
#include "../../qmvk/Image.hpp"

#include "VulkanYadifDeint.hpp"
#include "VulkanInstance.hpp"
#include "VulkanImagePool.hpp"
#include "VulkanHWInterop.hpp"

#include <QDebug>

namespace QmVk {

struct alignas(16) YadifPushConstants
{
    int parity;
    int filterParity;
};

YadifDeint::YadifDeint(const shared_ptr<HWInterop> &hwInterop)
    : VideoFilter(true)
    , m_spatialCheck(QMPlay2Core.getSettings().getBool("Vulkan/YadifSpatialCheck"))
    , m_instance(m_vkImagePool->instance())
{
    m_supportedPixelFormats += {
            AV_PIX_FMT_NV12,
            AV_PIX_FMT_P010,
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 2, 100)
            AV_PIX_FMT_P012,
#endif
            AV_PIX_FMT_P016,
            AV_PIX_FMT_NV16,
            AV_PIX_FMT_NV20,
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(56, 31, 100)
            AV_PIX_FMT_NV24,
#endif
    };
    if (m_instance->hasStorage16bit() && m_instance->supportedPixelFormats().contains(AV_PIX_FMT_YUV420P10))
    {
        m_supportedPixelFormats += {
            AV_PIX_FMT_YUV420P9,
            AV_PIX_FMT_YUV420P10,
            AV_PIX_FMT_YUV420P12,
            AV_PIX_FMT_YUV420P14,
            AV_PIX_FMT_YUV420P16,

            AV_PIX_FMT_YUV422P9,
            AV_PIX_FMT_YUV422P10,
            AV_PIX_FMT_YUV422P12,
            AV_PIX_FMT_YUV422P14,
            AV_PIX_FMT_YUV422P16,

            AV_PIX_FMT_YUV444P9,
            AV_PIX_FMT_YUV444P10,
            AV_PIX_FMT_YUV444P12,
            AV_PIX_FMT_YUV444P14,
            AV_PIX_FMT_YUV444P16,
        };
    }

    m_vkHwInterop = hwInterop;

    addParam("DeinterlaceFlags");
    addParam("W");
    addParam("H");
}
YadifDeint::~YadifDeint()
{
}

bool YadifDeint::filter(QQueue<Frame> &framesQueue)
{
    if (m_error)
        return false;

    addFramesToDeinterlace(framesQueue);

    if (m_internalQueue.count() >= 3) try
    {
        Q_ASSERT(!m_secondFrame);

        if (!ensureResources())
        {
            clearBuffer();
            return false;
        }

        Frame &prevFrame = m_internalQueue[0];
        Frame &currFrame = m_internalQueue[1];
        Frame &nextFrame = m_internalQueue[2];

        CopyImageLinearToOptimalFn copyFns[3];

        auto prevImage = vulkanImageFromFrame(prevFrame, m.device, &copyFns[0]);
        auto currImage = vulkanImageFromFrame(currFrame, m.device, &copyFns[1]);
        auto nextImage = vulkanImageFromFrame(nextFrame, m.device, &copyFns[2]);
        if (!prevImage || !currImage || !nextImage)
        {
            clearBuffer();
            return false;
        }

        vector<Frame> syncFrames;
        if (m_vkHwInterop)
            syncFrames = {prevFrame, currFrame, nextFrame};

        vk::SubmitInfo submitInfo;
        HWInterop::SyncDataPtr syncData;

        for (int f = 0; f < 2; ++f)
        {
            auto destFrame = m_vkImagePool->takeOptimalToFrame(
                currFrame,
                Frame::convert2PlaneTo3Plane(currFrame.pixelFormat())
            );
            if (destFrame.isEmpty())
            {
                clearBuffer();
                return false;
            }
            destFrame.setNoInterlaced();

            auto destImage = vulkanImageFromFrame(destFrame);

            const uint32_t srcNumPlanes = currImage->numPlanes();
            const bool tff = isTopFieldFirst(currFrame);

            if (m_vkHwInterop && f == 0)
                syncData = m_vkHwInterop->sync(syncFrames, &submitInfo);

            if (f == 0)
            {
                m.commandBuffer->resetAndBegin();
                for (auto &&copyFn : copyFns)
                {
                    if (copyFn)
                        copyFn(m.commandBuffer);
                }
            }

            for (uint32_t p = 0; p < 3; ++p)
            {
                auto yadifPushConstants = m.computes[p][f]->pushConstants<YadifPushConstants>();
                yadifPushConstants->parity = m_secondFrame == tff;
                yadifPushConstants->filterParity = yadifPushConstants->parity ^ int(tff);

                m.computes[p][f]->setCustomSpecializationData({
                    m_spatialCheck,
                    srcNumPlanes,
                    (srcNumPlanes == 3) ? p  : ((p != 0) ? 1u : 0u),
                    (srcNumPlanes == 3) ? 0u : ((p == 2) ? 1u : 0u),
                    p
                });
                m.computes[p][f]->setMemoryObjects({
                    {prevImage, m.sampler},
                    {currImage, m.sampler},
                    {nextImage, m.sampler},
                    {destImage, MemoryObjectDescr::Access::Write},
                });
                m.computes[p][f]->prepare();

                m.computes[p][f]->recordCommands(
                    m.commandBuffer,
                    m.computes[p][f]->groupCount(destImage->size(p))
                );
            }

            const bool lastIteration = (f == 1 || !(m_deintFlags & DoubleFramerate));

            if (lastIteration)
            {
                if (m_vkHwInterop)
                    m_vkHwInterop->updateInfo(syncFrames);
                m.commandBuffer->endSubmitAndWait(move(submitInfo));
            }

            if (m_deintFlags & DoubleFramerate)
                deinterlaceDoublerCommon(destFrame);
            else
                m_internalQueue.removeFirst();

            framesQueue.enqueue(destFrame);

            if (lastIteration)
                break;
        }
    }
    catch (const vk::SystemError &e)
    {
        if (e.code() == vk::Result::eErrorDeviceLost)
            m_instance->resetDevice(m.device);
        else
            m_error = true;
        clearBuffer();
        m = {};
        return false;
    }

    return m_internalQueue.count() >= 3;
}

bool YadifDeint::processParams(bool *paramsCorrected)
{
    Q_UNUSED(paramsCorrected)
    processParamsDeint();
    if (getParam("W").toInt() < 3 || getParam("H").toInt() < 3)
        return false;
    return true;
}

bool YadifDeint::ensureResources()
{
    auto device = m_instance->device();
    if (device && m.device == device)
        return true;

    if (m.device)
        m = {};

    m.device = move(device);
    if (!m.device)
        return false;

    try
    {
        m.sampler = Sampler::create(m.device);

        auto yadifShaderModuele = ShaderModule::create(
            m.device,
            vk::ShaderStageFlagBits::eCompute,
            Instance::readShader("yadif.comp")
        );

        const auto vendorID = m.device->physicalDevice()->properties().vendorID;
        const int nf = (m_deintFlags & DoubleFramerate) ? 2 : 1;

        for (auto &&computeF : m.computes)
        {
            for (int f = 0; f < nf; ++f)
            {
                auto &&compute = computeF[f];

                compute = ComputePipeline::create(
                    m.device,
                    yadifShaderModuele,
                    sizeof(YadifPushConstants)
                );

                switch (vendorID)
                {
                    case 0x8086: // Intel
                        if (!compute->setLocalWorkgroupSize(vk::Extent2D(16, 32)))
                            compute->setLocalWorkgroupSize(vk::Extent2D(16, 16));
                        break;
                    case 0x1002: // AMD
                        compute->setLocalWorkgroupSize(vk::Extent2D(64, 8));
                        break;
                    case 0x10de: // NVIDIA
                        compute->setLocalWorkgroupSize(vk::Extent2D(128, 8));
                        break;
                }
            }
        }

        m.commandBuffer = CommandBuffer::create(getVulkanComputeQueue(m.device));
        if (m.commandBuffer->queue()->queueFamilyIndex() != m.device->queueFamilyIndex(0))
            m.filtersOnOtherQueueFamiliy = m_instance->setFiltersOnOtherQueueFamiliy();
    }
    catch (const vk::SystemError &e)
    {
        if (e.code() == vk::Result::eErrorDeviceLost)
            m_instance->resetDevice(m.device);
        else
            m_error = true;
        m = {};
        return false;
    }

    return true;
}

}
