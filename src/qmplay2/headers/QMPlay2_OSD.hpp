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

#ifndef QMPlay2_OSD_HPP
#define QMPlay2_OSD_HPP

#include <QMutex>
#include <QList>
#include <QRect>
#include <QTime>

class QMPlay2_OSD
{
public:
	typedef QByteArray Checksum;

	class Image
	{
	public:
		inline Image() {}
		inline Image(const QRect &rect, const QByteArray &data) :
			rect(rect), data(data) {}

		QRect rect;
		QByteArray data;
	};

	inline QMPlay2_OSD()
	{
		clear();
	}

	inline void setText(const QByteArray &txt)
	{
		_text = txt;
	}
	inline void setDuration(double d)
	{
		_duration = d;
	}
	//for subtitles only, not for OSD
	inline void setPTS(double p)
	{
		_pts = p;
	}
	inline void setNeedsRescale()
	{
		_needsRescale = true;
	}

	inline const Image &getImage(int idx) const
	{
		return images[idx];
	}
	inline int imageCount() const
	{
		return images.count();
	}
	inline void addImage(const QRect &rect, const QByteArray &data)
	{
		images.push_back(Image(rect, data));
	}

	void genChecksum();

	void clear(bool all = true);

	inline void lock() const
	{
		mutex.lock();
	}
	inline void unlock() const
	{
		mutex.unlock();
	}

	inline QByteArray text()
	{
		return _text;
	}
	//for subtitles only
	inline double duration()
	{
		return _duration;
	}
	inline double pts()
	{
		return _pts;
	}
	inline bool isStarted()
	{
		return started;
	}
	inline bool needsRescale() const
	{
		return _needsRescale;
	}

	inline Checksum getChecksum() const
	{
		return checksum;
	}

	void start(); //for OSD: start counting left_duration, for subtitles: marks that subtitles are displayed

	//for OSD only
	double left_duration(); //if < 0 then time out and you must this delete class
private:
	QList<Image> images;

	QByteArray _text;
	double _duration, _pts;
	bool started, _needsRescale;
	QTime timer;
	mutable QMutex mutex;
	Checksum checksum;
};

#endif
