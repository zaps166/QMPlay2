#ifndef PLAYCLASS_HPP
#define PLAYCLASS_HPP

#include <PacketBuffer.hpp>

#include <QSize>
#include <QTimer>
#include <QMutex>
#include <QImage>
#include <QObject>
#include <QStringList>
#include <QWaitCondition>

class QMPlay2_OSD;
class DemuxerThr;
class VideoThr;
class AudioThr;
class Demuxer;
class LibASS;

enum { SEEK_NOWHERE = -1, SEEK_STREAM_RELOAD = -2 /* Seeks to current position after stream reload */ };

class PlayClass : public QObject
{
	Q_OBJECT
	friend class DemuxerThr;
	friend class AVThread;
	friend class VideoThr;
	friend class AudioThr;
public:
	PlayClass();

	Q_SLOT void play( const QString & );
	Q_SLOT void stop( bool quitApp = false );
	Q_SLOT void restart();

	void chPos( double, bool updateGUI = true );

	void togglePause();
	void seek( int );
	void chStream( const QString & );
	void setSpeed( double );

	bool isPlaying() const;
#ifdef Q_OS_WIN
	bool isNowPlayingVideo() const;
#endif

	inline QString getUrl() const
	{
		return url;
	}
	inline double getPos() const
	{
		return pos;
	}

	void loadSubsFile( const QString & );

	void messageAndOSD( const QString &, bool onStatusBar = true, double duration = 1.0 );

	bool doSilenceOnStart;
private:
	void speedMessageAndOSD();

	double getARatio();

	void flushAssEvents();
	inline void clearSubtitlesBuffer()
	{
		sPackets.clear();
		flushAssEvents();
	}

	void stopVThr();
	void stopAThr();
	inline void stopAVThr()
	{
		stopVThr();
		stopAThr();
	}

	void stopVDec();
	void stopADec();

	void setFlip();

	void clearPlayInfo();

	DemuxerThr *demuxThr;
	VideoThr *vThr;
	AudioThr *aThr;

	QWaitCondition emptyBufferCond;
	volatile bool fillBufferB, doSilenceBreak;
	QMutex loadMutex, stopPauseMutex;

	PacketBuffer aPackets, vPackets, sPackets;

	double frame_last_pts, frame_last_delay, audio_current_pts, audio_last_delay;
	bool canUpdatePos, paused, waitForData, flushVideo, flushAudio, muted, reload, nextFrameB, endOfStream;
	int seekTo, lastSeekTo, restartSeekTo;
	double vol, replayGain, zoom, pos, skipAudioFrame, videoSync, speed, subtitlesSync, subtitlesScale;
	int flip;

	QString url, newUrl, aRatioName;

	int audioStream, videoStream, subtitlesStream;
	int choosenAudioStream, choosenVideoStream, choosenSubtitlesStream;
	QString choosenAudioLang, choosenSubtitlesLang;

	int Brightness, Saturation, Contrast, Hue;

	double maxThreshold, fps;

	bool quitApp, audioEnabled, videoEnabled, subtitlesEnabled;
	QTimer timTerminate;

#ifdef Q_OS_WIN
	bool firsttimeUpdateCache;
#endif
	LibASS *ass;

	QMutex osd_mutex;
	QMPlay2_OSD *osd;
	int videoWinW, videoWinH;
	QStringList fileSubsList;
	QString fileSubs;
private slots:
	void saveCover();
	void settingsChanged( int, bool );
	void videoResized( int, int );

	void setVideoEqualizer( int, int, int, int );

	void speedUp();
	void slowDown();
	void setSpeed();
	void zoomIn();
	void zoomOut();
	void zoomReset();
	void aRatio();
	void volume( int );
	void toggleMute();
	void slowDownVideo();
	void speedUpVideo();
	void setVideoSync();
	void slowDownSubs();
	void speedUpSubs();
	void setSubtitlesSync();
	void biggerSubs();
	void smallerSubs();
	void toggleAVS( bool );
	void setHFlip( bool );
	void setVFlip( bool );
	void screenShot();
	void nextFrame();

	void aRatioUpdated();

	void demuxThrFinished();

	void timTerminateFinished();

	void load( Demuxer * );
signals:
	void aRatioUpdate();
	void chText( const QString & );
	void updateLength( int );
	void updatePos( int );
	void playStateChanged( bool );
	void setCurrentPlaying();
	void setInfo( const QString &, bool, bool );
	void updateCurrentEntry( const QString &, int );
	void playNext();
	void message( const QString &, int ms );
	void clearCurrentPlaying();
	void clearInfo();
	void quit();
	void resetARatio();
	void updateBitrate( int, int, double );
	void updateBuffered( qint64 backwardBytes, qint64 remainingBytes, double backwardSeconds, double remainingSeconds );
	void updateBufferedRange( int, int );
	void updateWindowTitle( const QString &t = QString() );
	void updateImage( const QImage &img = QImage() );
	void videoStarted();
};

#endif //PLAYCLASS_HPP
