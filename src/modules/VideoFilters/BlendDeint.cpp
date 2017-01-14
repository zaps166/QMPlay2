/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

#include <BlendDeint.hpp>
#include <VideoFilters.hpp>

BlendDeint::BlendDeint()
{
	addParam("W");
	addParam("H");
}

bool BlendDeint::filter(QQueue<FrameBuffer> &framesQueue)
{
	addFramesToDeinterlace(framesQueue);
	while (!internalQueue.isEmpty())
	{
		FrameBuffer dequeued = internalQueue.dequeue();
		VideoFrame &videoFrame = dequeued.frame;
		videoFrame.setNoInterlaced();
		for (int p = 0; p < 3; ++p)
		{
			const int linesize = videoFrame.linesize[p];
			quint8 *data = videoFrame.buffer[p].data() + linesize;
			const int h = videoFrame.size.getHeight(p) - 2;
			for (int i = 0; i < h; ++i)
			{
				VideoFilters::averageTwoLines(data, data, data + linesize, linesize);
				data += linesize;
			}
		}
		framesQueue.enqueue(dequeued);
	}
	return !internalQueue.isEmpty();
}

bool BlendDeint::processParams(bool *)
{
	deintFlags = getParam("DeinterlaceFlags").toInt();
	if (getParam("W").toInt() < 2 || getParam("H").toInt() < 4 || (deintFlags & DoubleFramerate))
		return false;
	return true;
}
