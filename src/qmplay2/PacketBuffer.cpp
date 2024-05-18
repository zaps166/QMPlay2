/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

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

#include <PacketBuffer.hpp>

#include <cmath>

#include <QDebug>

double PacketBuffer::s_backwardTime;

void PacketBuffer::iterate(const IterateCallback &cb)
{
    lock();

    const int count = packetsCount();

    int startPos = m_pos;
    if (startPos >= count)
    {
        unlock();
        return;
    }

    for (int i = startPos; i >= 0; --i) // Find nearest keyframe (backwards)
    {
        auto &packet = atPos(i);
        if (packet.hasKeyFrame())
        {
            startPos = i;
            break;
        }
    }

    // Iterate packets starting when found keyframe
    bool hasKeyframe = false;
    for (int i = startPos; i < count; ++i)
    {
        auto &packet = atPos(i);
        if (!hasKeyframe && packet.hasKeyFrame())
            hasKeyframe = true;
        if (hasKeyframe)
            cb(packet);
    }

    unlock();
}

bool PacketBuffer::seekTo(double seekPos, bool backward)
{
    const int count = packetsCount();
    if (count == 0)
        return false;

    const bool findBackwards = (m_pos > 0 && seekPos < atPos(m_pos - 1).ts());

    if (findBackwards && atPos(0).ts() > seekPos)
    {
        if (floor(atPos(0).ts()) > seekPos)
            return false; // No packets for backward seek
        seekPos = atPos(0).ts();
    }
    else if (!findBackwards && atPos(count - 1).ts() < seekPos)
    {
        if (ceil(atPos(count - 1).ts()) < seekPos)
            return false; // No packets for forward seek
        seekPos = atPos(count - 1).ts();
    }

    double durationToChange = 0.0;
    qint64 sizeToChange = 0;
    int tmpPos;

    const auto doSeek = [&](const int currPos, const bool forward, const bool keyFrame) {
        tmpPos = -1;
        if (forward)
        {
            for (int i = currPos; i < count; ++i)
            {
                const Packet &pkt = atPos(i);
                if (pkt.ts() >= seekPos && (!keyFrame || pkt.hasKeyFrame()))
                {
                    tmpPos = i;
                    return true;
                }
                else
                {
                    durationToChange += pkt.duration();
                    sizeToChange += pkt.size();
                }
            }
        }
        else for (int i = currPos - 1; i >= 0; --i)
        {
            const Packet &pkt = atPos(i);
            durationToChange -= pkt.duration();
            sizeToChange -= pkt.size();
            if (pkt.ts() <= seekPos && (!keyFrame || pkt.hasKeyFrame()))
            {
                tmpPos = i;
                return true;
            }
        }
        return false;
    };

    if (doSeek(m_pos, !findBackwards, false))
    {
        if (atPos(tmpPos).hasKeyFrame() || doSeek(tmpPos, !backward, true))
        {
            m_remainingDuration -= durationToChange;
            m_backwardDuration += durationToChange;
            m_remainingBytes -= sizeToChange;
            m_backwardBytes += sizeToChange;

            m_pos = tmpPos;

            return true;
        }
    }

    return false;
}
void PacketBuffer::clear()
{
    lock();
    std::deque<Packet>::clear();
    m_remainingDuration = m_backwardDuration = 0.0;
    m_remainingBytes = m_backwardBytes = 0;
    m_pos = 0;
    unlock();
}

void PacketBuffer::put(const Packet &packet)
{
    lock();
    clearBackwards();
    push_back(packet);
    m_remainingBytes += packet.size();
    m_remainingDuration += packet.duration();
    unlock();
}
Packet PacketBuffer::fetch()
{
    const Packet &packet = atPos(m_pos++);
    m_remainingDuration -= packet.duration();
    m_backwardDuration += packet.duration();
    m_remainingBytes -= packet.size();
    m_backwardBytes += packet.size();
    return packet;
}

void PacketBuffer::clearBackwards()
{
    while (m_backwardDuration > s_backwardTime && m_pos > 0)
    {
        const Packet &tmpPacket = *begin();
        m_backwardDuration -= tmpPacket.duration();
        m_backwardBytes -= tmpPacket.size();
        erase(begin());
        --m_pos;
    }
}

inline Packet &PacketBuffer::atPos(size_type idx)
{
    return operator[](idx);
}
