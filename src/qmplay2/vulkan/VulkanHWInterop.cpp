// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <vulkan/VulkanInstance.hpp>
#include <vulkan/VulkanHWInterop.hpp>

namespace QmVk {

void HWInterop::updateInfo(const vector<Frame> &frames)
{
    Q_UNUSED(frames)
}

bool HWInterop::syncNow(vk::SubmitInfo &submitInfo)
{
    if (!m_commandBuffer)
    {
        auto device = static_pointer_cast<Instance>(QMPlay2Core.gpuInstance())->device();
        if (!device)
            return false;

        m_commandBuffer = CommandBuffer::create(device->firstQueue());
    }

    m_commandBuffer->resetAndBegin();
    m_commandBuffer->endSubmitAndWait(std::move(submitInfo));
    return true;
}

}
