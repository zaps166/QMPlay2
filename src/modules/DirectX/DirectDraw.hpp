#include <ImgScaler.hpp>
#include <VideoWriter.hpp>

#include <QWidget>
#include <QMutex>

#include <ddraw.h>

class DirectDrawWriter;
class QMPlay2_OSD;

class Drawable : public QWidget
{
	Q_OBJECT
public:
	Drawable( DirectDrawWriter & );
	~Drawable();

	inline bool isOK() const
	{
		return mode != NONE;
	}
	inline bool canDraw() const
	{
		return DDSSecondary;
	}
	inline bool isOverlay() const
	{
		return mode == OVERLAY;
	}

	void dock();
	bool draw( const QByteArray &videoFrameData );
	bool createSecondary();
	void colorSet( int, int, int, int );

	void resizeEvent( QResizeEvent * );

	QList< const QMPlay2_OSD * > osd_list;
	QMutex osd_mutex;
private:
	void getRects( RECT &, RECT & );

	Q_SLOT void updateOverlay();
	Q_SLOT void overlayVisible( bool );
	void blit();

	void releaseSecondary();

	inline bool restoreLostSurface()
	{
		if ( DDSPrimary->IsLost() || DDSSecondary->IsLost() || DDSBackBuffer->IsLost() )
		{
			DDSPrimary->Restore();
			DDSSecondary->Restore();
			if ( DDSSecondary != DDSBackBuffer )
				DDSBackBuffer->Restore();
			return true;
		}
		return false;
	}

	QImage osdImg;
	QList< QByteArray > osd_checksums;

	DirectDrawWriter &writer;
	ImgScaler imgScaler;

	int X, Y, W, H;
	bool lastHasSubs;

	LPDIRECTDRAW DDraw;
	LPDIRECTDRAWCLIPPER DDClipper;
	LPDIRECTDRAWSURFACE DDSPrimary, DDSSecondary, DDSBackBuffer;
	LPDIRECTDRAWCOLORCONTROL DDrawColorCtrl;

	enum MODE { NONE, OVERLAY, YV12, RGB32 };
	DWORD MemoryFlag;
	MODE mode;
};

/**/

class DirectDrawWriter : public VideoWriter
{
	friend class Drawable;
public:
	DirectDrawWriter( Module & );
private:
	~DirectDrawWriter();

	bool set();

	bool readyWrite() const;

	bool processParams( bool *paramsCorrected );
	qint64 write( const QByteArray & );
	void writeOSD( const QList< const QMPlay2_OSD * > & );

	QString name() const;

	bool open();

	/**/

	int outW, outH, flip, Hue, Saturation, Brightness, Contrast;
	double aspect_ratio, zoom;
	bool onlyOverlay;

	Drawable *drawable;
};

#define DirectDrawWriterName "DirectDraw Writer"
