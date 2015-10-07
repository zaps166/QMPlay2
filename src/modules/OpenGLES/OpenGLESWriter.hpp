#include <VideoWriter.hpp>
#include <VideoFrame.hpp>

class OpenGLESWriter;
class QMPlay2_OSD;
struct GLPrivate;
typedef void * EGLConfig;

class GLDrawable : public QWidget
{
public:
	GLDrawable( OpenGLESWriter & );
	~GLDrawable();

	void resizeEvent( QResizeEvent * );
	bool isContextValid() const;
	bool makeCurrent();
	void clr();

	void paintGL();

	bool isOK;
	const VideoFrame *videoFrame;
	float Contrast, Saturation, Brightness, Hue;
	QList< const QMPlay2_OSD * > osd_list;
	QMutex osd_mutex;
private:
	bool initializeGL();

	QPaintEngine *paintEngine() const;

	void paintEvent( QPaintEvent * );
	bool event( QEvent * );

	GLPrivate *p;

	QList< QByteArray > osd_checksums;
	QImage osdImg;

	OpenGLESWriter &writer;
	int W, H, X, Y;
	bool hasImage, lastVSyncState, hasCurrentContext, doReset;

	EGLConfig eglConfig;
	float texCoordYCbCr[ 8 ];
	quint32 shaderProgramYCbCr, shaderProgramOSD;
	qint32 scaleLoc, videoEqLoc, texCoordYCbCrLoc, positionYCbCrLoc, texCoordOSDLoc, positionOSDLoc;
};

/**/

#include <QCoreApplication>

class OpenGLESWriter : public VideoWriter
{
	Q_DECLARE_TR_FUNCTIONS( OpenGLESWriter )
	friend class GLDrawable;
public:
	OpenGLESWriter( Module & );
private:
	~OpenGLESWriter();

	bool set();

	bool readyWrite() const;

	bool processParams( bool *paramsCorrected );
	qint64 write( const QByteArray & );
	void writeOSD( const QList< const QMPlay2_OSD * > & );

	QString name() const;

	bool open();

	/**/

	int outW, outH, W, flip;
	double aspect_ratio, zoom;
	bool VSync;

	GLDrawable *drawable;
};

#define OpenGLESWriterName "OpenGLES Writer"
