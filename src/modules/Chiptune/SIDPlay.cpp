#include <SIDPlay.hpp>

#include <Functions.hpp>
#include <Reader.hpp>
#include <Packet.hpp>

#include <sidplayfp/SidTuneInfo.h>
#include <sidplayfp/SidConfig.h>
#include <sidplayfp/SidTune.h>
#include <sidplayfp/SidInfo.h>

#include <QDebug>

SIDPlay::SIDPlay( Module &module ) :
	srate( Functions::getBestSampleRate() ),
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
	len = sets().getInt( "DefaultLength" );
	return sets().getBool( "SIDPlay" );
}

QString SIDPlay::name() const
{
	return tune->getInfo()->formatString();
}
QString SIDPlay::title() const
{
	const SidTuneInfo *info = tune->getInfo();
	const QString title  = info->infoString( 0 );
	const QString author = info->infoString( 1 );
	if ( !author.isEmpty() && !title.isEmpty() )
		return author + " - " + title;
	return title;
}
QList<QMPlay2Tag> SIDPlay::tags() const
{
	QList<QMPlay2Tag> tags;
	const SidTuneInfo *info = tune->getInfo();
	const QString title    = info->infoString( 0 );
	const QString author   = info->infoString( 1 );
	const QString released = info->infoString( 2 );
	if ( !title.isEmpty() )
		tags += qMakePair( QString::number( QMPLAY2_TAG_TITLE ), title );
	if ( !author.isEmpty() )
		tags += qMakePair( QString::number( QMPLAY2_TAG_ARTIST ), author );
	if ( !released.isEmpty() )
		tags += qMakePair( QString::number( QMPLAY2_TAG_DATE ), released );
	return tags;
}
double SIDPlay::length() const
{
	return len;
}
int SIDPlay::bitrate() const
{
	return -1;
}

bool SIDPlay::seek( int s, bool backwards )
{
	if ( backwards && !sidplay.load( tune ) )
		return false;

	if ( s > 0 )
	{
		const int bufferSize = chn * srate;
		qint16 *buff1sec = new qint16[ bufferSize ];
		for ( int i = sidplay.time() ; i <= s && !aborted ; ++i )
			sidplay.play( buff1sec, bufferSize );
		delete[] buff1sec;
	}

	return true;
}
bool SIDPlay::read( Packet &decoded, int &idx )
{
	if ( aborted )
		return false;

	const int t = sidplay.time();
	if ( t > len )
		return false;

	const int chunkSize = 1024 * chn;

	decoded.resize( chunkSize * sizeof( float ) );

	int16_t *srcData = ( int16_t * )decoded.data();
	float *dstData = ( float * )decoded.data();

	sidplay.play( srcData, chunkSize );

	for ( int i = chunkSize - 1 ; i >= 0 ; --i )
		dstData[ i ] = srcData[ i ] / 16384.0;

	decoded.ts = t;
	decoded.duration = chunkSize / chn / ( double )srate;

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
		rs.filter( true );

		const bool isStereo = info->sidChips() > 1 ? true : false;

		SidConfig cfg;
		cfg.frequency = srate;
		cfg.sidEmulation = &rs;
		if ( isStereo )
			cfg.playback = SidConfig::STEREO;
		cfg.samplingMethod = SidConfig::INTERPOLATE;
		if ( !sidplay.config( cfg ) )
			return false;

		chn = isStereo ? 2 : 1;
		streams_info += new StreamInfo( srate, chn );

		tune->selectSong( 1 );
		return sidplay.load( tune );
	}
	return false;
}
