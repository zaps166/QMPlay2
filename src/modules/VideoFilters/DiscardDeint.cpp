/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

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

#include <DiscardDeint.hpp>
#include <VideoFilters.hpp>

DiscardDeint::DiscardDeint()
{
    addParam("W");
    addParam("H");
}

bool DiscardDeint::filter(QQueue<FrameBuffer> &framesQueue)
{
    addFramesToDeinterlace(framesQueue);
    if (!internalQueue.isEmpty())
    {
        FrameBuffer dequeued = internalQueue.dequeue();
        VideoFrame &videoFrame = dequeued.frame;
        const bool TFF = isTopFieldFirst(videoFrame);
        videoFrame.setNoInterlaced();
        for (int p = 0; p < 3; ++p)
        {
            const int linesize = videoFrame.linesize[p];
            quint8 *data = videoFrame.buffer[p].data();
            const int lines = (videoFrame.size.getHeight(p) >> 1) - 1;
            if (!TFF)
            {
                memcpy(data, data + linesize, linesize);
                data += linesize;
            }
            data += linesize;
            for (int i = 0; i < lines; ++i)
            {
                VideoFilters::averageTwoLines(data, data - linesize, data + linesize, linesize);
                data += linesize << 1;
            }
            if (TFF)
                memcpy(data, data - linesize, linesize);
        }
        framesQueue.enqueue(dequeued);
    }
    return !internalQueue.isEmpty();
}

bool DiscardDeint::processParams(bool *)
{
    deintFlags = getParam("DeinterlaceFlags").toInt();
    if (getParam("W").toInt() < 2 || getParam("H").toInt() < 4 || (deintFlags & DoubleFramerate))
        return false;
    return true;
}
