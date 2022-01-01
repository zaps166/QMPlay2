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

#include <QElapsedTimer>
#include <QByteArray>
#include <QRect>

#include <functional>
#include <vector>
#include <mutex>

#ifdef USE_VULKAN
#   include <QVector4D>

#   include <memory>

namespace QmVk {

class Buffer;
class BufferView;
class BufferPool;

}
#endif

class QMPLAY2SHAREDLIB_EXPORT QMPlay2OSD
{
public:
    struct Image
    {
        // Common
        QRect rect;

        // CPU only
        QByteArray rgba;

#ifdef USE_VULKAN // Vulkan only
        std::shared_ptr<QmVk::BufferView> dataBufferView;
        int linesize;

        // AV subtitles
        std::shared_ptr<QmVk::BufferView> paletteBufferView;

        // ASS subtitles
        QVector4D color;
#endif
    };

    using IterateCallback = std::function<void(const Image &)>;

public:
    QMPlay2OSD();
    ~QMPlay2OSD();

    // OSD only

    inline void setText(const QByteArray &txt)
    {
        m_text = txt;
    }
    inline QByteArray text() const
    {
        return m_text;
    }

    double leftDuration(); // If < 0 then time out and this class must be deleted

    // Subtitles only

    inline void setDuration(double d)
    {
        m_duration = d;
    }
    inline double duration() const
    {
        return m_duration;
    }

    inline void setPTS(double p)
    {
        m_pts = p;
    }
    inline double pts() const
    {
        return m_pts;
    }

    inline void setNeedsRescale()
    {
        m_needsRescale = true;
    }
    inline bool needsRescale() const
    {
        return m_needsRescale;
    }

    // Common

    // For OSD: start counting of "leftDuration()"
    // For subtitles: mark subtitles that are displayed
    void start();
    inline bool isStarted() const
    {
        return m_started;
    }

    inline Image &add()
    {
        m_images.emplace_back();
        return m_images.back();
    }
    void iterate(const IterateCallback &fn) const;

    inline auto lock() const
    {
        return std::unique_lock<std::mutex>(m_mutex);
    }

    void genId();
    inline quint64 id() const
    {
        return m_id;
    }

    void clear();

#ifdef USE_VULKAN // Vulkan only
    void setReturnVkBufferFn(
        const std::weak_ptr<QmVk::BufferPool> &bufferPoolWeak,
        std::shared_ptr<QmVk::Buffer> &&buffer
    );
#endif

private:
    std::vector<Image> m_images;
    QByteArray m_text;
    double m_duration;
    double m_pts;
    bool m_needsRescale;
    bool m_started;
    quint64 m_id;
    QElapsedTimer m_timer;
    mutable std::mutex m_mutex;
#ifdef USE_VULKAN
    std::function<void()> m_returnVkBufferFn;
#endif
};
