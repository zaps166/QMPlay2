/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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
#include <Frame.hpp>

#include <algorithm>

MotionBlur::MotionBlur()
    : VideoFilter(true)
{
    addParam("W");
    addParam("H");
}

bool MotionBlur::filter(QQueue<Frame> &framesQueue)
{
    addFramesToInternalQueue(framesQueue);
    if (m_internalQueue.count() >= 2)
    {
        const Frame videoFrame1 = m_internalQueue.dequeue();
        Frame videoFrame2 = getNewFrame(videoFrame1);
        const Frame &videoFrame3 = m_internalQueue.at(0);

        for (int p = 0; p < 3; ++p)
        {
            const quint8 *src1 = videoFrame1.constData(p);
            const quint8 *src2 = videoFrame3.constData(p);
            quint8 *dest = videoFrame2.data(p);
            const int linesizeSrc1 = videoFrame1.linesize(p);
            const int linesizeDest = videoFrame2.linesize(p);
            const int linesizeSrc2 = videoFrame3.linesize(p);
            const int minLinesize = std::min({linesizeSrc1, linesizeDest, linesizeSrc2});
            const int h = videoFrame1.height(p);
            for (int i = 0; i < h; ++i)
            {
                VideoFilters::averageTwoLines(dest, src1, src2, minLinesize);
                dest += linesizeDest;
                src1 += linesizeSrc1;
                src2 += linesizeSrc2;
            }
        }

        videoFrame2.setTS(getMidFrameTS(videoFrame2.ts(), videoFrame3.ts()));

        framesQueue.enqueue(videoFrame1);
        framesQueue.enqueue(videoFrame2);
    }
    return m_internalQueue.count() >= 2;
}

bool MotionBlur::processParams(bool *)
{
    if (getParam("W").toInt() < 2 || getParam("H").toInt() < 4)
        return false;
    return true;
}
