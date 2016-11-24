/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

#ifndef TIMESTAMP_HPP
#define TIMESTAMP_HPP

#include <qnumeric.h>

class TimeStamp
{
public:
	inline bool isValid() const
	{
		return !qIsNaN(m_pts) && !qIsNaN(m_dts);
	}

	inline void set(double dts, double pts, double start_time = 0.0)
	{
		m_dts = dts - start_time;
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
		m_dts += t;
		m_pts += t;
	}
	inline operator double() const
	{
		return dtsPts();
	}

	inline double pts() const
	{
		return m_pts;
	}
	inline double dts() const
	{
		return m_dts;
	}

	inline double ptsDts() const
	{
		if (m_pts >= 0.0)
			return m_pts;
		if (m_dts >= 0.0)
			return m_dts;
		return 0.0;
	}
	inline double dtsPts() const
	{
		if (m_dts >= 0.0)
			return m_dts;
		if (m_pts >= 0.0)
			return m_pts;
		return 0.0;
	}

private:
	double m_pts, m_dts;
};

#endif //TIMESTAMP_HPP
