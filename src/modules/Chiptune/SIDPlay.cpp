#include <SIDPlay.hpp>

#include <Reader.hpp>
#include <Packet.hpp>

#include <sidplayfp/SidTuneInfo.h>
#include <sidplayfp/SidConfig.h>
#include <sidplayfp/SidTune.h>
#include <sidplayfp/SidInfo.h>

#include <QDebug>

SIDPlay::SIDPlay( Module &module ) :
	aborted( false ),
	rs( NULL ),
	tune( NULL )
{
	SetModule( module );
}
SIDPlay::~SIDPlay()
{
	delete tune;
}

bool SIDPlay::set()
{
	return sets().getBool( "SIDPlay" );
}

QString SIDPlay::name() const
{
	return tune->getInfo()->formatString();
}
QString SIDPlay::title() const
{
	const SidTuneInfo *info = tune->getInfo();
	const QString title  = info->infoString(0);
	const QString author = info->infoString(1);
	return author + " - " + title;
}
double SIDPlay::length() const
{
	return tune->getInfo()->songs() - 1;
}
int SIDPlay::bitrate() const
{
	return -1;
}

bool SIDPlay::dontUseBuffer() const
{
	return true; //Allow seek backwards (seeking changes the song...)
}

bool SIDPlay::seek( int s, bool )
{
	++s;
	return ( ( int )tune->selectSong( s ) == s ) && sidplay.load( tune );
}
bool SIDPlay::read( Packet &decoded, int &idx )
{
	if ( aborted )
		return false;

	const int chunkSize = 1024 * chn;

	decoded.resize( chunkSize * sizeof( float ) );

	int16_t *srcData = ( int16_t * )decoded.data();
	float *dstData = ( float * )decoded.data();

	sidplay.play( srcData, chunkSize );
	for ( int i = chunkSize - 1 ; i >= 0 ; --i )
		dstData[ i ] = srcData[ i ] / 16384.0;

	decoded.ts = tune->getInfo()->currentSong() - 1.0; //sidplay.time();
	decoded.duration = decoded.size() / chn / sizeof( float ) / ( double )srate;

	idx = 0;

	return true;
}
void SIDPlay::abort()
{
	reader.abort();
	aborted = true;
}

bool SIDPlay::open( const QString &url )
{
	if ( Reader::create( url, reader ) )
	{
		const QByteArray data = reader->read( reader->size() );
		reader.clear();

		tune = new SidTune( ( const quint8 * )data.data(), data.length() );
		if ( !tune->getStatus() )
			return false;

		const SidTuneInfo *info = tune->getInfo();

		rs.create( sidplay.info().maxsids() );
		if ( !rs.getStatus() )
			return false;

		const bool isStereo = info->sidChips() > 1 ? true : false;

		SidConfig cfg;
		if ( QMPlay2Core.getSettings().getBool( "ForceSamplerate" ) )
			cfg.frequency = QMPlay2Core.getSettings().getInt( "Samplerate" );
		else
			cfg.frequency = 48000; //Use 48kHz as default
		cfg.sidEmulation = &rs;
		if ( isStereo )
			cfg.playback = SidConfig::STEREO;
		if ( !sidplay.config( cfg ) )
			return false;

		StreamInfo *streamInfo = new StreamInfo;
		streamInfo->type = QMPLAY2_TYPE_AUDIO;
		streamInfo->is_default = true;
		srate = streamInfo->sample_rate = cfg.frequency;
		chn = streamInfo->channels = isStereo ? 2 : 1;
		const QString released = info->infoString( 2 );
		if ( !released.isEmpty() )
			streamInfo->other_info += qMakePair( tr( "Wydano" ), released );
		streams_info += streamInfo;

		tune->selectSong( 1 );
		return sidplay.load( tune );
	}
	return false;
}
