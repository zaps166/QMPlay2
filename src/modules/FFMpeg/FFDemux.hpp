#include <Demuxer.hpp>

struct AVFormatContext;
struct AVDictionary;
struct AVStream;
class QMutex;

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

	bool seek( int, bool );
	bool read( QByteArray &, int &, TimeStamp &, double & );
	void pause();
	void abort();

	bool open( const QString & );

	/**/

	StreamInfo *getStreamInfo( AVStream *stream ) const;
	AVDictionary *getMetadata() const;
private:
	QVector< int > index_map;
	QList< AVStream * > streams;
	AVFormatContext *formatCtx;
	TimeStamp lastTS;

	bool isLocal, paused, isStreamed, aborted, fix_mkv_ass;
	mutable bool isMetadataChanged;
	double lastTime, start_time;
	qint64 seekByByteOffset;

	int lastErr;

#if LIBAVFORMAT_VERSION_MAJOR <= 55
	AVDictionary *metadata;
#endif

	QMutex &avcodec_mutex;
};
