#include <PacketBuffer.hpp>

int PacketBuffer::backwardPackets;

bool PacketBuffer::seekTo( double seek_pos, bool backwards )
{
	if ( isEmpty() )
		return true;

	if ( backwards && at( 0 ).ts > seek_pos )
		return false; //Brak paczek do skoku w tył

	double durationToChange = 0.0;
	qint64 sizeToChange = 0;

	if ( !backwards ) //Skok do przodu
	{
		int count = packetsCount();
		for ( int i = pos ; i < count ; ++i )
		{
			const Packet &pkt = at( i );
			if ( pkt.ts < seek_pos )
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
	else for ( int i = pos ; i >= 0 ; --i ) //Skok do tyłu
	{
		const Packet &pkt = at( i );
		if ( pkt.ts > seek_pos )
		{
			durationToChange += pkt.duration;
			sizeToChange += pkt.size();
		}
		else
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
		backward_duration -= first().duration;
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
	const Packet &pkt = at( pos++ );
	remaining_duration -= pkt.duration;
	backward_duration += pkt.duration;
	remaining_bytes -= pkt.size();
	backward_bytes += pkt.size();
	return pkt;
}
