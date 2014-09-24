#include <Echo.hpp>

Echo::Echo( Module &module ) :
	hasParameters( false ), canFilter( false )
{
	SetModule( module );
}

bool Echo::set()
{
	enabled = sets().getBool( "Echo" );
	echo_delay = sets().getUInt( "Echo/Delay" );
	echo_volume = sets().getUInt( "Echo/Volume" );
	echo_repeat = sets().getUInt( "Echo/Feedback" );
	echo_surround = sets().getBool( "Echo/Surround" );
	if ( echo_delay > 1000 )
		echo_delay = 1000;
	if ( echo_volume > 100 )
		echo_volume = 100;
	if ( echo_repeat > 100 )
		echo_repeat = 100;
	alloc( enabled && hasParameters );
	return true;
}

bool Echo::setAudioParameters( uchar chn, uint srate )
{
	hasParameters = chn && srate;
	if ( hasParameters )
	{
		this->chn = chn;
		this->srate = srate;
	}
	alloc( enabled && hasParameters );
	return hasParameters;
}
double Echo::filter( QByteArray &data, bool )
{
	if ( canFilter )
	{
		const int size = data.size() / sizeof( float );
		const int sampleBufferSize = sampleBuffer.size();
		const int repeat_div = echo_surround ? 200 : 100;
		float *samples = ( float * )data.data();
		int r_ofs = w_ofs - ( srate * echo_delay / 1000 ) * chn;
		if ( r_ofs < 0 )
			r_ofs += sampleBufferSize;
		for ( int i = 0 ; i < size ; i++ )
		{
			float sample = sampleBuffer[ r_ofs ];
			if ( echo_surround && chn >= 2 )
			{
				if ( i & 1 )
					sample -= sampleBuffer[ r_ofs - 1 ];
				else
					sample -= sampleBuffer[ r_ofs + 1 ];
			}
			sampleBuffer[ w_ofs ] = samples[ i ] + sample * echo_repeat / repeat_div;
			samples[ i ] += sample * echo_volume / 100;
			if ( ++r_ofs >= sampleBufferSize )
				r_ofs -= sampleBufferSize;
			if ( ++w_ofs >= sampleBufferSize )
				w_ofs -= sampleBufferSize;
		}
	}
	return 0.0;
}

void Echo::alloc( bool b )
{
	if ( !b || srate * chn != ( uint )sampleBuffer.size() )
		sampleBuffer.clear();
	if ( b && sampleBuffer.isEmpty() )
	{
		w_ofs = 0;
		sampleBuffer.fill( 0.0, srate * chn );
	}
	canFilter = b;
}
