#ifndef OPENGLCOMMON_HPP
#define OPENGLCOMMON_HPP

#ifdef OPENGL_NEW_API
	#include <QOpenGLShaderProgram>
#else
	#include <QGLShaderProgram>
	#define QOpenGLShaderProgram QGLShaderProgram
#endif

#include <QCoreApplication>
#include <QByteArray>
#include <QImage>
#include <QMutex>

#include <QDebug>

class QMPlay2_OSD;

class OpenGL2Common
{
	Q_DECLARE_TR_FUNCTIONS(OpenGL2Common)
public:
#ifndef OPENGL_ES2
	typedef void (APIENTRY *GLActiveTexture)(GLenum);
#endif
	OpenGL2Common();
	virtual ~OpenGL2Common();

	virtual void deleteMe();

	virtual QWidget *widget() = 0;

	virtual bool testGL() = 0;
	virtual bool VSync(bool enable) = 0;
	virtual void updateGL() = 0;

	void newSize(const QSize &size = QSize());
	void clearImg();
protected:
	void initializeGL();
	void paintGL();

	void testGLInternal();

#ifndef OPENGL_ES2
	void initGLProc();
	void showOpenGLMissingFeaturesMessage();

	bool supportsShaders, canCreateNonPowerOfTwoTextures;
	GLActiveTexture glActiveTexture;
#endif

#ifdef Q_OS_WIN
	bool preventFullscreen;
#endif

	void dispatchEvent(QEvent *e, QObject *p);
public:
	QByteArray videoFrameArr;

	QOpenGLShaderProgram *shaderProgramYCbCr, *shaderProgramOSD;

	qint32 texCoordYCbCrLoc, positionYCbCrLoc, texCoordOSDLoc, positionOSDLoc;
	float Contrast, Saturation, Brightness, Hue;
	float texCoordYCbCr[8];

	bool isPaused, isOK, hasImage, doReset;
	int X, Y, W, H, outW, outH, flip;
	int glVer;

	double aspectRatio, zoom;

	QList< QByteArray > osdChecksums;
	QImage osdImg;

	QList< const QMPlay2_OSD * > osdList;
	QMutex osdMutex;
};

#endif // OPENGLCOMMON_HPP
