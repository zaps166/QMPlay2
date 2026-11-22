// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
        auto buffer = std::move(m_buffers[idx]);
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
    m_buffers.push_back(std::move(buffer));
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
