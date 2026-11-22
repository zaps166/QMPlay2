// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <DeintHWPrepareFilter.hpp>

#include <QDebug>

DeintHWPrepareFilter::DeintHWPrepareFilter()
    : VideoFilter(false)
{
    m_supportedPixelFormats = {
        AV_PIX_FMT_NV12
    };

    addParam("Deinterlace");
    addParam("DeinterlaceFlags");
}
DeintHWPrepareFilter::~DeintHWPrepareFilter()
{
}

bool DeintHWPrepareFilter::filter(QQueue<Frame> &framesQueue)
{
    addFramesToInternalQueue(framesQueue);
    if (!m_internalQueue.isEmpty())
    {
        Frame frame = m_internalQueue.constFirst();

        if (!m_deinterlace)
            frame.setNoInterlaced();
        else if (!(m_deintFlags & AutoDeinterlace) || frame.isInterlaced())
            frame.setInterlaced(isTopFieldFirst(frame));

        if ((m_deintFlags & DoubleFramerate) && frame.isInterlaced())
            deinterlaceDoublerCommon(frame);
        else
            m_internalQueue.removeFirst();

        framesQueue.enqueue(frame);
    }
    return !m_internalQueue.isEmpty();
}

bool DeintHWPrepareFilter::processParams(bool *paramsCorrected)
{
    Q_UNUSED(paramsCorrected)

    processParamsDeint();
    m_deinterlace = getParam("Deinterlace").toBool();

    return true;
}
