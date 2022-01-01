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

#include <QMPlay2OSD.hpp>

#if USE_VULKAN
#   include <vulkan/VulkanBufferPool.hpp>
#endif

#include <atomic>

using namespace std;

static atomic<uint64_t> g_id;

QMPlay2OSD::QMPlay2OSD()
{
    clear();
}
QMPlay2OSD::~QMPlay2OSD()
{
    clear();
}

double QMPlay2OSD::leftDuration()
{
    if (!m_started || m_pts != -1.0)
        return 0.0;

    return m_duration - m_timer.elapsed() / 1000.0;
}

void QMPlay2OSD::start()
{
    m_started = true;
    if (m_pts == -1.0)
        m_timer.start();
}

void QMPlay2OSD::iterate(const IterateCallback &fn) const
{
    for (auto &&image : m_images)
        fn(image);
}

void QMPlay2OSD::genId()
{
    m_id = ++g_id;
}

void QMPlay2OSD::clear()
{
    m_images.clear();
    m_text.clear();
    m_duration = m_pts = -1.0;
    m_needsRescale = m_started = false;
    m_timer.invalidate();
    m_id = 0;
#ifdef USE_VULKAN
    if (m_returnVkBufferFn)
    {
        m_returnVkBufferFn();
        m_returnVkBufferFn = nullptr;
    }
#endif
}

#ifdef USE_VULKAN
void QMPlay2OSD::setReturnVkBufferFn(
    const std::weak_ptr<QmVk::BufferPool> &bufferPoolWeak,
    std::shared_ptr<QmVk::Buffer> &&buffer)
{
    m_returnVkBufferFn = [=]() mutable {
        if (auto vkBufferPool = bufferPoolWeak.lock())
            vkBufferPool->put(move(buffer));
        else
            buffer.reset();
    };
}
#endif
