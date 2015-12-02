#include <Demuxer.hpp>

#include <sidplayfp/builders/residfp.h>
#include <sidplayfp/sidplayfp.h>

class Reader;

class SIDPlay : public Demuxer
{
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

	IOController< Reader > reader;
	quint32 srate;
	quint8 chn;

	bool aborted;
	int len;

	ReSIDfpBuilder rs;
	sidplayfp sidplay;
	SidTune *tune;
};

#define SIDPlayName "SID"
