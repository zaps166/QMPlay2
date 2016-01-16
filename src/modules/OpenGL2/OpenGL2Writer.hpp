#include <VideoWriter.hpp>
#include <VideoFrame.hpp>

#ifndef USE_NEW_OPENGL_API
	#include <QGLWidget>
	#include <QGLShaderProgram>
#else
	#include <QOpenGLWidget>
	#include <QOpenGLShaderProgram>
#endif

class OpenGL2Writer;
class QMPlay2_OSD;

class Drawable : public QGLWidget
{
	Q_OBJECT
#ifndef OPENGL_ES2
	typedef void (APIENTRY *GLActiveTexture)(GLenum);
#endif
public:
#ifndef USE_NEW_OPENGL_API
	Drawable(OpenGL2Writer &, const QGLFormat &);
#else
	Drawable(OpenGL2Writer &);
#endif

	bool testGL();

#ifndef OPENGL_ES2
	void initGLProc();
#endif

	void clr();

	void resizeEvent(QResizeEvent *);

	bool isOK, doReset, paused;
	QByteArray videoFrameArr;
	float Contrast, Saturation, Brightness, Hue;
	QList< const QMPlay2_OSD * > osd_list;
	QMutex osd_mutex;
	QString glVer;
#ifndef USE_NEW_OPENGL_API
private slots:
	void resetClearCounter();
#endif
private:
#ifndef OPENGL_ES2
	void showOpenGLMissingFeaturesMessage();
#endif

	void initializeGL();
#ifndef USE_NEW_OPENGL_API
	void resizeGL(int w, int h);
#endif
	void paintGL();

#ifndef USE_NEW_OPENGL_API
	void paintEvent(QPaintEvent *);
#endif
	bool event(QEvent *);

	QGLShaderProgram shaderProgramYCbCr, shaderProgramOSD;
#ifndef OPENGL_ES2
	bool supportsShaders, canCreateNonPowerOfTwoTextures;
	GLActiveTexture glActiveTexture;
#endif

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
#ifndef USE_NEW_OPENGL_API
	quint32 doClear;
#endif
};

/**/

#include <QCoreApplication>

class OpenGL2Writer : public VideoWriter
{
	Q_DECLARE_TR_FUNCTIONS(OpenGL2Writer)
	friend class Drawable;
public:
	OpenGL2Writer(Module &);
private:
	~OpenGL2Writer();

	bool set();

	bool readyWrite() const;

	bool processParams(bool *paramsCorrected);
	qint64 write(const QByteArray &);
	void writeOSD(const QList< const QMPlay2_OSD * > &);

	void pause();

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
