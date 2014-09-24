#include <Demuxer.hpp>

#include <QCoreApplication>

class Reader;
struct _ModPlugFile;

class MPDemux : public Demuxer
{
	Q_DECLARE_TR_FUNCTIONS( MPDemux )
public:
	MPDemux( Module & );
private:
	~MPDemux();

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
	double pos;
	_ModPlugFile *mpfile;
};

#define DemuxerName "Modplug Demuxer"
