#include <MPRIS2.hpp>
#include <QMPlay2Core.hpp>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>

static void propertyChanged( const QString &name, const QVariant &value )
{
	QVariantMap map;
	map.insert( name, value );
	QDBusMessage msg = QDBusMessage::createSignal( "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "PropertiesChanged" );
	msg << "org.mpris.MediaPlayer2.Player" << map << QStringList();
	QDBusConnection::sessionBus().send( msg );
}

/**/

MediaPlayer2Root::MediaPlayer2Root( QObject *p ) :
	QDBusAbstractAdaptor( p ),
	fullScreen( false )
{
	connect( &QMPlay2Core, SIGNAL( fullScreenChanged( bool ) ), this, SLOT( fullScreenChanged( bool ) ) );
}

bool MediaPlayer2Root::canQuit() const
{
	return true;
}
bool MediaPlayer2Root::canRaise() const
{
	return true;
}
bool MediaPlayer2Root::canSetFullscreen() const
{
	return true;
}
bool MediaPlayer2Root::isFullscreen() const
{
	return fullScreen;
}
void MediaPlayer2Root::setFullscreen( bool fs )
{
	if ( fullScreen != fs )
	{
		QMPlay2Core.processParam( "fullscreen" );
		fullScreen = fs;
	}
}
bool MediaPlayer2Root::hasTrackList() const
{
	return false;
}
QString MediaPlayer2Root::identity() const
{
	return qApp->applicationName();
}
QStringList MediaPlayer2Root::supportedMimeTypes() const
{
	return QStringList();
}
QStringList MediaPlayer2Root::supportedUriSchemes() const
{
	return QStringList();
}

void MediaPlayer2Root::Quit()
{
	QMPlay2Core.processParam( "quit" );
}
void MediaPlayer2Root::Raise()
{
	QMPlay2Core.processParam( "show" );
}

void MediaPlayer2Root::fullScreenChanged( bool fs )
{
	propertyChanged( "Fullscreen", fullScreen = fs );

}

/**/

MediaPlayer2Player::MediaPlayer2Player( QObject *p ) :
	QDBusAbstractAdaptor( p ),
	exportCovers( false ), removeCover( false ),
	trackID( QDBusObjectPath( QString( "/org/qmplay2/MediaPlayer2/Track/%1" ).arg( qrand() ) ) ), //is it OK?
	playState( "Stopped" ),
	can_seek( false ),
	vol( 1.0 ), r( 1.0 ),
	len( 0 ), pos( 0 )
{
	clearMetaData();
	m_data[ "mpris:trackid" ] = QVariant::fromValue< QDBusObjectPath >( trackID );
	connect( &QMPlay2Core, SIGNAL( updatePlaying( bool, const QString &, const QString &, const QString &, int, bool, const QString & ) ), this, SLOT( updatePlaying( bool, const QString &, const QString &, const QString &, int, bool, const QString & ) ) );
	connect( &QMPlay2Core, SIGNAL( coverDataFromMediaFile( const QByteArray & ) ), this, SLOT( coverDataFromMediaFile( const QByteArray & ) ) );
	connect( &QMPlay2Core, SIGNAL( playStateChanged( const QString & ) ), this, SLOT( playStateChanged( const QString & ) ) );
	connect( &QMPlay2Core, SIGNAL( coverFile( const QString & ) ), this, SLOT( coverFile( const QString & ) ) );
	connect( &QMPlay2Core, SIGNAL( speedChanged( double ) ), this, SLOT( speedChanged( double ) ) );
	connect( &QMPlay2Core, SIGNAL( volumeChanged( double ) ), this, SLOT( volumeChanged( double ) ) );
	connect( &QMPlay2Core, SIGNAL( posChanged( int ) ), this, SLOT( posChanged( int ) ) );
	connect( &QMPlay2Core, SIGNAL( seeked( int ) ), this, SLOT( seeked( int ) ) );
}
MediaPlayer2Player::~MediaPlayer2Player()
{
	if ( removeCover )
		QFile::remove( m_data[ "mpris:artUrl" ].toString() );
}

bool MediaPlayer2Player::canControl() const
{
	return true;
}
bool MediaPlayer2Player::canGoNext() const
{
	return true;
}
bool MediaPlayer2Player::canGoPrevious() const
{
	return true;
}
bool MediaPlayer2Player::canPause() const
{
	return true;
}
bool MediaPlayer2Player::canPlay() const
{
	return true;
}
bool MediaPlayer2Player::canSeek() const
{
	return can_seek;
}

QVariantMap MediaPlayer2Player::metadata() const
{
	return m_data;
}
QString MediaPlayer2Player::playbackStatus() const
{
	return playState;
}
qint64 MediaPlayer2Player::position() const
{
	return pos;
}

double MediaPlayer2Player::minimumRate() const
{
	return 0.05;
}
double MediaPlayer2Player::maximumRate() const
{
	return 100.0;
}
double MediaPlayer2Player::rate() const
{
	return r;
}
void MediaPlayer2Player::setRate( double rate )
{
	if ( rate >= minimumRate() && rate <= maximumRate() )
		QMPlay2Core.processParam( "speed", QString::number( rate ) );
}

double MediaPlayer2Player::volume() const
{
	return vol;
}
void MediaPlayer2Player::setVolume( double value )
{
	QMPlay2Core.processParam( "volume", QString::number( ( int )( value * 100 ) ) );
}

void MediaPlayer2Player::Next()
{
	QMPlay2Core.processParam( "next" );
}
void MediaPlayer2Player::Previous()
{
	QMPlay2Core.processParam( "prev" );
}
void MediaPlayer2Player::Pause()
{
	if ( playState == "Playing" )
		QMPlay2Core.processParam( "toggle" );
}
void MediaPlayer2Player::PlayPause()
{
	QMPlay2Core.processParam( "toggle" );
}
void MediaPlayer2Player::Stop()
{
	QMPlay2Core.processParam( "stop" );
}
void MediaPlayer2Player::Play()
{
	if ( playState != "Playing" )
		QMPlay2Core.processParam( "toggle" );
}
void MediaPlayer2Player::Seek( qint64 Offset )
{
	if ( Offset != position() && Offset >= 0 && Offset <= m_data[ "mpris:length" ].toLongLong() )
		QMPlay2Core.processParam( "seek", QString::number( Offset / 1000000LL ) );
}
void MediaPlayer2Player::SetPosition( const QDBusObjectPath &TrackId, qint64 Position )
{
	if ( trackID == TrackId )
		Seek( Position );
}
void MediaPlayer2Player::OpenUri( const QString &Uri )
{
	QMPlay2Core.processParam( "open", Uri );
}

void MediaPlayer2Player::updatePlaying( bool play, const QString &title, const QString &artist, const QString &album, int length, bool needCover, const QString &fileName )
{
	Q_UNUSED( needCover )
	bool tmp = play && length > 0;
	if ( tmp != can_seek )
		propertyChanged( "CanSeek", can_seek = tmp );
	if ( !play )
		clearMetaData();
	else
	{
		m_data[ "mpris:length" ] = len = length * 1000000LL;
		if ( title.isEmpty() && artist.isEmpty() )
			m_data[ "xesam:title" ] = fileName;
		else
		{
			m_data[ "xesam:title" ] = title;
			m_data[ "xesam:artist" ] = artist;
		}
		m_data[ "xesam:album" ] = album;
	}
	propertyChanged( "Metadata", m_data );
}
void MediaPlayer2Player::coverDataFromMediaFile( const QByteArray &cover )
{
	if ( exportCovers )
	{
		QFile coverF( QDir::tempPath() + "/QMPlay2." + QString( "%1.%2.mpris2cover" ).arg( getenv( "USER" ) ).arg( time( NULL ) ) );
		if ( coverF.open( QFile::WriteOnly ) )
		{
			coverF.write( cover );
			coverF.close();
			m_data[ "mpris:artUrl" ] = coverF.fileName();
			propertyChanged( "Metadata", m_data );
			removeCover = true;
		}
	}
}
void MediaPlayer2Player::playStateChanged( const QString &plState )
{
	propertyChanged( "PlaybackStatus", playState = plState );
}
void MediaPlayer2Player::coverFile( const QString &filePath )
{
	m_data[ "mpris:artUrl" ] = filePath;
	propertyChanged( "Metadata", m_data );
	removeCover = false;
}
void MediaPlayer2Player::speedChanged( double speed )
{
	propertyChanged( "Rate", r = speed );
}
void MediaPlayer2Player::volumeChanged( double v )
{
	propertyChanged( "Volume", vol = v );
}
void MediaPlayer2Player::posChanged( int p )
{
	pos = p * 1000000LL;
	propertyChanged( "Position", pos );
}
void MediaPlayer2Player::seeked( int pos )
{
	emit Seeked( pos * 1000000LL );
}

void MediaPlayer2Player::clearMetaData()
{
	if ( removeCover )
	{
		QFile::remove( m_data[ "mpris:artUrl" ].toString() );
		removeCover = false;
	}
	m_data[ "mpris:artUrl" ] = m_data[ "xesam:title" ] = m_data[ "xesam:artist" ] = m_data[ "xesam:album" ] = QString();
	m_data[ "mpris:length" ] = qint64();
}

/**/

MPRIS2Interface::MPRIS2Interface( time_t instance_val ) :
	service( QString( "org.mpris.MediaPlayer2.QMPlay2.instance%1" ).arg( instance_val ) ),
	mediaPlayer2Root( this ),
	mediaPlayer2Player( this )
{
	QDBusConnection::sessionBus().registerObject( "/org/mpris/MediaPlayer2", this );
	QDBusConnection::sessionBus().registerService( service );
}
MPRIS2Interface::~MPRIS2Interface()
{
	QDBusConnection::sessionBus().unregisterService( service );
	QDBusConnection::sessionBus().unregisterObject( "/org/mpris/MediaPlayer2" );
}

/**/

MPRIS2::MPRIS2( Module &module ) :
	mpris2Interface( NULL ),
	instance_val( time( NULL ) )
{
	SetModule( module );
}
MPRIS2::~MPRIS2()
{
	delete mpris2Interface;
}

bool MPRIS2::set()
{
	if ( !sets().getBool( "MPRIS2/Enabled" ) )
	{
		delete mpris2Interface;
		mpris2Interface = NULL;
	}
	else if ( !mpris2Interface )
		mpris2Interface = new MPRIS2Interface( instance_val );
	if ( mpris2Interface )
		mpris2Interface->setExportCovers( sets().getBool( "MPRIS2/ExportCovers" ) );
	return true;
}
