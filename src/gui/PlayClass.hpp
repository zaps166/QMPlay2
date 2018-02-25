/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <PacketBuffer.hpp>

#include <QSet>
#include <QSize>
#include <QTimer>
#include <QMutex>
#include <QImage>
#include <QObject>
#include <QStringList>
#include <QWaitCondition>

class QMPlay2OSD;
class DemuxerThr;
class VideoThr;
class AudioThr;
class Demuxer;
class Slider;
class LibASS;

enum
{
	SEEK_NOWHERE = -1,
	SEEK_STREAM_RELOAD = -2, /* Seeks to current position after stream reload */
	SEEK_REPEAT = -3
};

class PlayClass final : public QObject
{
	Q_OBJECT
	friend class DemuxerThr;
	friend class AVThread;
	friend class VideoThr;
	friend class AudioThr;
public:
	PlayClass();
	~PlayClass();

	Q_SLOT void play(const QString &);
	Q_SLOT void stop(bool quitApp = false);
	void restart();

	inline bool canUpdatePosition() const
	{
		return canUpdatePos;
	}

	void chPos(double, bool updateGUI = true);

	void togglePause();
	void seek(double pos, bool allowAccurate = true);
	Q_SLOT void chStream(const QString &s);
	void setSpeed(double);

	bool isPlaying() const;

	inline QString getUrl() const
	{
		return url;
	}
	inline double getPos() const
	{
		return pos;
	}

	void loadSubsFile(const QString &);

	void messageAndOSD(const QString &, bool onStatusBar = true, double duration = 1.0);

	inline void setDoSilenceOnStart()
	{
		doSilenceOnStart = true;
	}

private:
	inline bool hasVideoStream();
	inline bool hasAudioStream();

	void speedMessageAndOSD();

	double getARatio();
	inline double getSAR();

	void flushAssEvents();
	void clearSubtitlesBuffer();

	void stopVThr();
	void stopAThr();
	inline void stopAVThr();

	void stopVDec();
	void stopADec();

	void setFlip();
	void flipRotMsg();

	void clearPlayInfo();

	void updateABRepeatInfo(bool showDisabledInfo);

	bool setAudioParams(quint8 realChannels, quint32 realSampleRate);

	inline void emitSetVideoCheckState();

	DemuxerThr *demuxThr;
	VideoThr *vThr;
	AudioThr *aThr;

	QWaitCondition emptyBufferCond;
	volatile bool fillBufferB, doSilenceBreak;
	QMutex loadMutex, stopPauseMutex;

	PacketBuffer aPackets, vPackets, sPackets;

	double frame_last_pts, frame_last_delay, audio_current_pts, audio_last_delay;
	bool doSilenceOnStart, canUpdatePos, paused, waitForData, flushVideo, flushAudio, muted, reload, nextFrameB, endOfStream, ignorePlaybackError, videoDecErrorLoad, pauseAfterFirstFrame = false;
	double seekTo, lastSeekTo, restartSeekTo, seekA, seekB, videoSeekPos, audioSeekPos;
	double vol[2], replayGain, zoom, pos, skipAudioFrame, videoSync, speed, subtitlesSync, subtitlesScale;
	int flip;
	bool rotate90, spherical, stillImage;

	QString url, newUrl, aRatioName;

	int audioStream, videoStream, subtitlesStream;
	int choosenAudioStream, choosenVideoStream, choosenSubtitlesStream;
	QString choosenAudioLang, choosenSubtitlesLang;

	QSet<QString> videoDecodersError;
	QString videoDecoderModuleName;

	double maxThreshold, fps;

	bool quitApp, audioEnabled, videoEnabled, subtitlesEnabled, doSuspend, doRepeat, allowAccurateSeek, paramsForced = false;
	QTimer timTerminate;

#if defined Q_OS_WIN && !defined Q_OS_WIN64
	bool firsttimeUpdateCache;
#endif
	LibASS *ass;

	QMutex osdMutex, subsMutex;
	QMPlay2OSD *osd;
	int videoWinW, videoWinH;
	QStringList fileSubsList;
	QString fileSubs;
private slots:
	void suspendWhenFinished(bool b);
	void repeatEntry(bool b);

	void saveCover();
	void settingsChanged(int, bool);
	void videoResized(int, int);

	void videoAdjustmentChanged();

	void setAB();
	void speedUp();
	void slowDown();
	void setSpeed();
	void zoomIn();
	void zoomOut();
	void zoomReset();
	void otherReset();
	void aRatio();
	void volume(int, int);
	void toggleMute();
	void slowDownVideo();
	void speedUpVideo();
	void setVideoSync();
	void slowDownSubs();
	void speedUpSubs();
	void setSubtitlesSync();
	void biggerSubs();
	void smallerSubs();
	void toggleAVS(bool);
	void setSpherical(bool);
	void setHFlip(bool);
	void setVFlip(bool);
	void setRotate90(bool);
	void screenShot();
	void prevFrame();
	void nextFrame();

	void frameSizeUpdated(int w, int h);
	void aRatioUpdated(double sar);
	void pixelFormatUpdated(const QByteArray &pixFmt);

	void audioParamsUpdated(quint8 channels, quint32 sampleRate);

	void demuxThrFinished();

	void timTerminateFinished();

	void load(Demuxer *);
signals:
	void frameSizeUpdate(int w, int h);
	void audioParamsUpdate(quint8 channels, quint32 sampleRate);
	void aRatioUpdate(double sar);
	void pixelFormatUpdate(const QByteArray &pixFmt);
	void chText(const QString &);
	void updateLength(int);
	void updatePos(double);
	void playStateChanged(bool);
	void setCurrentPlaying();
	void setInfo(const QString &, bool, bool);
	void updateCurrentEntry(const QString &, double);
	void playNext(bool playingError);
	void clearCurrentPlaying();
	void clearInfo();
	void quit();
	void resetARatio();
	void updateBitrateAndFPS(int a, int v, double fps = -1.0, double realFPS = -1.0, bool interlaced = false);
	void updateBuffered(qint64 backwardBytes, qint64 remainingBytes, double backwardSeconds, double remainingSeconds);
	void updateBufferedRange(int, int);
	void updateWindowTitle(const QString &t = QString());
	void updateImage(const QImage &img = QImage());
	void videoStarted(bool hasVideo);
	void videoNotStarted();
	void uncheckSuspend();
	void setVideoCheckState(bool rotate90, bool hFlip, bool vFlip, bool spherical);
};
