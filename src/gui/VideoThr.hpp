#ifndef VIDEOTHR_HPP
#define VIDEOTHR_HPP

#include <AVThread.hpp>
#include <VideoFilters.hpp>

class QMPlay2_OSD;

class VideoThr : public AVThread
{
	Q_OBJECT
public:
	VideoThr( PlayClass &, Writer *, const QStringList &pluginsName = QStringList() );

	inline bool isHWAccel() const
	{
		return HWAccel;
	}

	inline void setDoScreenshot()
	{
		doScreenshot = true;
	}
	inline void setSyncVtoA( bool b )
	{
		syncVtoA = b;
	}
	inline void setDeleteOSD()
	{
		deleteOSD = true;
	}

	void destroySubtitlesDecoder();
	inline void setSubtitlesDecoder( Decoder *dec )
	{
		sDec = dec;
	}

	bool setFlip();
	void setVideoEqualizer();
	void setFrameSize( int w, int h );
	void setARatio( double );
	void setZoom();

	void initFilters( bool processParams = true );

	bool processParams();

	void updateSubs();

private:
	~VideoThr();

	void run();

	bool deleteSubs, syncVtoA, doScreenshot, canWrite, HWAccel, deleteOSD, deleteFrame;
	double lastSampleAspectRatio;
	int W, H;

	Decoder *sDec;
	QMPlay2_OSD *subtitles;
	VideoFilters filters;
private slots:
	void write_slot( const QByteArray & );
	void screenshot_slot( const QByteArray & );
	void pause_slot();
signals:
	void write( const QByteArray & );
	void screenshot( const QByteArray & );
	void pause();
};

#endif //VIDEOTHR_HPP
