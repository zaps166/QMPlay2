#include <AudioCDDemux.hpp>

#ifdef Q_OS_WIN
	#include <QDir>
#else
	#include <QFileInfo>
#endif
#include <QUrl>
#if QT_VERSION < 0x050000
	#define QUrlQuery( url ) url
#else
	#include <QUrlQuery>
#endif

#define CD_BLOCKSIZE 2352/2
#define srate 44100

CDIODestroyTimer::CDIODestroyTimer() :
	QMutex( QMutex::Recursive ),
	timerID( 0 ),
	discID( 0 )
{
	connect( this, SIGNAL( startTimerSig( CdIo_t *, const QString &, unsigned ) ), this, SLOT( startTimerSlot( CdIo_t *, const QString &, unsigned ) ) );
}
CDIODestroyTimer::~CDIODestroyTimer()
{
	lock();
	if ( timerID )
		cdio_destroy( cdio );
	unlock();
}

void CDIODestroyTimer::setInstance( CdIo_t *cdio, const QString &device, unsigned discID )
{
	if ( tryLock() )
		emit startTimerSig( cdio, device, discID );
	else
		cdio_destroy( cdio );
}
CdIo_t *CDIODestroyTimer::getInstance( const QString &device, unsigned &discID )
{
	lock();
	if ( timerID )
	{
		timerID = 0;
		if ( device == this->device )
		{
			discID = this->discID;
			unlock();
			return cdio;
		}
		cdio_destroy( cdio );
	}
	unlock();
	return NULL;
}

void CDIODestroyTimer::startTimerSlot( CdIo_t *cdio, const QString &device, unsigned discID )
{
	if ( timerID )
	{
		cdio_destroy( this->cdio );
		timerID = 0;
	}
	if ( !( timerID = startTimer( 2500 ) ) )
		cdio_destroy( cdio );
	else
	{
		this->cdio = cdio;
		this->device = device;
		this->discID = discID;
	}
	unlock();
}

void CDIODestroyTimer::timerEvent( QTimerEvent *e )
{
	lock();
	if ( e->timerId() == timerID )
	{
		cdio_destroy( cdio );
		timerID = 0;
	}
	killTimer( e->timerId() );
	unlock();
}

/**/

AudioCDDemux::AudioCDDemux( Module &module, CDIODestroyTimer &destroyTimer, const QString &AudioCDPlaylist ) :
	AudioCDPlaylist( AudioCDPlaylist ), destroyTimer( destroyTimer ), cdio( NULL ), sector( 0 ), aborted( false ), discID( 0 )
{
	SetModule( module );
}

AudioCDDemux::~AudioCDDemux()
{
	if ( cdio )
		destroyTimer.setInstance( cdio, device, discID );
}

bool AudioCDDemux::set()
{
	useCDDB   = sets().getBool( "AudioCD/CDDB" );
	useCDTEXT = sets().getBool( "AudioCD/CDTEXT" );
	return true;
}

QString AudioCDDemux::name() const
{
	if ( !cdTitle.isEmpty() && !cdArtist.isEmpty() )
		return AudioCDName" [" + cdArtist + " - " + cdTitle + "]";
	else if ( !cdTitle.isEmpty() )
		return AudioCDName" [" + cdTitle + "]";
	else if ( !cdArtist.isEmpty() )
		return AudioCDName" [" + cdArtist + "]";
	return AudioCDName;
}
QString AudioCDDemux::title() const
{
	QString prefix, suffix;
	if ( isData )
		suffix = " - " + tr( "Dane" );
	else if ( !Title.isEmpty() && !Artist.isEmpty() )
		return Artist + " - " + Title;
	else if ( !Title.isEmpty() )
		return Title;
	else if ( !Artist.isEmpty() )
		prefix = Artist + " - ";
	return prefix + tr( "Ścieżka" ) + " " + QString::number( trackNo ) + suffix;
}
QList< QMPlay2Tag > AudioCDDemux::tags() const
{
	QList< QMPlay2Tag > tagList;
	if ( !Title.isEmpty() )
		tagList << qMakePair( QString::number( QMPLAY2_TAG_TITLE ), Title );
	if ( !Artist.isEmpty() )
		tagList << qMakePair( QString::number( QMPLAY2_TAG_ARTIST ), Artist );
	if ( !cdTitle.isEmpty() )
		tagList << qMakePair( QString::number( QMPLAY2_TAG_ALBUM ), cdTitle );
	if ( !Genre.isEmpty() )
		tagList << qMakePair( QString::number( QMPLAY2_TAG_GENRE ), Genre );
	return tagList;
}
double AudioCDDemux::length() const
{
	return numSectors * duration;
}
int AudioCDDemux::bitrate() const
{
	return 8 * ( srate * chn * 2 ) / 1000;
}

