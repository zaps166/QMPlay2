/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2020  Błażej Szczygieł

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

#include <vector>
#include <mutex>

class QMPLAY2SHAREDLIB_EXPORT QMPlay2OSD
{
public:
    using Image = std::pair<QRect, QByteArray>;

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

    inline const Image &getImage(int idx) const
    {
        return m_images[idx];
    }
    inline int imageCount() const
    {
        return m_images.size();
    }
    inline void addImage(const QRect &rect, const QByteArray &data)
    {
        m_images.emplace_back(rect, data);
    }

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
};
