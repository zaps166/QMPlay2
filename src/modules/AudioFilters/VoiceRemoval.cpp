#include <VoiceRemoval.hpp>

VoiceRemoval::VoiceRemoval( Module &module ) :
	hasParameters( false ), canFilter( false )
{
	SetModule( module );
}

bool VoiceRemoval::set()
{
	enabled = sets().getBool( "VoiceRemoval" );
	canFilter = enabled && hasParameters;
	return true;
}

bool VoiceRemoval::setAudioParameters( uchar chn, uint /*srate*/ )
{
	hasParameters = chn >= 2;
	if ( hasParameters )
		this->chn = chn;
	canFilter = enabled && hasParameters;
	return hasParameters;
}
double VoiceRemoval::filter( QByteArray &data, bool )
{
	if ( canFilter )
	{
		const int size = data.size() / sizeof( float );
		float *samples = ( float * )data.data();

		for ( int i = 0 ; i < size ; i += chn )
			samples[ i ] = samples[ i+1 ] = samples[ i ] - samples[ i+1 ];
	}
	return 0.0;
}