bool AudioCDDemux::seek( int s, bool )
{
	return ( sector = s / duration ) < numSectors;
}
bool AudioCDDemux::read( QByteArray &decoded, int &idx, TimeStamp &ts, double &duration )
{
	if ( aborted || numSectors <= sector || isData )
		return false;

	short cd_samples[ CD_BLOCKSIZE ];
	if ( cdio_read_audio_sector( cdio, cd_samples, startSector + sector ) == DRIVER_OP_SUCCESS )
	{
		decoded.resize( CD_BLOCKSIZE * sizeof( float ) );
		float *decoded_data = ( float * )decoded.data();
		for ( int i = 0 ; i < CD_BLOCKSIZE ; ++i )
			decoded_data[ i ] = cd_samples[ i ] / 32768.0f;

		idx = 0;
		duration = this->duration;
		ts = sector * duration;

		++sector;
		return true;
	}

	return false;
}
void AudioCDDemux::abort()
{
	aborted = true;
}

bool AudioCDDemux::open( const QString &_url )
{
#ifdef Q_OS_WIN
	if ( _url.toLower().contains( QRegExp( "file://\\D:/track\\d\\d.cda" ) ) )
	{
		QString url = _url;
		url.remove( "file://" );
		device = url.mid( 0, url.indexOf( '/' ) );
		trackNo = url.mid( url.toLower().indexOf( "track" ) + 5, 2 ).toUInt();
	}
	else
#endif
	{
		if ( _url.left( 10 ) != "AudioCD://" )
			return false;
		QUrl url( _url );
		device = QUrlQuery( url ).queryItemValue( "device" );
		trackNo = url.host().toUInt();
	}
	if ( trackNo > 0 && trackNo < CDIO_INVALID_TRACK )
	{
		cdio = destroyTimer.getInstance( device, discID );
		if ( cdio || ( cdio = cdio_open( device.toLocal8Bit(), DRIVER_UNKNOWN ) ) )
		{
			cdio_set_speed( cdio, 1 );
			numTracks = cdio_get_num_tracks( cdio );
			if ( cdio_get_discmode( cdio ) != CDIO_DISC_MODE_ERROR && numTracks > 0 && numTracks != CDIO_INVALID_TRACK )
			{
				chn = cdio_get_track_channels( cdio, trackNo );
				if ( numTracks >= trackNo && ( chn == 2 || chn == 4 ) )
				{
					if ( useCDTEXT )
					{
						readCDText( 0 );
						readCDText( trackNo );
					}
					isData = cdio_get_track_format( cdio, trackNo ) != TRACK_FORMAT_AUDIO;
					duration = CD_BLOCKSIZE / chn / ( double )srate;
					startSector = cdio_get_track_lsn( cdio, trackNo );
					numSectors = cdio_get_track_last_lsn( cdio, trackNo ) - startSector;

					if ( useCDDB && Title.isEmpty() )
					{
						cddb_disc_t *cddb_disc;
						if ( freedb_query( cddb_disc ) )
						{
							if ( cdTitle.isEmpty() && cdArtist.isEmpty() )
								freedb_get_disc_info( cddb_disc );
							freedb_get_track_info( cddb_disc );
							cddb_disc_destroy( cddb_disc );
						}
					}

					StreamInfo *streamInfo = new StreamInfo;
					streamInfo->type = QMPLAY2_TYPE_AUDIO;
					streamInfo->is_default = true;
					streamInfo->sample_rate = srate;
					streamInfo->channels = chn;
					streams_info += streamInfo;

					return true;
				}
				else
					QMPlay2Core.log( tr( "Błąd odczytu ścieżki" ) );
			}
			else
				QMPlay2Core.log( tr( "Brak płyty w napędzie" ) );
		}
		else
			QMPlay2Core.log( tr( "Nieprawidłowa ścieżka do napędu CD" ) );
	}
	else //dodawanie do listy ścieżek AudioCD
	{
#ifndef Q_OS_WIN
		device = QUrl( _url ).path();
#else
		device = _url.mid( strlen( AudioCDName"://" ), 2 );
#endif
#ifndef Q_OS_WIN
		if ( !QFileInfo( device ).isDir() )
#endif
			if ( !device.isEmpty() )
			{
				emit QMPlay2Core.processParam( "DelPlaylistEntries", _url );
				QList< Playlist::Entry > entries = getTracks( device );
				if ( !entries.isEmpty() && Playlist::write( entries, "file://" + AudioCDPlaylist ) )
				{
					emit QMPlay2Core.processParam( "open", AudioCDPlaylist );
					return true;
				}
			}
	}
	return false;
}

