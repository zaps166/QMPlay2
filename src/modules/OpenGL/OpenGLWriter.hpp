#include <VideoWriter.hpp>
#include <VideoFrame.hpp>
#include <ImgScaler.hpp>

#include <QGLWidget>
#include <QMutex>

class QGLShaderProgram;
class OpenGLWriter;
class QMPlay2_OSD;

#if !defined Q_WS_X11 && !defined Q_OS_WIN && !defined Q_OS_LINUX && !defined Q_OS_FREEBSD
	#define QtVSync
#endif

class Drawable : public QGLWidget
{
public:
#ifdef QtVSync
	Drawable( OpenGLWriter &, const QGLFormat & );
#else
	Drawable( OpenGLWriter & );
#endif
	~Drawable();

	void clr();

	void resizeEvent( QResizeEvent * );

	bool setVideoEQ;
	const VideoFrame *videoFrame;
	float Contrast, Saturation, Brightness, Hue;
	QList< const QMPlay2_OSD * > osd_list;
	QMutex osd_mutex;
private:
#ifndef QtVSync
	void VSync();
#endif

	void initializeGL();
	void resizeGL( int, int );
	void paintGL();

	typedef void ( APIENTRY *GLActiveTexture )( GLenum texture );
	GLActiveTexture glActiveTexture;

	QList< QByteArray > osd_checksums;
	QImage osdImg;

	OpenGLWriter &writer;
	QGLShaderProgram *program;
	float tex_w;
	ImgScaler imgScaler;
	int W, H, X, Y, maxTextureSize;
	bool noShaders, hasImage, canCreateTexturesNonPowerOf2;
#ifndef QtVSync
	bool lastVSyncState;
#endif
};

/**/

#include <QCoreApplication>

class OpenGLWriter : public VideoWriter
{
	Q_DECLARE_TR_FUNCTIONS( OpenGLWriter )
	friend class Drawable;
public:
	OpenGLWriter( Module & );
private:
	~OpenGLWriter();

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
	bool VSync, useShaders;

	Drawable *drawable;
};

#define OpenGLWriterName "OpenGL Writer"
