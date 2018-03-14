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

#include <QMPlay2Lib.hpp>

#include <QMutex>
#include <QList>
#include <QRect>
#include <QTime>

class QMPLAY2SHAREDLIB_EXPORT QMPlay2OSD
{
public:
	class Image
	{
	public:
		Image() = default;
		inline Image(const QRect &rect, const QByteArray &data) :
			rect(rect), data(data)
		{}

		QRect rect;
		QByteArray data;
	};

	inline QMPlay2OSD()
	{
		clear();
	}

	inline void setText(const QByteArray &txt)
	{
		m_text = txt;
	}
	inline void setDuration(double d)
	{
		m_duration = d;
	}
	//for subtitles only, not for OSD
	inline void setPTS(double p)
	{
		m_pts = p;
	}
	inline void setNeedsRescale()
	{
		m_needsRescale = true;
	}

	inline const Image &getImage(int idx) const
	{
		return m_images[idx];
	}
	inline int imageCount() const
	{
		return m_images.count();
	}
	inline void addImage(const QRect &rect, const QByteArray &data)
	{
		m_images.push_back(Image(rect, data));
	}

	void genId();

	void clear(bool all = true);

	inline void lock() const
	{
		m_mutex.lock();
	}
	inline void unlock() const
	{
		m_mutex.unlock();
	}

	inline QByteArray text() const
	{
		return m_text;
	}
	//for subtitles only
	inline double duration() const
	{
		return m_duration;
	}
	inline double pts() const
	{
		return m_pts;
	}
	inline bool isStarted() const
	{
		return m_started;
	}
	inline bool needsRescale() const
	{
		return m_needsRescale;
	}

	inline quint64 getId() const
	{
		return m_id;
	}

	void start(); //for OSD: start counting leftDuration; for subtitles: marks that subtitles are displayed

	//for OSD only
	double leftDuration(); //if < 0 then time out and you must this delete class
private:
	QList<Image> m_images;

	QByteArray m_text;
	double m_duration, m_pts;
	bool m_started, m_needsRescale;
	QTime m_timer;
	mutable QMutex m_mutex;
	quint64 m_id;
};
