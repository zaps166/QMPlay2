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

#pragma once

#include <QMPlay2Lib.hpp>

#include <vulkan/vulkan.hpp>

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
