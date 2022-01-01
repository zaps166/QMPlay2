/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

int PacketBuffer::backwardPackets;

bool PacketBuffer::seekTo(double seekPos, bool backward)
{
    const int count = packetsCount();
    if (count == 0)
        return false;

    const bool findBackwards = (m_pos > 0 && seekPos < at(m_pos - 1).ts());

    if (findBackwards && at(0).ts() > seekPos)
    {
        if (floor(at(0).ts()) > seekPos)
            return false; // No packets for backward seek
        seekPos = at(0).ts();
    }
    else if (!findBackwards && at(count - 1).ts() < seekPos)
    {
        if (ceil(at(count - 1).ts()) < seekPos)
            return false; // No packets for forward seek
        seekPos = at(count - 1).ts();
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
                const Packet &pkt = at(i);
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
            const Packet &pkt = at(i);
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
        if (at(tmpPos).hasKeyFrame() || doSeek(tmpPos, !backward, true))
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
    QList<Packet>::clear();
    m_remainingDuration = m_backwardDuration = 0.0;
    m_remainingBytes = m_backwardBytes = 0;
    m_pos = 0;
    unlock();
}

void PacketBuffer::put(const Packet &packet)
{
    lock();
    clearBackwards();
    append(packet);
    m_remainingBytes += packet.size();
    m_remainingDuration += packet.duration();
    unlock();
}
Packet PacketBuffer::fetch()
{
    const Packet &packet = at(m_pos++);
    m_remainingDuration -= packet.duration();
    m_backwardDuration += packet.duration();
    m_remainingBytes -= packet.size();
    m_backwardBytes += packet.size();
    return packet;
}

void PacketBuffer::clearBackwards()
{
    while (m_pos > backwardPackets)
    {
        const Packet &tmpPacket = first();
        m_backwardDuration -= tmpPacket.duration();
        m_backwardBytes -= tmpPacket.size();
        removeFirst();
        --m_pos;
    }
}
