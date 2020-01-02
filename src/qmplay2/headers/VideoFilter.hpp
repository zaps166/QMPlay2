/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2020  Błażej Szczygieł

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

#pragma once

#include <ModuleParams.hpp>
#include <Frame.hpp>

#include <QQueue>

class QMPLAY2SHAREDLIB_EXPORT VideoFilter : public ModuleParams
{
public:
    enum DeintFlags
    {
        AutoDeinterlace = 0x1,
        DoubleFramerate = 0x2,
        AutoParity = 0x4,
        TopFieldFirst = 0x8
    };

public:
    virtual void clearBuffer();

    bool removeLastFromInternalBuffer();

    virtual bool filter(QQueue<Frame> &framesQueue) = 0;

protected:
    void processParamsDeint();

    void addFramesToInternalQueue(QQueue<Frame> &framesQueue);
    void addFramesToDeinterlace(QQueue<Frame> &framesQueue);

    void deinterlaceDoublerCommon(Frame &frame);

    bool isTopFieldFirst(const Frame &videoFrame) const;

    double getMidFrameTS(double ts1, double ts2) const;

protected:
    QQueue<Frame> m_internalQueue;

    quint8 m_deintFlags = 0;

    // For doubler
    bool m_secondFrame = false;
    double m_lastTS = qQNaN();
};
