// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Lib.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>
#include <deque>
#include <mutex>

namespace QmVk {

using namespace std;

class Instance;
class Device;
class Buffer;

class QMPLAY2SHAREDLIB_EXPORT BufferPool : public enable_shared_from_this<BufferPool>
{
public:
    BufferPool(const shared_ptr<Instance> &instance);
    ~BufferPool();

    inline shared_ptr<Instance> instance() const;

    shared_ptr<Buffer> take(vk::DeviceSize size);
    void put(shared_ptr<Buffer> &&buffer);

private:
    void maybeClear(const shared_ptr<Device> &device);

private:
    const shared_ptr<Instance> m_instance;

    deque<shared_ptr<Buffer>> m_buffers;
    mutex m_mutex;
};

/* Inline implementation */

shared_ptr<Instance> BufferPool::instance() const
{
    return m_instance;
}

}
