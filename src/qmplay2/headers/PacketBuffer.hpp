/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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

#include <Packet.hpp>

#include <QMutex>
#include <QList>

class QMPLAY2SHAREDLIB_EXPORT PacketBuffer : private QList<Packet>
{
	static int backwardPackets;
public:
	static void setBackwardPackets(int backwardPackets)
	{
		PacketBuffer::backwardPackets = backwardPackets;
	}

	bool seekTo(double seekPos, bool backward);
	void clear(); //Thread-safe

	void put(const Packet &packet); //Thread-safe
	Packet fetch();

	void clearBackwards();

	inline bool isEmpty() const
	{
		return QList<Packet>::isEmpty();
	}

	inline bool canFetch() const
	{
		return remainingPacketsCount() > 0;
	}
	inline int remainingPacketsCount() const
	{
		return packetsCount() - m_pos;
	}
	inline int packetsCount() const
	{
		return count();
	}

	inline double firstPacketTime() const
	{
		return begin()->ts;
	}
	inline double currentPacketTime() const
	{
		return at(m_pos).ts;
	}
	inline double lastPacketTime() const
	{
		return (--end())->ts;
	}

	inline double remainingDuration() const
	{
		return m_remainingDuration;
	}
	inline double backwardDuration() const
	{
		return m_backwardDuration;
	}

	inline qint64 remainingBytes() const
	{
		return m_remainingBytes;
	}
	inline qint64 backwardBytes() const
	{
		return m_backwardBytes;
	}

	inline void lock()
	{
		m_mutex.lock();
	}
	inline void unlock()
	{
		m_mutex.unlock();
	}
private:
	double m_remainingDuration = 0.0, m_backwardDuration = 0.0;
	qint64 m_remainingBytes = 0, m_backwardBytes = 0;
	QMutex m_mutex;
	int m_pos = 0;
};
