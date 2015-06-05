#include <VideoWriter.hpp>
#include <ImgScaler.hpp>

#include <QWidget>
#include <QMutex>

class QPainterWriter;
class QMPlay2_OSD;

class Drawable : public QWidget
{
public:
	Drawable( class QPainterWriter & );
	~Drawable();

	void draw( const QByteArray &, bool, bool );
	void clr();

	void resizeEvent( QResizeEvent * );

	QByteArray videoFrameData;
	QList< const QMPlay2_OSD * > osd_list;
	int Brightness, Contrast;
	QMutex osd_mutex;
private:
	void paintEvent( QPaintEvent * );
	bool event( QEvent * );

	int X, Y, W, H;
	QPainterWriter &writer;
	QImage img;
	ImgScaler imgScaler;
};

/**/

class QPainterWriter : public VideoWriter
{
	friend class Drawable;
public:
	QPainterWriter( Module & );
private:
	~QPainterWriter();

	bool set();

	bool readyWrite() const;

	bool processParams( bool *paramsCorrected );
	qint64 write( const QByteArray & );
	void writeOSD( const QList< const QMPlay2_OSD * > & );

	QString name() const;

	bool open();

	/**/

	int outW, outH, flip;
	double aspect_ratio, zoom;

	Drawable *drawable;
};

#define QPainterWriterName "QPainter Writer"
