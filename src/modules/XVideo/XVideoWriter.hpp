#include <VideoWriter.hpp>

#include <xv.hpp>

#include <QWidget>

class Drawable : public QWidget
{
	friend class XVideoWriter;
public:
	Drawable( class XVideoWriter & );
private:
	void resizeEvent( QResizeEvent * );
	void paintEvent( QPaintEvent * );
	bool event( QEvent * );

	QPaintEngine *paintEngine() const;

	int X, Y, W, H;
	QRect dstRect, srcRect;
	XVideoWriter &writer;
};

/**/

class QMPlay2_OSD;

class XVideoWriter : public VideoWriter
{
	friend class Drawable;
public:
	XVideoWriter( Module & );
private:
	~XVideoWriter();

	bool set();

	bool readyWrite() const;

	bool processParams( bool *paramsCorrected );
	qint64 write( const QByteArray & );
	void writeOSD( const QList< const QMPlay2_OSD * > & );

	QString name() const;

	bool open();

	/**/

	int outW, outH, Hue, Saturation, Brightness, Contrast;
	double aspect_ratio, zoom;
	QString adaptorName;
	bool useSHM;

	Drawable *drawable;
	XVIDEO *xv;

	QList< const QMPlay2_OSD * > osd_list;
	QMutex osd_mutex;
};

#define XVideoWriterName "XVideo Writer"
