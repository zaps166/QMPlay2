/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2025  Błażej Szczygieł

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

#include <FPSDoubler.hpp>

#include <QDebug>

FPSDoubler::FPSDoubler(Module &module, bool &fullScreen)
    : VideoFilter(false)
    , m_fullScreen(fullScreen)
{
    SetModule(module);
}
FPSDoubler::~FPSDoubler()
{
}

bool FPSDoubler::set()
{
    m_minFps = sets().getDouble("FPSDoubler/MinFPS");
    m_maxFps = sets().getDouble("FPSDoubler/MaxFPS");

    m_onlyFullScreen = sets().getBool("FPSDoubler/OnlyFullScreen");

    return true;
}

bool FPSDoubler::filter(QQueue<Frame> &framesQueue)
{
    addFramesToInternalQueue(framesQueue);
    if (!m_internalQueue.isEmpty())
    {
        auto frame = m_internalQueue.dequeue();
        framesQueue.enqueue(frame);
        const auto frameTs = frame.ts();
        if (!qIsNaN(m_lastTS))
        {
            m_frameTimeSum += frameTs - m_lastTS;
            ++m_frames;
            if (m_frameTimeSum >= 1.0)
            {
                m_fps = m_frames / m_frameTimeSum;
                m_frameTimeSum = 0.0;
                m_frames = 0;
            }
            if (m_fps > m_minFps && m_fps < m_maxFps && (!m_onlyFullScreen || m_fullScreen))
            {
                frame.setTS(getMidFrameTS(frameTs, m_lastTS));
                framesQueue.enqueue(frame);
            }
        }
        m_lastTS = frameTs;
    }
    return !m_internalQueue.isEmpty();
}

bool FPSDoubler::processParams(bool *)
{
    m_fps = 0.0;

    m_frameTimeSum = 0.0;
    m_frames = 0;

    m_lastTS = qQNaN();

    return true;
}
