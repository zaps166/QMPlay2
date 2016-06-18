#ifndef VIDEOTHR_HPP
#define VIDEOTHR_HPP

#include <AVThread.hpp>
#include <VideoFilters.hpp>
#include <PixelFormats.hpp>

#if defined(Q_WS_X11) || (QT_VERSION >= 0x050000 && defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_ANDROID))
	#include <QScopedPointer>
	class X11ResetScreenSaver;
	#define X11_RESET_SCREEN_SAVER
#endif
class QMPlay2_OSD;
class VideoWriter;

class VideoThr : public AVThread
{
	Q_OBJECT
public:
	VideoThr(PlayClass &, Writer *, const QStringList &pluginsName = QStringList());

	inline Writer *getHWAccelWriter() const
	{
		return HWAccelWriter;
	}

	QMPlay2PixelFormats getSupportedPixelFormats() const;

	inline void setDoScreenshot()
	{
		doScreenshot = true;
	}
	inline void setSyncVtoA(bool b)
	{
		syncVtoA = b;
	}
	inline void setDeleteOSD()
	{
		deleteOSD = true;
	}

	inline void frameSizeUpdateUnlock()
	{
		frameSizeUpdateMutex.unlock();
	}

	void destroySubtitlesDecoder();
	inline void setSubtitlesDecoder(Decoder *dec)
	{
		sDec = dec;
	}

	bool setSpherical();
	bool setFlip();
	bool setRotate90();
	void setVideoEqualizer();
	void setFrameSize(int w, int h);
	void setARatio(double aRatio, double sar);
	void setZoom();

	void initFilters(bool processParams = true);

	bool processParams();

	void updateSubs();
private:
	~VideoThr();

	inline VideoWriter *videoWriter() const;

	void run();

	bool deleteSubs, syncVtoA, doScreenshot, canWrite, deleteOSD, deleteFrame;
	double lastSampleAspectRatio;
	int W, H;

	Decoder *sDec;
	Writer *HWAccelWriter;
	QMPlay2_OSD *subtitles;
	VideoFilters filters;
#ifdef X11_RESET_SCREEN_SAVER
	QScopedPointer<X11ResetScreenSaver> x11ResetScreenSaver;
#endif
	QMutex filtersMutex, frameSizeUpdateMutex;
private slots:
	void write(VideoFrame videoFrame);
	void screenshot(VideoFrame videoFrame);
	void pause();
};

#endif //VIDEOTHR_HPP
