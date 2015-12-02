#include <Demuxer.hpp>

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

	IOController< Reader > reader;
	quint32 srate;
	bool aborted;
	int len;

	QList< QMPlay2Tag > m_tags;

	Music_Emu *gme;
};

#define GMEName "Game-Music-Emu"
