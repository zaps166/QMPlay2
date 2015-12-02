#include <Demuxer.hpp>

#include <IOController.hpp>

#include <QCoreApplication>

struct _ModPlugFile;
class Reader;

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
	bool read( Packet &, int & );
	void abort();

	bool open( const QString & );

	/**/

	bool aborted;
	double pos;
	quint32 srate;
	_ModPlugFile *mpfile;
	IOController< Reader > reader;
};

#define DemuxerName "Modplug Demuxer"
