// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <BlendDeint.hpp>
#include <VideoFilters.hpp>

BlendDeint::BlendDeint()
    : VideoFilter(true)
{
    addParam("DeinterlaceFlags");
    addParam("W");
    addParam("H");
}

bool BlendDeint::filter(QQueue<Frame> &framesQueue)
{
    addFramesToDeinterlace(framesQueue);
    if (!m_internalQueue.isEmpty())
    {
        Frame videoFrame = m_internalQueue.dequeue();
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
            quint8 *data = videoFrame.data(p) + linesize;
            const int h = videoFrame.height(p) - 2;
            for (int i = 0; i < h; ++i)
            {
                VideoFilters::averageTwoLines(data, data, data + linesize, linesize);
                data += linesize;
            }
        }
        framesQueue.enqueue(videoFrame);
    }
    return !m_internalQueue.isEmpty();
}

bool BlendDeint::processParams(bool *)
{
    processParamsDeint();
    if (getParam("W").toInt() < 2 || getParam("H").toInt() < 4 || (m_deintFlags & DoubleFramerate))
        return false;
    return true;
}
