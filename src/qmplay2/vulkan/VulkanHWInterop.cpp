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

#include "../../qmvk/CommandBuffer.hpp"
#include "../../qmvk/Device.hpp"
#include "../../qmvk/Queue.hpp"

#include <vulkan/VulkanInstance.hpp>
#include <vulkan/VulkanHWInterop.hpp>

namespace QmVk {

bool HWInterop::syncNow(vk::SubmitInfo &submitInfo)
{
    if (!m_commandBuffer)
    {
        auto device = static_pointer_cast<Instance>(QMPlay2Core.gpuInstance())->device();
        if (!device)
            return false;

        m_commandBuffer = CommandBuffer::create(device->queue());
    }

    m_commandBuffer->resetAndBegin();
    m_commandBuffer->endSubmitAndWait(move(submitInfo));
    return true;
}

}
