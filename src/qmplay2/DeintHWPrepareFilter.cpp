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

#include <DeintHWPrepareFilter.hpp>

#include <QDebug>

DeintHWPrepareFilter::DeintHWPrepareFilter()
{
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
