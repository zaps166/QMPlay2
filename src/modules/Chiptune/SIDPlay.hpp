#include <Demuxer.hpp>

#include <sidplayfp/builders/residfp.h>
#include <sidplayfp/sidplayfp.h>

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
	double length() const;
	int bitrate() const;

	bool dontUseBuffer() const;

	bool seek( int, bool );
	bool read( Packet &, int & );
	void abort();

	bool open( const QString & );

	IOController< Reader > reader;
	quint32 srate;
	quint8 chn;

	bool aborted;

	ReSIDfpBuilder rs;
	sidplayfp sidplay;
	SidTune *tune;
};

#define SIDPlayName "SID"
