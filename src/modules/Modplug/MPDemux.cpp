#include <MPDemux.hpp>
#include <Reader.hpp>
#include <Functions.hpp>

#include <libmodplug/modplug.h>

MPDemux::MPDemux( Module &module ) :
	aborted( false ), pos( 0.0 ), mpfile( NULL )
{
	SetModule( module );
}

MPDemux::~MPDemux()
{
	if ( mpfile )
		ModPlug_Unload( mpfile );
}

bool MPDemux::set()
{
	bool restartPlaying = false;

	ModPlug_Settings settings;
	ModPlug_GetSettings( &settings );
	if ( settings.mResamplingMode != sets().getInt( "ModplugResamplingMethod" ) )
	{
		settings.mResamplingMode = sets().getInt( "ModplugResamplingMethod" );
		restartPlaying = true;
	}
	settings.mFlags = MODPLUG_ENABLE_OVERSAMPLING;
	settings.mChannels = 2;
	settings.mBits = 32;
	settings.mFrequency = 44100;
	ModPlug_SetSettings( &settings );

	return !restartPlaying && sets().getBool( "ModplugEnabled" );
}

QString MPDemux::name() const
{
	switch ( ModPlug_GetModuleType( mpfile ) )
	{
		case 0x01:
			return "ProTracker MOD";
		case 0x02:
			return "ScreamTracker S3M";
		case 0x04:
			return "FastTracker XM";
		case 0x08:
			return "OctaMED";
		case 0x10:
			return "Multitracker MTM";
		case 0x20:
			return "ImpulseTracker IT";
		case 0x40:
			return "UNIS Composer 669";
		case 0x80:
			return "UltraTracker ULT";
		case 0x100:
			return "ScreamTracker STM";
		case 0x200:
			return "Farandole Composer FAR";
		case 0x800:
		case 0x200000:
			return "Advanced Module File AMF";
		case 0x1000:
			return "Extreme Tracker Module AMS";
		case 0x2000:
			return "Digital Sound Module DSM";
		case 0x4000:
			return "DigiTrakker Module MDL";
		case 0x8000:
			return "Oktalyzer Module OKT";
		case 0x20000:
			return "Delusion Digital Music File DMF";
		case 0x40000:
			return "PolyTracker Module PTM";
		case 0x80000:
			return "DigiBooster Pro DBM";
		case 0x100000:
			return "MadTracker MT2";
		case 0x400000:
			return "Protracker Studio Module PSM";
		case 0x800000:
			return "Jazz Jackrabbit 2 Music J2B";
		case 0x1000000:
			return "Amiga SoundFX";
	}
	return "";
}
QString MPDemux::title() const
{
	return ModPlug_GetName( mpfile );
}
double MPDemux::length() const
{
	return ModPlug_GetLength( mpfile ) / 1000.0;
}
int MPDemux::bitrate() const
{
	return -1;
}

bool MPDemux::seek( int val, bool backward )
{
	Q_UNUSED( backward )
	if ( val >= length() )
		val = length()-1;
	ModPlug_Seek( mpfile, val * 1000 );
	pos = val;
	return true;
}
bool MPDemux::read( QByteArray &decoded, int &idx, TimeStamp &ts, double &duration )
{
	if ( aborted )
		return false;

	decoded.resize( 1024*2*4 ); //BASE_SIZE * CHN * BITS/8
	decoded.resize( ModPlug_Read( mpfile, decoded.data(), decoded.size() ) );

	if ( !decoded.size() )
		return false;

	//Konwersja 32bit-int na 32bit-float
	for ( unsigned i = 0 ; i < decoded.size() / sizeof( float ) ; ++i )
		( ( float * )decoded.data() )[ i ] = ( ( int * )decoded.data() )[ i ]/2147483648.0;

	idx = 0;
	ts = pos;
	duration = ( double )decoded.size() / ( 44100*2*4 ); //SRATE * CHN * BITS/8
	pos += duration;

	return true;
}
void MPDemux::abort()
{
	aborted = true;
	reader.abort();
}

bool MPDemux::open( const QString &url )
{
	if ( Reader::create( url, reader ) )
	{
		if ( reader->size() > 0 )
			mpfile = ModPlug_Load( reader->read( reader->size() ), reader->size() );
		reader.clear();
		if ( mpfile && ModPlug_GetModuleType( mpfile ) )
		{
			StreamInfo *streamInfo = new StreamInfo;
			streamInfo->type = QMPLAY2_TYPE_AUDIO;
			streamInfo->is_default = true;
			streamInfo->sample_rate = 44100;
			streamInfo->channels = 2;
			streamInfo->other_info += qMakePair( tr( "próbki" ), QString::number( ModPlug_NumSamples( mpfile ) ) );
			streamInfo->other_info += qMakePair( tr( "wzorce" ), QString::number( ModPlug_NumPatterns( mpfile ) ) );
			streamInfo->other_info += qMakePair( tr( "kanały" ), QString::number( ModPlug_NumChannels( mpfile ) ) );
			streams_info += streamInfo;

			ModPlug_SetMasterVolume( mpfile, 256 ); //?
			return true;
		}
	}
	return false;
}
