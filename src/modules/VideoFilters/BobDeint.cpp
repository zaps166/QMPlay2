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

#include <BobDeint.hpp>
#include <VideoFilters.hpp>

#include <algorithm>

BobDeint::BobDeint()
{
    addParam("W");
    addParam("H");
}

void BobDeint::clearBuffer()
{
    secondFrame = false;
    lastTS = -1.0;
    DeintFilter::clearBuffer();
}

bool BobDeint::filter(QQueue<Frame> &framesQueue)
{
    addFramesToDeinterlace(framesQueue);
    if (internalQueue.count() >= 1)
    {
        const Frame &sourceFrame = internalQueue.at(0);

        Frame destFrame = Frame::createEmpty(sourceFrame);
        destFrame.setNoInterlaced();

        const bool parity = (isTopFieldFirst(sourceFrame) == secondFrame);

        for (int p = 0; p < 3; ++p)
        {
            const int linesizeSrc = sourceFrame.linesize(p);
            const int linesizeDst = destFrame.linesize(p);
            const int minLinesize = std::min(linesizeSrc, linesizeDst);
            const quint8 *src = sourceFrame.constData(p);
            quint8 *dst = destFrame.data(p);

            const int h = sourceFrame.height(p);
            const int halfH = (h >> 1) - 1;

            if (parity)
            {
                src += linesizeSrc;
                memcpy(dst, src, minLinesize); //Duplicate first line (simple deshake)
                dst += linesizeDst;
            }
            for (int y = 0; y < halfH; ++y)
            {
                memcpy(dst, src, minLinesize);
                dst += linesizeDst;

                VideoFilters::averageTwoLines(dst, src, src + (linesizeSrc << 1), minLinesize);
                dst += linesizeDst;

                src += linesizeSrc << 1;
            }
            memcpy(dst, src, minLinesize); //Copy last line
            if (!parity)
                memcpy(dst + linesizeDst, dst, linesizeDst);
            if (h & 1) //Duplicate last line for odd height
            {
                if (!parity)
                    dst += linesizeDst;
                memcpy(dst + linesizeDst, dst, linesizeDst);
            }
        }

        if (secondFrame)
        {
            const double ts = destFrame.ts();
            destFrame.setTS(ts + halfDelay(ts, lastTS));
        }
        framesQueue.enqueue(destFrame);

        if (secondFrame || lastTS < 0.0)
            lastTS = sourceFrame.ts();

        if (secondFrame)
            internalQueue.removeFirst();
        secondFrame = !secondFrame;
    }
    return internalQueue.count() >= 1;
}

bool BobDeint::processParams(bool *)
{
    deintFlags = getParam("DeinterlaceFlags").toInt();
    if (getParam("W").toInt() < 2 || getParam("H").toInt() < 4 || !(deintFlags & DoubleFramerate))
        return false;
    secondFrame = false;
    lastTS = -1.0;
    return true;
}
