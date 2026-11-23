// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <BobDeint.hpp>
#include <VideoFilters.hpp>

BobDeint::BobDeint()
    : VideoFilter(true)
{
    addParam("DeinterlaceFlags");
    addParam("W");
    addParam("H");
}

bool BobDeint::filter(QQueue<Frame> &framesQueue)
{
    addFramesToDeinterlace(framesQueue);
    if (!m_internalQueue.isEmpty())
    {
        const Frame &sourceFrame = m_internalQueue.at(0);

        Frame destFrame = getNewFrame(sourceFrame);
        destFrame.setNoInterlaced();

        const bool parity = (isTopFieldFirst(sourceFrame) == m_secondFrame);

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

        deinterlaceDoublerCommon(destFrame);
        framesQueue.enqueue(destFrame);
    }
    return !m_internalQueue.isEmpty();
}

bool BobDeint::processParams(bool *)
{
    processParamsDeint();
    if (getParam("W").toInt() < 2 || getParam("H").toInt() < 4 || !(m_deintFlags & DoubleFramerate))
        return false;
    return true;
}
