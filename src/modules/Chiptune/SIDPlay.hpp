#include <Demuxer.hpp>

#include <sidplayfp/builders/residfp.h>
#include <sidplayfp/sidplayfp.h>

class SidTuneInfo;
class Reader;

class SIDPlay : public Demuxer
{
	Q_DECLARE_TR_FUNCTIONS( SIDPlay )
public:
	SIDPlay( Module & );
	~SIDPlay();
private:
	bool set();

	QString name() const;
	QString title() const;
	QList< QMPlay2Tag > tags() const;
	double length() const;
	int bitrate() const;

	bool seek( int, bool );
	bool read( Packet &, int & );
	void abort();

	bool open( const QString & );

	Playlist::Entries fetchTracks( const QString &url, bool &ok );


	bool open( const QString &url, bool tracksOnly );

	QString getTitle( const SidTuneInfo *info, int track ) const;


	IOController< Reader > m_reader;
	quint32 m_srate;
	bool m_aborted;
	int m_length;
	quint8 m_chn;

	QList< QMPlay2Tag > m_tags;
	QString m_url, m_title;

	ReSIDfpBuilder m_rs;
	sidplayfp m_sidplay;
	SidTune *m_tune;
};

#define SIDPlayName "SIDPlay"
