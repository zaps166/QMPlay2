// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <MotionBlur.hpp>
#include <VideoFilters.hpp>
#include <Frame.hpp>

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
