#include <Demuxer.hpp>

class Reader;

class Rayman2 : public Demuxer
{
public:
	Rayman2( Module & );
private:
	~Rayman2();

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

	void readHeader( const char *data );

	Reader *reader;
	QMutex readerMutex;

	bool aborted;

	double len;
	unsigned srate;
	unsigned short chn;
	int predictor[ 2 ];
	short stepIndex[ 2 ];
};

#define Rayman2Name "Rayman2 Audio"
