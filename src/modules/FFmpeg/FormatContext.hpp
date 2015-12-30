#ifndef FORMATCONTEXT_HPP
#define FORMATCONTEXT_HPP

#include <ChapterInfo.hpp>
#include <StreamInfo.hpp>
#include <TimeStamp.hpp>

#include <QCoreApplication>
#include <QVector>
#include <QMutex>

extern "C"
{
	#include <libavformat/version.h>
}

#if LIBAVFORMAT_VERSION_MAJOR >= 56 && LIBAVFORMAT_VERSION_MINOR >= 36
	#define MP3_FAST_SEEK
#endif

struct AVFormatContext;
struct AVDictionary;
struct AVStream;
struct AVPacket;
struct Packet;

class FormatContext
{
	Q_DECLARE_TR_FUNCTIONS( FormatContext )
public:
	FormatContext( QMutex &avcodec_mutex );
	~FormatContext();

	bool metadataChanged() const;

	QList< ChapterInfo > getChapters() const;

	QString name() const;
	QString title() const;
	QList< QMPlay2Tag > tags() const;
	bool getReplayGain( bool album, float &gain_db, float &peak ) const;
	double length() const;
	int bitrate() const;
	QByteArray image( bool forceCopy ) const;

	bool seek( int pos );
	bool read( Packet &encoded, int &idx );
	void pause();
	void abort();

	bool open( const QString &_url );

	bool isLocal, isStreamed, isError;
	StreamsInfo streamsInfo;
	TimeStamp lastTS;
private:
	StreamInfo *getStreamInfo( AVStream *stream ) const;
	AVDictionary *getMetadata() const;

	QVector< int > index_map;
	QList< AVStream * > streams;
	AVFormatContext *formatCtx;
	AVPacket *packet;

	bool isPaused, isAborted, fixMkvAss;
	mutable bool isMetadataChanged;
	double lastTime, startTime;
#ifndef MP3_FAST_SEEK
	qint64 seekByByteOffset;
#endif
	bool isOneStreamOgg;

	int lastErr;

#if LIBAVFORMAT_VERSION_MAJOR <= 55
	AVDictionary *metadata;
#endif

	QMutex &avcodec_mutex;
};

#endif // FORMATCONTEXT_HPP
