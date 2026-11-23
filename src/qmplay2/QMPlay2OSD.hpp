// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

#ifdef USE_VULKAN
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
        QRectF rect;
        QSize size;

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
    static std::unique_lock<std::mutex> ensure(std::shared_ptr<QMPlay2OSD> &osd);

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

    inline void setScale(double scale)
    {
        m_scale = scale;
    }
    QRectF getRect(const Image &image) const;

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
    double m_scale;
    bool m_needsRescale;
    bool m_started;
    quint64 m_id;
    QElapsedTimer m_timer;
    mutable std::mutex m_mutex;
#ifdef USE_VULKAN
    std::function<void()> m_returnVkBufferFn;
#endif
};

using QMPlay2OSDList = QVector<std::shared_ptr<const QMPlay2OSD>>;
