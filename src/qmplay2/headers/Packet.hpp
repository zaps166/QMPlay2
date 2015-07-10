#ifndef PACKET_HPP
#define PACKET_HPP

#include <TimeStamp.hpp>

#include <QByteArray>

struct Packet : public QByteArray
{
	inline void reset()
	{
		*this = Packet();
	}

	TimeStamp ts;
	double duration;
	bool hasKeyFrame;
};

#endif
