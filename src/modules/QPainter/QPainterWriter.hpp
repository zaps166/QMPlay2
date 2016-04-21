#include <VideoWriter.hpp>
#include <VideoFrame.hpp>
#include <ImgScaler.hpp>

#include <QWidget>

class QPainterWriter;
class QMPlay2_OSD;

class Drawable : public QWidget
{
public:
	Drawable(class QPainterWriter &);
	~Drawable();

	void draw(const VideoFrame &newVideoFrame, bool, bool);
	void clr();

	void resizeEvent(QResizeEvent *);

	VideoFrame videoFrame;
	QList<const QMPlay2_OSD *> osd_list;
	int Brightness, Contrast;
	QMutex osd_mutex;
private:
	void paintEvent(QPaintEvent *);
	bool event(QEvent *);

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
	QPainterWriter(Module &);
private:
	~QPainterWriter();

	bool set();

	bool readyWrite() const;

	bool processParams(bool *paramsCorrected);
	void writeVideo(const VideoFrame &videoFrame);
	void writeOSD(const QList<const QMPlay2_OSD *> &);

	QString name() const;

	bool open();

	/**/

	int outW, outH, flip;
	double aspect_ratio, zoom;

	Drawable *drawable;
};

#define QPainterWriterName "QPainter"
