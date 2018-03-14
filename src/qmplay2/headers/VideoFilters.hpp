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

#include <VideoFilter.hpp>

#include <QWaitCondition>
#include <QThread>
#include <QVector>
#include <QMutex>
#include <QQueue>

class VideoFiltersThr;
class TimeStamp;

class QMPLAY2SHAREDLIB_EXPORT VideoFilters
{
	Q_DISABLE_COPY(VideoFilters)
	friend class VideoFiltersThr;
public:
	static void init();

	static inline void averageTwoLines(quint8 *dest, const quint8 *src1, const quint8 *src2, int linesize)
	{
		averageTwoLinesPtr(dest, src1, src2, linesize);
	}

	VideoFilters();
	~VideoFilters();

	void start();
	void clear();

	VideoFilter *on(const QString &filterName);
	void off(VideoFilter *&videoFilter);

	void clearBuffers();
	void removeLastFromInputBuffer();

	void addFrame(const VideoFrame &videoFrame, double ts);
	bool getFrame(VideoFrame &videoFrame, TimeStamp &ts);

	bool readyRead();
private:
	static void (*averageTwoLinesPtr)(quint8 *, const quint8 *, const quint8 *, int);

	QQueue<VideoFilter::FrameBuffer> outputQueue;
	QVector<VideoFilter *> filters;
	VideoFiltersThr &filtersThr;
	bool outputNotEmpty = false;
};
