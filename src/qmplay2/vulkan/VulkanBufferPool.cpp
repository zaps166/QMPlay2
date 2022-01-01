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

#include "../../qmvk/Buffer.hpp"

#include "VulkanInstance.hpp"
#include "VulkanBufferPool.hpp"

namespace QmVk {

BufferPool::BufferPool(const shared_ptr<Instance> &instance)
    : m_instance(instance)
{
}
BufferPool::~BufferPool()
{
}

shared_ptr<Buffer> BufferPool::take(vk::DeviceSize size)
{
    lock_guard<mutex> locker(m_mutex);

    auto device = m_instance->device();

    maybeClear(device);

    if (!device)
        return nullptr;

    constexpr auto noIdx = static_cast<size_t>(-1);
    auto idx = noIdx;

    for (size_t i = 0; i < m_buffers.size(); ++i)
    {
        if (m_buffers[i]->size() < size)
            continue;

        if (idx == noIdx)
        {
            idx = i;
            continue;
        }

        if (m_buffers[i]->size() < m_buffers[idx]->size())
            idx = i;
    }

    if (idx != noIdx)
    {
        auto buffer = move(m_buffers[idx]);
        m_buffers.erase(m_buffers.begin() + idx);
        return buffer;
    }

    try
    {
        auto buffer = Buffer::createUniformTexelBuffer(
            device,
            size
        );
        buffer->map();
        return buffer;
    }
    catch (const vk::SystemError &e)
    {
        Q_UNUSED(e)
    }

    return nullptr;
}
void BufferPool::put(shared_ptr<Buffer> &&buffer)
{
    lock_guard<mutex> locker(m_mutex);
    maybeClear(buffer->device());
    m_buffers.push_back(move(buffer));
}

void BufferPool::maybeClear(const shared_ptr<Device> &device)
{
    if (m_buffers.empty())
        return;

    const auto &buffer = m_buffers[0];
    if (buffer->device() != device)
        m_buffers.clear();
}

}
