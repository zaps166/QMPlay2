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

	inline PacketBuffer() :
		remaining_duration(0.0), backward_duration(0.0),
		remaining_bytes(0), backward_bytes(0),
		pos(0)
	{}

	bool seekTo(double seekPos, bool backward);
	void clear(); //Thread-safe

	void put(const Packet &packet); //Thread-safe
	Packet fetch();

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
	double remaining_duration, backward_duration;
	qint64 remaining_bytes, backward_bytes;
	QMutex mutex;
	int pos;
};

#endif // PACKETBUFFER_HPP
