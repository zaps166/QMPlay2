/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

#ifndef PACKETBUFFER_HPP
#define PACKETBUFFER_HPP

#include <Packet.hpp>

#include <QMutex>
#include <QList>

class PacketBuffer : private QList<Packet>
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
		return packetsCount() - pos;
	}
	inline int packetsCount() const
	{
		return count();
	}

	inline double firstPacketTime() const
	{
		return begin()->ts;
	}
	inline double lastPacketTime() const
	{
		return (--end())->ts;
	}

	inline double remainingDuration() const
	{
		return remaining_duration;
	}
	inline double backwardDuration() const
	{
		return backward_duration;
	}

	inline qint64 remainingBytes() const
	{
		return remaining_bytes;
	}
	inline qint64 backwardBytes() const
	{
		return backward_bytes;
	}

	inline void lock()
	{
		mutex.lock();
	}
	inline void unlock()
	{
		mutex.unlock();
	}
private:
	double remaining_duration = 0.0, backward_duration = 0.0;
	qint64 remaining_bytes = 0, backward_bytes = 0;
	QMutex mutex;
	int pos = 0;
};

#endif // PACKETBUFFER_HPP
