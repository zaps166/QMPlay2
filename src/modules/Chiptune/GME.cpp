#include <GME.hpp>

#include <Functions.hpp>
#include <Reader.hpp>
#include <Packet.hpp>

#include <QDebug>

#include <gme/gme.h>

GME::GME( Module &module ) :
	srate( Functions::getBestSampleRate() ),
	aborted( false ),
	gme( NULL )
{
	SetModule( module );
}
GME::~GME()
{
	gme_delete( gme );
}

bool GME::set()
{
	return sets().getBool( "GME" );
}

QString GME::name() const
{
	return gme_type_system( gme_type( gme ) );
}
QString GME::title() const
{
	return QString();
}
double GME::length() const
{
	return -1;
}
int GME::bitrate() const
{
	return -1;
}

bool GME::seek( int s, bool )
{
	Q_UNUSED( s )
	return false;
}
bool GME::read( Packet &decoded, int &idx )
{
	if ( aborted )
		return false;

	const int chunkSize = 1024 * 2;

	decoded.resize( chunkSize * sizeof( float ) );
	int16_t *srcData = ( int16_t * )decoded.data();
	float *dstData = ( float * )decoded.data();

	gme_play( gme, chunkSize, srcData );
	for ( int i = chunkSize - 1 ; i >= 0 ; --i )
		dstData[ i ] = srcData[ i ] / 32768.0;

	decoded.ts = 0.0;
	decoded.duration = decoded.size() / 2 / sizeof( float ) / ( double )srate;

	idx = 0;

	return !gme_track_ended( gme );
}
void GME::abort()
{
	reader.abort();
	aborted = true;
}

bool GME::open( const QString &url )
{
	if ( Reader::create( url, reader ) )
	{
		const QByteArray data = reader->read( reader->size() );
		reader.clear();

		gme_open_data( data.data(), data.size(), &gme, srate );
		if ( !gme )
			return false;

		gme_start_track( gme, 0 );

		StreamInfo *streamInfo = new StreamInfo;
		streamInfo->type = QMPLAY2_TYPE_AUDIO;
		streamInfo->is_default = true;
		streamInfo->sample_rate = srate;
		streamInfo->channels = 2;
		streams_info += streamInfo;

		return true;
	}
	return false;
}
