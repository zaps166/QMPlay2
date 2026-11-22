// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <DiscardDeint.hpp>
#include <VideoFilters.hpp>

DiscardDeint::DiscardDeint()
    : VideoFilter(true)
{
    addParam("DeinterlaceFlags");
    addParam("W");
    addParam("H");
}

bool DiscardDeint::filter(QQueue<Frame> &framesQueue)
{
    addFramesToDeinterlace(framesQueue);
    if (!m_internalQueue.isEmpty())
    {
        Frame videoFrame = m_internalQueue.dequeue();
        const bool TFF = isTopFieldFirst(videoFrame);
        videoFrame.setNoInterlaced();
#ifdef USE_VULKAN
        if (videoFrame.vulkanImage())
        {
            auto newFrame = getNewFrame(videoFrame);
            videoFrame.copyData(newFrame.dataArr(), newFrame.linesize());
            videoFrame = std::move(newFrame);
        }
#endif
        for (int p = 0; p < 3; ++p)
        {
            const int linesize = videoFrame.linesize(p);
            quint8 *data = videoFrame.data(p);
            const int lines = (videoFrame.height(p) >> 1) - 1;
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
        framesQueue.enqueue(videoFrame);
    }
    return !m_internalQueue.isEmpty();
}

bool DiscardDeint::processParams(bool *)
{
    processParamsDeint();
    if (getParam("W").toInt() < 2 || getParam("H").toInt() < 4 || (m_deintFlags & DoubleFramerate))
        return false;
    return true;
}
