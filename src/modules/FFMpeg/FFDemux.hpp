#include <Demuxer.hpp>

#include <QNetworkAccessManager>
#include <QTimer>

class QMutex;
class Reader;
struct AVStream;
struct AVDictionary;
struct AVFormatContext;

class FFDemux : public QObject, public Demuxer
{
	Q_OBJECT
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
	bool getReplayGain( bool album, float &gain_db, float &peak );
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
private slots:
	void netInfoTimeout();
	void netDLProgress( qint64 bytesReceived, qint64 bytesTotal );
	void netFinished();
private:
	QVector< int > index_map;
	QList< AVStream * > streams;
	AVFormatContext *formatCtx;

	Reader *reader;
	bool seekByByte, paused, isStreamed, aborted;
	mutable bool isMetadataChanged;
	double lastTime, start_time;

	int lastErr;

	QString netInfoURL, streamTitle, streamGenre, songTitle;
	QTimer netInfoTimer;
	QNetworkAccessManager net;

	QMutex &avcodec_mutex, readerMutex;
};