void AudioCDDemux::readCDText( track_t trackNo )
{
#if LIBCDIO_VERSION_NUM >= 84
	if ( cdtext_t *cdtext = cdio_get_cdtext( cdio ) )
	{
		if ( trackNo == 0 )
		{
			cdTitle  = cdtext_get_const( cdtext, CDTEXT_FIELD_TITLE, 0 );
			cdArtist = cdtext_get_const( cdtext, CDTEXT_FIELD_PERFORMER, 0 );
		}
		else
		{
			Title  = cdtext_get_const( cdtext, CDTEXT_FIELD_TITLE, trackNo );
			Artist = cdtext_get_const( cdtext, CDTEXT_FIELD_PERFORMER, trackNo );
			Genre  = cdtext_get_const( cdtext, CDTEXT_FIELD_GENRE, trackNo );
		}
	}
#else
	if ( cdtext_t *cdtext = cdio_get_cdtext( cdio, trackNo ) )
	{
		if ( trackNo == 0 )
		{
			cdTitle  = cdtext_get_const( CDTEXT_TITLE, cdtext );
			cdArtist = cdtext_get_const( CDTEXT_PERFORMER, cdtext );
		}
		else
		{
			Title  = cdtext_get_const( CDTEXT_TITLE, cdtext );
			Artist = cdtext_get_const( CDTEXT_PERFORMER, cdtext );
			Genre  = cdtext_get_const( CDTEXT_GENRE, cdtext );
		}
	}
#endif
}

