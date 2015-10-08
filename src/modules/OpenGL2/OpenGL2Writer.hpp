#include <VideoWriter.hpp>
#include <VideoFrame.hpp>

#include <QGLWidget>
#include <QGLFunctions>
#include <QGLShaderProgram>

class OpenGL2Writer;
class QMPlay2_OSD;

class Drawable : public QGLWidget, private QGLFunctions
{
public:
	Drawable( OpenGL2Writer & );

	bool init();
	void clr();

	void resizeEvent( QResizeEvent * );

	bool isOK, doReset;
	const VideoFrame *videoFrame;
	float Contrast, Saturation, Brightness, Hue;
	QList< const QMPlay2_OSD * > osd_list;
	QMutex osd_mutex;
private:
	void initializeGL();
	void resizeGL( int w, int h );
	void paintGL();

	bool event( QEvent * );

	QGLShaderProgram shaderProgramYCbCr, shaderProgramOSD;

	QList< QByteArray > osd_checksums;
	QImage osdImg;

	OpenGL2Writer &writer;
	int W, H, X, Y;
	bool hasImage;
#ifdef VSYNC_SETTINGS
	bool lastVSyncState;
#endif

	float texCoordYCbCr[ 8 ];
	qint32 texCoordYCbCrLoc, positionYCbCrLoc, texCoordOSDLoc, positionOSDLoc;
};

/**/

#include <QCoreApplication>

class OpenGL2Writer : public VideoWriter
{
	Q_DECLARE_TR_FUNCTIONS( OpenGL2Writer )
	friend class Drawable;
public:
	OpenGL2Writer( Module & );
private:
	~OpenGL2Writer();

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
#ifdef VSYNC_SETTINGS
	bool vSync;
#endif

	Drawable *drawable;
};

#define OpenGL2WriterName "OpenGL 2"
