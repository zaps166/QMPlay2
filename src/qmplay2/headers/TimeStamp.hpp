/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2018  Błażej Szczygieł

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

#include <qnumeric.h>

class TimeStamp
{
public:
    inline bool isValid() const
    {
        return hasDts() || hasPts();
    }
    inline bool hasDts() const
    {
        return !qIsNaN(m_dts);
    }
    inline bool hasPts() const
    {
        return !qIsNaN(m_pts);
    }

    inline void setDts(double dts, double start_time = 0.0)
    {
        m_dts = dts - start_time;
    }
    inline void setPts(double pts, double start_time = 0.0)
    {
        m_pts = pts - start_time;
    }
    inline void setInvalid()
    {
        m_dts = m_pts = qQNaN();
    }

    inline double operator =(double t)
    {
        return (m_pts = m_dts = t);
    }
    inline void operator +=(double t)
    {
        if (hasDts())
            m_dts += t;
        if (hasPts())
            m_pts += t;
    }
    inline operator double() const
    {
        // XXX: Should it always return "m_dts"?
        if (m_dts >= 0.0)
            return m_dts;
        if (m_pts >= 0.0)
            return m_pts;
        return 0.0;
    }

    inline double pts() const
    {
        return m_pts;
    }
    inline double dts() const
    {
        return m_dts;
    }

private:
    double m_pts, m_dts;
};
