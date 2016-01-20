#ifndef AUDIOCD_HPP
#define AUDIOCD_HPP

#include <Demuxer.hpp>
#include <Playlist.hpp>

#include <QAtomicInt>

#include <cdio/cdio.h>
#include <cddb/cddb.h>

class CDIODestroyTimer : public QObject
{
	Q_OBJECT
public:
	CDIODestroyTimer();
	~CDIODestroyTimer();

	Q_SIGNAL void setInstance(CdIo_t *_cdio, const QString &_device, unsigned _discID);
	CdIo_t *getInstance(const QString &_device, unsigned &_discID);
private slots:
	void setInstanceSlot(CdIo_t *_cdio, const QString &_device, unsigned _discID);
private:
	void timerEvent(QTimerEvent *e);

	QAtomicInt timerId;
	CdIo_t *cdio;
	QString device;
	unsigned discID;
};

/**/

class AudioCDDemux : public Demuxer
{
	Q_DECLARE_TR_FUNCTIONS(AudioCDDemux)
public:
	static QStringList getDevices();

	AudioCDDemux(Module &, CDIODestroyTimer &destroyTimer);
private:
	~AudioCDDemux();

	bool set();

	QString name() const;
	QString title() const;
	QList< QMPlay2Tag > tags() const;
	double length() const;
	int bitrate() const;

	bool seek(int, bool);
	bool read(Packet &, int & );
	void abort();

	bool open(const QString &);

	Playlist::Entries fetchTracks(const QString &url, bool &ok);

	/**/

	void readCDText(track_t trackNo);

	bool freedb_query(cddb_disc_t *&cddb_disc);
	void freedb_get_disc_info(cddb_disc_t *cddb_disc);
	void freedb_get_track_info(cddb_disc_t *cddb_disc);

	CDIODestroyTimer &destroyTimer;

	Playlist::Entries getTracks(const QString &device);

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
