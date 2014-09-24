#ifndef VIDEOTHR_HPP
#define VIDEOTHR_HPP

#include <AVThread.hpp>
#include <VideoFilters.hpp>

class QMPlay2_OSD;
class PlayClass;
class Decoder;
class Writer;

class VideoThr : public AVThread
{
	Q_OBJECT
public:
	VideoThr( PlayClass &, Writer *, const QStringList &pluginsName = QStringList() );

	void stop( bool terminate = false );

	inline bool isHWAccel() const
	{
		return HWAccel;
	}

	bool setFlip();
	void setVideoEqualizer();
	void setFrameSize( int w, int h );
	void setARatio( double );
	void setZoom();

	void initFilters( bool processParams = true );

	bool processParams();

	void updateSubs();

	Decoder *sDec;
	bool deleteSubs, syncVtoA, do_screenshot;
private:
	~VideoThr();

	void run();

	bool canWrite, HWAccel, deleteOSD, deleteFrame;
	int W, H;

	QMPlay2_OSD *napisy;
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
