// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
