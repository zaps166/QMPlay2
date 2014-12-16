#ifndef DEMUXERTHR_HPP
#define DEMUXERTHR_HPP

#include <StreamInfo.hpp>

#include <QString>
#include <QThread>
#include <QMutex>
#include <QImage>

class PlayClass;
class AVThread;
class Demuxer;
class Reader;

class DemuxerThr : public QThread
{
	friend class PlayClass;
	Q_OBJECT
public:
	DemuxerThr( PlayClass & );

	QByteArray getCoverFromStream() const;

	void loadImage();

	void stop();
	void end();

	void emitInfo();
private:
	bool load( bool canEmitInfo = true );

	void run();

	void updateCoverAndPlaying();

	void addSubtitleStream( bool, QString &, int, int, const QString &, const QString &, const QString &, const QList< QMPlay2Tag > &other_info = QList< QMPlay2Tag >() );

	bool mustReloadStreams();
	bool bufferedPackets( int, int, int );
	bool emptyBuffers( int, int );
	bool canBreak( const AVThread *avThr1, const AVThread *avThr2 );
	void getAVBuffersSize( int &vS, int &aS, qint64 *buffered = NULL, double *backwardTime = NULL, double *bufferedTime = NULL );
	void clearBuffers();

	PlayClass &playC;
	Demuxer *demuxer;

	QString name, url, updatePlayingName;

	int minBuffSizeLocal, minBuffSizeNetwork;
	bool err, updateBufferedSeconds, br, demuxerReady, hasCover;
	QMutex stopVAMutex, endMutex, abortMutex;
	Reader *convertAddressReader;
	QString title, artist, album;
	double playIfBuffered;
private slots:
	void stopVADecSlot();
	void updateCover( const QString &title, const QString &artist, const QString &album, const QByteArray &cover );
signals:
	void load( Demuxer * );
	void stopVADec();
};

#endif //DEMUXERTHR_HPP
