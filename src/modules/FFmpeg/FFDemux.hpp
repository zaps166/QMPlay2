#include <Demuxer.hpp>

class FormatContext;

class FFDemux : public Demuxer
{
	Q_DECLARE_TR_FUNCTIONS( FFDemux )
public:
	FFDemux( QMutex &, Module & );
private:
	~FFDemux();

	bool set();

	bool metadataChanged() const;

	QList< ChapterInfo > getChapters() const;

	QString name() const;
	QString title() const;
	QList< QMPlay2Tag > tags() const;
	bool getReplayGain( bool album, float &gain_db, float &peak ) const;
	double length() const;
	int bitrate() const;
	QByteArray image( bool ) const;

	bool localStream() const;

	bool seek( int );
	bool read( Packet &, int & );
	void pause();
	void abort();

	bool open( const QString & );

	/**/

	QVector< FormatContext * > formatContexts;

	QMutex &avcodec_mutex;
};
