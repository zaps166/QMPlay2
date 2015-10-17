#include <PacketBuffer.hpp>

#include <math.h>

int PacketBuffer::backwardPackets;

bool PacketBuffer::seekTo( double seek_pos, bool backwards )
{
	if ( isEmpty() )
		return true;

	if ( backwards && at( 0 ).ts > seek_pos )
	{
		if ( floor( at( 0 ).ts ) > seek_pos )
			return false; //Brak paczek do skoku w tył
		seek_pos = at( 0 ).ts;
	}

	double durationToChange = 0.0;
	qint64 sizeToChange = 0;

	if ( !backwards ) //Skok do przodu
	{
		const int count = packetsCount();
		for ( int i = pos ; i < count ; ++i )
		{
			const Packet &pkt = at( i );
			if ( pkt.ts < seek_pos || !pkt.hasKeyFrame )
			{
				durationToChange += pkt.duration;
				sizeToChange += pkt.size();
			}
			else
			{
				remaining_duration -= durationToChange;
				backward_duration += durationToChange;
				remaining_bytes -= sizeToChange;
				backward_bytes += sizeToChange;
				pos = i;
				return true;
			}
		}
	}
	else for ( int i = pos - 1 ; i >= 0 ; --i ) //Skok do tyłu
	{
		const Packet &pkt = at( i );
		durationToChange += pkt.duration;
		sizeToChange += pkt.size();
		if ( pkt.hasKeyFrame && pkt.ts <= seek_pos )
		{
			remaining_duration += durationToChange;
			backward_duration -= durationToChange;
			remaining_bytes += sizeToChange;
			backward_bytes -= sizeToChange;
			pos = i;
			return true;
		}
	}

	return false;
}
void PacketBuffer::clear()
{
	lock();
	QList< Packet >::clear();
	remaining_duration = backward_duration = 0.0;
	remaining_bytes = backward_bytes = 0;
	pos = 0;
	unlock();
}

void PacketBuffer::put( const Packet &packet )
{
	lock();
	while ( pos > backwardPackets )
	{
		const Packet &tmpPacket = first();
		backward_duration -= tmpPacket.duration;
		backward_bytes -= tmpPacket.size();
		removeFirst();
		--pos;
	}
	append( packet );
	remaining_bytes += packet.size();
	remaining_duration += packet.duration;
	unlock();
}
Packet PacketBuffer::fetch()
{
	const Packet &packet = at( pos++ );
	remaining_duration -= packet.duration;
	backward_duration += packet.duration;
	remaining_bytes -= packet.size();
	backward_bytes += packet.size();
	return packet;
}
