#ifndef MPRIS2_HPP
#define MPRIS2_HPP

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>

#include <time.h>

class MediaPlayer2Root : public QDBusAbstractAdaptor
{
	Q_OBJECT

	Q_CLASSINFO( "D-Bus Interface", "org.mpris.MediaPlayer2" )

	Q_PROPERTY( bool CanQuit READ canQuit )
	Q_PROPERTY( bool CanRaise READ canRaise )
	Q_PROPERTY( bool CanSetFullscreen READ canSetFullscreen )
	Q_PROPERTY( bool Fullscreen READ isFullscreen WRITE setFullscreen )
	Q_PROPERTY( bool HasTrackList READ hasTrackList )
	Q_PROPERTY( QString Identity READ identity )
	Q_PROPERTY( QStringList SupportedMimeTypes READ supportedMimeTypes )
	Q_PROPERTY( QStringList SupportedUriSchemes READ supportedUriSchemes )
public:
	MediaPlayer2Root( QObject *p );

	bool canQuit() const;
	bool canRaise() const;
	bool canSetFullscreen() const;
	bool isFullscreen() const;
	void setFullscreen( bool fs );
	bool hasTrackList() const;
	QString identity() const;
	QStringList supportedMimeTypes() const;
	QStringList supportedUriSchemes() const;
public slots:
	void Quit();
	void Raise();
private slots:
	void fullScreenChanged( bool fs );
private:
	bool fullScreen;
};

/**/

class MediaPlayer2Player : public QDBusAbstractAdaptor
{
	Q_OBJECT

	Q_CLASSINFO( "D-Bus Interface", "org.mpris.MediaPlayer2.Player" )

	Q_PROPERTY( bool CanControl READ canControl )
	Q_PROPERTY( bool CanGoNext READ canGoNext )
	Q_PROPERTY( bool CanGoPrevious READ canGoPrevious )
	Q_PROPERTY( bool CanPause READ canPause )
	Q_PROPERTY( bool CanPlay READ canPlay )
	Q_PROPERTY( bool CanSeek READ canSeek )
	Q_PROPERTY( QVariantMap Metadata READ metadata )
	Q_PROPERTY( QString PlaybackStatus READ playbackStatus )
	Q_PROPERTY( qint64 Position READ position )
	Q_PROPERTY( double MinimumRate READ minimumRate )
	Q_PROPERTY( double MaximumRate READ maximumRate )
	Q_PROPERTY( double Rate READ rate WRITE setRate )
	Q_PROPERTY( double Volume READ volume WRITE setVolume )
public:
	MediaPlayer2Player( QObject *p );
	~MediaPlayer2Player();

	inline void setExportCovers( bool e )
	{
		exportCovers = e;
	}

	bool canControl() const;
	bool canGoNext() const;
	bool canGoPrevious() const;
	bool canPause() const;
	bool canPlay() const;
	bool canSeek() const;

	QVariantMap metadata() const;
	QString playbackStatus() const;
	qint64 position() const;

	double minimumRate() const;
	double maximumRate() const;
	double rate() const;
	void setRate( double rate );

	double volume() const;
	void setVolume( double value );
public slots:
	void Next();
	void Previous();
	void Pause();
	void PlayPause();
	void Stop();
	void Play();
	void Seek( qint64 Offset );
	void SetPosition( const QDBusObjectPath &TrackId, qint64 Position );
	void OpenUri( const QString &Uri );
signals:
	void Seeked( qint64 Position );
private slots:
	void updatePlaying( bool play, const QString &title, const QString &artist, const QString &album, int length, bool needCover, const QString &fileName );
	void coverDataFromMediaFile( const QByteArray &cover );
	void playStateChanged( const QString &plState );
	void coverFile( const QString &filePath );
	void speedChanged( double speed );
	void volumeChanged( double v );
	void posChanged( int p );
	void seeked( int pos );
private:
	void clearMetaData();

	bool exportCovers, removeCover;

	QDBusObjectPath trackID;
	QVariantMap m_data;
	QString playState;
	bool can_seek;
	double vol, r;
	qint64 len, pos;
};

/**/

class MPRIS2Interface : public QObject
{
public:
	MPRIS2Interface( time_t instance_val );
	~MPRIS2Interface();

	inline void setExportCovers( bool e )
	{
		mediaPlayer2Player.setExportCovers( e );
	}
private:
	QString service;
	MediaPlayer2Root mediaPlayer2Root;
	MediaPlayer2Player mediaPlayer2Player;
};

/**/

#include <QMPlay2Extensions.hpp>

class MPRIS2 : public QMPlay2Extensions
{
public:
	MPRIS2( Module &module );
	~MPRIS2();
private:
	bool set();

	MPRIS2Interface *mpris2Interface;
	time_t instance_val;
};

#define MPRIS2Name "MPRIS2"

#endif // MPRIS2_HPP
