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

#include <Settings.hpp>

#include "../../qmvk/PhysicalDevice.hpp"
#include "../../qmvk/Device.hpp"
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
            AV_PIX_FMT_P016,
            AV_PIX_FMT_NV16,
            AV_PIX_FMT_NV20,
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(56, 31, 100)
            AV_PIX_FMT_NV24,
#endif
    };

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

        vk::SubmitInfo submitInfo;
        HWInterop::SyncDataPtr syncData;
        if (m_vkHwInterop)
            syncData = m_vkHwInterop->sync({prevFrame, currFrame, nextFrame}, &submitInfo);

        m.commandBuffer->resetAndBegin();
        for (auto &&copyFn : copyFns)
        {
            if (copyFn)
                copyFn(m.commandBuffer);
        }
        for (uint32_t p = 0; p < 3; ++p)
        {
            auto yadifPushConstants = m.computes[p]->pushConstants<YadifPushConstants>();
            yadifPushConstants->parity = m_secondFrame == tff;
            yadifPushConstants->filterParity = yadifPushConstants->parity ^ int(tff);

            m.computes[p]->setCustomSpecializationData({
                m_spatialCheck,
                srcNumPlanes,
                (srcNumPlanes == 3) ? p  : ((p != 0) ? 1u : 0u),
                (srcNumPlanes == 3) ? 0u : ((p == 2) ? 1u : 0u),
                p
            });
            m.computes[p]->setMemoryObjects({
                {prevImage, m.sampler},
                {currImage, m.sampler},
                {nextImage, m.sampler},
                {destImage, MemoryObjectDescr::Access::Write},
            });
            m.computes[p]->prepare();

            m.computes[p]->recordCommands(
                m.commandBuffer,
                m.computes[p]->groupCount(destImage->size(p))
            );
        }
        m.commandBuffer->endSubmitAndWait(move(submitInfo));

        if (m_deintFlags & DoubleFramerate)
            deinterlaceDoublerCommon(destFrame);
        else
            m_internalQueue.removeFirst();

        framesQueue.enqueue(destFrame);
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

        for (auto &&compute : m.computes)
        {
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

        m.commandBuffer = CommandBuffer::create(m.device->queue(min(1u, m.device->numQueues() - 1u)));
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
