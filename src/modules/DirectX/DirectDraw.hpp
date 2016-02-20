#include <VideoWriter.hpp>

#include <QWidget>
#include <QTimer>

#include <ddraw.h>

class DirectDrawWriter;
class QMPlay2_OSD;

class Drawable : public QWidget
{
	Q_OBJECT
	typedef HRESULT (WINAPI *DwmEnableCompositionProc)(UINT uCompositionAction);
public:
	Drawable(DirectDrawWriter &);
	~Drawable();

	inline bool canDraw() const
	{
		return DDSSecondary;
	}

	void dock();
	bool createSecondary();
	void videoEqSet();
	void setFlip();

	void draw(const VideoFrame &videoFrame);

	void resizeEvent(QResizeEvent *);

	QList< const QMPlay2_OSD * > osd_list;
	QMutex osd_mutex;
	bool isOK, isOverlay, paused;
private:
	void getRects(RECT &, RECT &);
	void fillRects();

	Q_SLOT void updateOverlay();
	Q_SLOT void overlayVisible(bool);
	Q_SLOT void doOverlayVisible();
	void blit();

	void releaseSecondary();
	bool restoreLostSurface();

	void paintEvent(QPaintEvent *);
	bool event(QEvent *);

	QPaintEngine *paintEngine() const;

	QImage osdImg;
	QList< QByteArray > osd_checksums;

	QTimer visibleTim;

	DirectDrawWriter &writer;

	int X, Y, W, H, flip;

	HBRUSH blackBrush;
	LPDIRECTDRAW DDraw;
	LPDIRECTDRAWCLIPPER DDClipper;
	LPDIRECTDRAWSURFACE DDSPrimary, DDSSecondary, DDSBackBuffer;
	LPDIRECTDRAWCOLORCONTROL DDrawColorCtrl;

	DwmEnableCompositionProc DwmEnableComposition;
};

/**/

class DirectDrawWriter : public VideoWriter
{
	friend class Drawable;
public:
	DirectDrawWriter(Module &);
private:
	~DirectDrawWriter();

	bool set();

	bool readyWrite() const;

	bool processParams(bool *paramsCorrected);
	void writeVideo(const VideoFrame &videoFrame);
	void writeOSD(const QList< const QMPlay2_OSD * > &);

	void pause();

	QString name() const;

	bool open();

	/**/

	int outW, outH, flip, Hue, Saturation, Brightness, Contrast;
	double aspect_ratio, zoom;

	Drawable *drawable;
};

#define DirectDrawWriterName "DirectDraw"
