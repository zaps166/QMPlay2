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
	len = sets().getInt( "DefaultLength" );
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
QList<QMPlay2Tag> GME::tags() const
{
	return m_tags;
}
double GME::length() const
{
	return len;
}
int GME::bitrate() const
{
	return -1;
}

bool GME::seek( int s, bool )
{
	return !gme_seek( gme, s * 1000 );
}
bool GME::read( Packet &decoded, int &idx )
{
	if ( aborted || gme_track_ended( gme ) )
		return false;

	const int chunkSize = 1024 * 2; //Always stereo

	decoded.resize( chunkSize * sizeof( float ) );
	int16_t *srcData = ( int16_t * )decoded.data();
	float *dstData = ( float * )decoded.data();

	if ( gme_play( gme, chunkSize, srcData ) != NULL )
		return false;

	for ( int i = chunkSize - 1 ; i >= 0 ; --i )
		dstData[ i ] = srcData[ i ] / 32768.0;

	decoded.ts = gme_tell( gme ) / 1000.0;
	decoded.duration = chunkSize / 2 / ( double )srate;

	idx = 0;

	return true;
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


		gme_info_t *info = NULL;
		if ( !gme_track_info( gme, &info, 0 ) && info )
		{
			if ( info->length > -1 )
				len = info->play_length / 1000.0;
			else if ( info->intro_length > -1 && info->loop_length > -1 )
				len = ( info->intro_length * info->loop_length * 2 ) / 1000.0;

			if ( *info->game )
				m_tags += qMakePair( QString::number( QMPLAY2_TAG_TITLE ), info->game );
			if ( *info->author )
				m_tags += qMakePair( QString::number( QMPLAY2_TAG_ARTIST ), info->author );
			if ( *info->system )
				m_tags += qMakePair( tr( "System" ), info->system );
			if ( *info->copyright )
				m_tags += qMakePair( tr( "Prawo autorskie" ), info->copyright );
			if ( *info->comment )
				m_tags += qMakePair( tr( "Komentarz" ), info->comment );

			gme_free_info( info );
		}

		streams_info += new StreamInfo( srate, 2 );

		return !gme_start_track( gme, 0 );
	}
	return false;
}
