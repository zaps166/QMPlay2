#ifndef PACKET_HPP
#define PACKET_HPP

#include <TimeStamp.hpp>

#include <QByteArray>

struct Packet
{
	QByteArray data;
	TimeStamp ts;
	double duration;
};

#endif
