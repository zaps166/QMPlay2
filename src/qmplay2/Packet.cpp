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

#include <Packet.hpp>

#include <QDebug>

Packet::Packet()
    : m_packet(av_packet_alloc())
{
    m_packet->flags = AV_PKT_FLAG_KEY;
}
Packet::Packet(AVPacket *packet, bool forceCopy)
    : Packet()
{
    av_packet_ref(m_packet, packet);
    if (forceCopy && m_packet->buf)
    {
        const auto offset = m_packet->data - m_packet->buf->data;
        av_buffer_make_writable(&m_packet->buf);
        m_packet->data = m_packet->buf->data + offset;
    }
}
Packet::Packet(const Packet &other)
    : Packet()
{
    *this = other;
}
Packet::Packet(Packet &&other)
    : Packet()
{
    *this = std::move(other);
}
Packet::~Packet()
{
    av_packet_free(&m_packet);
}

bool Packet::isEmpty() const
{
    return (m_packet->size <= 0);
}
void Packet::clear()
{
    av_packet_unref(m_packet);
}

void Packet::setTimeBase(const AVRational &timeBase)
{
    m_timeBase = timeBase;
}

void Packet::resize(int size)
{
    av_buffer_realloc(&m_packet->buf, size);
    m_packet->data = m_packet->buf->data;
    m_packet->size = m_packet->buf->size;
}
int Packet::size() const
{
    return m_packet->size;
}

AVBufferRef *Packet::getBufferRef() const
{
    return av_buffer_ref(m_packet->buf);
}
uint8_t *Packet::data() const
{
    return m_packet->data;;
}

double Packet::duration() const
{
    return m_packet->duration * av_q2d(m_timeBase);
}
void Packet::setDuration(double duration)
{
    m_packet->duration = qRound64(duration / av_q2d(m_timeBase));
}

bool Packet::hasKeyFrame() const
{
    return (m_packet->flags & AV_PKT_FLAG_KEY);
}

bool Packet::isTsValid() const
{
    return (hasDts() || hasPts());
}
void Packet::setTsInvalid()
{
    m_packet->dts = m_packet->pts = AV_NOPTS_VALUE;
}

bool Packet::hasDts() const
{
    return (m_packet->dts != AV_NOPTS_VALUE);
}
double Packet::dts() const
{
    return m_packet->dts * av_q2d(m_timeBase);
}
void Packet::setDts(double dts)
{
    m_packet->dts = qRound64(dts / av_q2d(m_timeBase));
}

bool Packet::hasPts() const
{
    return (m_packet->pts != AV_NOPTS_VALUE);
}
double Packet::pts() const
{
    return m_packet->pts * av_q2d(m_timeBase);
}
void Packet::setPts(double pts)
{
    m_packet->pts = qRound64(pts / av_q2d(m_timeBase));
}

double Packet::ts() const
{
    // XXX: Should it always return dts?
    if (hasDts() && m_packet->dts >= 0)
        return dts();
    if (hasPts() && m_packet->pts >= 0)
        return pts();
    return 0.0;
}
void Packet::setTS(double ts)
{
    setDts(ts);
    setPts(ts);
}

Packet &Packet::operator =(const Packet &other)
{
    av_packet_move_ref(m_packet, other.m_packet);
    sampleAspectRatio = other.sampleAspectRatio;
    m_timeBase = other.m_timeBase;
    return *this;
}
Packet &Packet::operator =(Packet &&other)
{
    av_packet_ref(m_packet, other.m_packet);
    qSwap(sampleAspectRatio, other.sampleAspectRatio);
    qSwap(m_timeBase, other.m_timeBase);
    return *this;
}
