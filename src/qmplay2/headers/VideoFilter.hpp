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

#include <ModuleParams.hpp>
#include <VideoFrame.hpp>

#include <QQueue>

class QMPLAY2SHAREDLIB_EXPORT VideoFilter : public ModuleParams
{
public:
	class FrameBuffer
	{
	public:
		FrameBuffer() = default;
		inline FrameBuffer(const VideoFrame &frame, double ts) :
			frame(frame),
			ts(ts)
		{}

		VideoFrame frame;
		double ts = 0.0;
	};

	virtual ~VideoFilter() = default;

	virtual void clearBuffer();

	bool removeLastFromInternalBuffer();

	virtual bool filter(QQueue<FrameBuffer> &framesQueue) = 0;

protected:
	void addFramesToInternalQueue(QQueue<FrameBuffer> &framesQueue);

	inline double halfDelay(double f1_ts, double f2_ts) const
	{
		return (f1_ts - f2_ts) / 2.0;
	}

	QQueue<FrameBuffer> internalQueue;
};
