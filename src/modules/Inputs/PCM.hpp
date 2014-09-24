#include <Demuxer.hpp>

class Reader;

class PCM : public Demuxer
{
public:
	enum FORMAT { PCM_U8, PCM_S8, PCM_S16, PCM_S24, PCM_S32, PCM_FLT, FORMAT_COUNT };

	PCM( Module & );
private:
	~PCM();

	bool set();

	QString name() const;
	QString title() const;
	double length() const;
	int bitrate() const;

	bool seek( int, bool );
	bool read( QByteArray &, int &, TimeStamp &, double & );
	void abort();

	bool open( const QString & );

	/**/

	Reader *reader;
	QMutex readerMutex;

	bool aborted;

	double len;
	FORMAT fmt;
	unsigned char chn;
	int srate, offset;
	bool bigEndian;
};

#define PCMName "PCM Audio"
