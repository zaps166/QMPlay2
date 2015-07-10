#ifndef AUDIOCD_HPP
#define AUDIOCD_HPP

#include <Demuxer.hpp>
#include <Playlist.hpp>

#include <QCoreApplication>

#include <cdio/cdio.h>
#include <cddb/cddb.h>

class CDIODestroyTimer : public QObject, private QMutex
{
	Q_OBJECT
public:
	CDIODestroyTimer();
	~CDIODestroyTimer();

	void setInstance( CdIo_t *cdio, const QString &device, unsigned discID );
	CdIo_t *getInstance( const QString &device, unsigned &discID );
signals:
	void startTimerSig( CdIo_t *cdio, const QString &device, unsigned discID );
private slots:
	void startTimerSlot( CdIo_t *cdio, const QString &device, unsigned discID );
private:
	void timerEvent( QTimerEvent *e );

	int timerID;
	CdIo_t *cdio;
	QString device;
	unsigned discID;
};

/**/

class AudioCDDemux : public Demuxer
{
	friend class AudioCD;
	Q_DECLARE_TR_FUNCTIONS( AudioCDDemux )
public:
	AudioCDDemux( Module &, CDIODestroyTimer &destroyTimer, const QString &AudioCDPlaylist = QString() );
private:
	~AudioCDDemux();

	bool set();

	QString name() const;
	QString title() const;
	QList< QMPlay2Tag > tags() const;
	double length() const;
	int bitrate() const;

	bool seek( int, bool );
	bool read( Packet &, int &  );
	void abort();

	bool open( const QString & );

	/**/

	void readCDText( track_t trackNo );

	bool freedb_query( cddb_disc_t *&cddb_disc );
	void freedb_get_disc_info( cddb_disc_t *cddb_disc );
	void freedb_get_track_info( cddb_disc_t *cddb_disc );

	const QString &AudioCDPlaylist;
	CDIODestroyTimer &destroyTimer;

	QList< Playlist::Entry > getTracks( const QString &device );
	QStringList getDevices();

	QString Title, Artist, Genre, cdTitle, cdArtist, device;
	CdIo_t *cdio;
	track_t trackNo, numTracks;
	lsn_t startSector, numSectors, sector;
	double duration;
	bool isData, aborted, useCDDB, useCDTEXT;
	unsigned char chn;
	unsigned discID;
};

#define AudioCDName "AudioCD"

#endif
