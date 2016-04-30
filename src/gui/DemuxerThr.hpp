#ifndef DEMUXERTHR_HPP
#define DEMUXERTHR_HPP

#include <IOController.hpp>
#include <StreamInfo.hpp>

#include <QString>
#include <QThread>
#include <QMutex>

class BufferInfo;
class PlayClass;
class AVThread;
class Demuxer;
class BasicIO;

class DemuxerThr : public QThread
{
	friend class PlayClass;
	Q_OBJECT
public:
	DemuxerThr(PlayClass &);

	QByteArray getCoverFromStream() const;

	inline bool isDemuxerReady() const
	{
		return demuxerReady;
	}
	inline bool canSeek() const
	{
		return !unknownLength;
	}

	void loadImage();

	void seek(bool doDemuxerSeek);

	void stop();
	void end();

	void emitInfo();
private:
	bool load(bool canEmitInfo = true);

	void run();

	void updateCoverAndPlaying();

	void addSubtitleStream(bool, QString &, int, int, const QString &, const QString &, const QString &, const QList<QMPlay2Tag> &other_info = QList<QMPlay2Tag>());

	bool mustReloadStreams();
	bool bufferedAllPackets(int vS, int aS, int p);
	bool emptyBuffers(int vS, int aS);
	bool canBreak(const AVThread *avThr1, const AVThread *avThr2);
	void getAVBuffersSize(int &vS, int &aS, BufferInfo *bufferInfo = NULL);
	void clearBuffers();

	double getFrameDelay() const;

	PlayClass &playC;

	QString name, url, updatePlayingName;

	int minBuffSizeLocal, minBuffSizeNetwork;
	bool err, updateBufferedSeconds, demuxerReady, hasCover, skipBufferSeek, localStream, unknownLength;
	QMutex stopVAMutex, endMutex, seekMutex;
	IOController<> ioCtrl;
	IOController<Demuxer> demuxer;
	QString title, artist, album;
	double playIfBuffered, time, updateBufferedTime;
private slots:
	void stopVADec();
	void updateCover(const QString &title, const QString &artist, const QString &album, const QByteArray &cover);
signals:
	void load(Demuxer *);
};

#endif //DEMUXERTHR_HPP