bool AudioCDDemux::freedb_query( cddb_disc_t *&cddb_disc )
{
#ifdef Q_OS_WIN
	bool hasHomeEnv = getenv( "HOME" );
	if ( !hasHomeEnv )
		putenv( "HOME=" + QDir::homePath().toLocal8Bit() );
#endif
	cddb_conn_t *cddb = cddb_new();
#ifdef Q_OS_WIN
	if ( !hasHomeEnv )
		putenv( "HOME=" );
#endif
	cddb_disc = cddb_disc_new();

#ifdef Q_OS_WIN
	const QString cddbDir = QMPlay2Core.getSettingsDir() + "CDDB";
	if ( QDir( cddbDir ).exists() || QDir( QMPlay2Core.getSettingsDir() ).mkdir( "CDDB" ) )
		cddb_cache_set_dir( cddb, cddbDir.toLocal8Bit() );
#endif

	cddb_disc_set_length( cddb_disc, FRAMES_TO_SECONDS( cdio_get_track_lba( cdio, CDIO_CDROM_LEADOUT_TRACK ) ) );
	for ( int trackno = 1 ; trackno <= numTracks ; ++trackno )
	{
		cddb_track_t *pcddb_track = cddb_track_new();
		cddb_track_set_frame_offset( pcddb_track, cdio_get_track_lba( cdio, trackno ) );
		cddb_disc_add_track( cddb_disc, pcddb_track );
	}

	bool useNetwork = false;

	cddb_disc_calc_discid( cddb_disc );
	if ( cddb_disc_get_discid( cddb_disc ) == discID )
		cddb_cache_only( cddb );
	else
	{
		discID = cddb_disc_get_discid( cddb_disc );

		cddb_set_timeout( cddb, 3 );
		cddb_http_enable( cddb );
		cddb_set_server_port( cddb, 80 );

		Settings sets( "QMPlay2" );
		if ( sets.getBool( "Proxy/Use" ) )
		{
			cddb_http_proxy_enable( cddb );
			cddb_set_http_proxy_server_name( cddb, sets.getString( "Proxy/Host" ).toLocal8Bit() );
			cddb_set_http_proxy_server_port( cddb, sets.getUInt( "Proxy/Port" ) );
			if ( sets.getBool( "Proxy/Login" ) )
			{
				cddb_set_http_proxy_username( cddb, sets.getString( "Proxy/User" ).toLocal8Bit() );
				cddb_set_http_proxy_password( cddb, QString( QByteArray::fromBase64( sets.getByteArray( "Proxy/Password" ) ) ).toLocal8Bit() );
			}
		}

		useNetwork = true;
	}

	for ( int i = 0 ; i <= useNetwork ; ++i )
	{
		if ( cddb_query( cddb, cddb_disc ) > 0 )
		{
			do if ( cddb_disc_get_discid( cddb_disc ) == discID )
			{
				cddb_read( cddb, cddb_disc );
				cddb_destroy( cddb );
				return true;
			} while ( cddb_query_next( cddb, cddb_disc ) );
		}
		if ( useNetwork && !i )
			cddb_set_server_name( cddb, "freedb.musicbrainz.org" );
	}

	cddb_disc_destroy( cddb_disc );
	cddb_destroy( cddb );
	cddb_disc = NULL;
	return false;
}
void AudioCDDemux::freedb_get_disc_info( cddb_disc_t *cddb_disc )
{
	if ( cddb_disc )
	{
		cdTitle  = cddb_disc_get_title( cddb_disc );
		cdArtist = cddb_disc_get_artist( cddb_disc );
	}
}
void AudioCDDemux::freedb_get_track_info( cddb_disc_t *cddb_disc )
{
	cddb_track_t *cddb_track;
	if ( cddb_disc && ( cddb_track = cddb_disc_get_track( cddb_disc, trackNo - 1 ) ) )
	{
		Title  = cddb_track_get_title( cddb_track );
		Artist = cddb_track_get_artist( cddb_track );
	}
}

QList< Playlist::Entry > AudioCDDemux::getTracks( const QString &_device )
{
	QList< Playlist::Entry > tracks;
	Playlist::Entry entry;
	device = _device;
	cdio_close_tray( device.toLocal8Bit(), NULL );
	if ( ( cdio = cdio_open( device.toLocal8Bit(), DRIVER_UNKNOWN ) ) )
	{
		numTracks = cdio_get_num_tracks( cdio );
		if ( cdio_get_discmode( cdio ) != CDIO_DISC_MODE_ERROR && numTracks > 0 && numTracks != CDIO_INVALID_TRACK )
		{
			cddb_disc_t *cddb_disc = NULL;
			bool cddb_ok = useCDDB;
			for ( trackNo = 1 ; trackNo <= numTracks ; ++trackNo )
			{
				chn = cdio_get_track_channels( cdio, trackNo );
				if ( chn != 2 && chn != 4 )
					continue;

				if ( useCDTEXT )
					readCDText( trackNo );
				isData = cdio_get_track_format( cdio, trackNo ) != TRACK_FORMAT_AUDIO;
				duration = CD_BLOCKSIZE / chn / ( double )srate;
				numSectors = cdio_get_track_last_lsn( cdio, trackNo ) - cdio_get_track_lsn( cdio, trackNo );

				if ( cddb_ok && ( cddb_disc || ( Title.isEmpty() && ( cddb_ok = freedb_query( cddb_disc ) ) ) ) )
					freedb_get_track_info( cddb_disc );

				entry.name = title();
				entry.url = AudioCDName"://" + QString::number( trackNo ) + "?device=" + device;
				entry.length = length();

				tracks += entry;
			}
			cddb_disc_destroy( cddb_disc );
		}
	}
	return tracks;
}
QStringList AudioCDDemux::getDevices()
{
	QStringList devicesList;
	if ( char **devices = cdio_get_devices( DRIVER_DEVICE ) )
	{
		for ( size_t i = 0 ; char *device = devices[ i ] ; ++i )
		{
			devicesList += device;
#ifdef Q_OS_WIN
			devicesList.last().remove( 0, 4 );
#endif
		}
		cdio_free_device_list( devices );
	}
	return devicesList;
}
