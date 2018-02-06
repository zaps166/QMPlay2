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

#include <MotionBlur.hpp>
#include <VideoFilters.hpp>
#include <VideoFrame.hpp>

MotionBlur::MotionBlur()
{
	addParam("W");
	addParam("H");
}

bool MotionBlur::filter(QQueue<FrameBuffer> &framesQueue)
{
	addFramesToInternalQueue(framesQueue);
	if (internalQueue.count() >= 2)
	{
		FrameBuffer dequeued      = internalQueue.dequeue();
		const FrameBuffer &lookup = internalQueue.at(0);

		const VideoFrame &videoFrame1 = dequeued.frame;
		VideoFrame videoFrame2(videoFrame1.size, videoFrame1.linesize);
		const VideoFrame &videoFrame3 = lookup.frame;

		for (int p = 0; p < 3; ++p)
		{
			const quint8 *src1 = videoFrame1.buffer[p].data();
			const quint8 *src2 = videoFrame3.buffer[p].data();
			quint8 *dest = videoFrame2.buffer[p].data();
			const int linesize = videoFrame1.linesize[p];
			const int h = videoFrame1.size.getHeight(p);
			for (int i = 0; i < h; ++i)
			{
				VideoFilters::averageTwoLines(dest, src1, src2, linesize);
				dest += linesize;
				src1 += linesize;
				src2 += linesize;
			}
		}

		framesQueue.enqueue(dequeued);
		framesQueue.enqueue(FrameBuffer(videoFrame2, dequeued.ts + halfDelay(lookup.ts, dequeued.ts)));
	}
	return internalQueue.count() >= 2;
}

bool MotionBlur::processParams(bool *)
{
	if (getParam("W").toInt() < 2 || getParam("H").toInt() < 4)
		return false;
	return true;
}
