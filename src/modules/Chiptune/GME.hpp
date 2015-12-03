#include <Demuxer.hpp>

struct gme_info_t;
struct Music_Emu;
class Reader;

class GME : public Demuxer
{
	Q_DECLARE_TR_FUNCTIONS( GME )
public:
	GME( Module & );
	~GME();
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

	bool hasTracks() const;
	Playlist::Entries fetchTracks();


	QString getTitle( gme_info_t *info, int track ) const;
	int getLength( gme_info_t *info ) const;


	IOController< Reader > m_reader;
	quint32 m_srate;
	bool m_aborted;
	int m_length;

	QList< QMPlay2Tag > m_tags;
	QString m_url, m_title;

	Music_Emu *m_gme;
};

#define GMEName "Game-Music-Emu"
