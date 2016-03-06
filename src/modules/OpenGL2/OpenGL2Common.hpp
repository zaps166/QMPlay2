#ifndef OPENGLCOMMON_HPP
#define OPENGLCOMMON_HPP

#include <VideoFrame.hpp>

#ifdef OPENGL_NEW_API
	#include <QOpenGLShaderProgram>
#else
	#include <QGLShaderProgram>
	#define QOpenGLShaderProgram QGLShaderProgram
#endif

#include <QCoreApplication>
#include <QImage>
#include <QMutex>

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
	virtual bool setVSync(bool enable) = 0;
	virtual void updateGL() = 0;

	void newSize(const QSize &size = QSize());
	void clearImg();

	inline bool isRotate90() const
	{
		return verticesIdx >= 4;
	}

	virtual void resetClearCounter();
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

#ifdef VSYNC_SETTINGS
	bool vSync;
#endif

	void dispatchEvent(QEvent *e, QObject *p);
public:
	VideoFrame videoFrame;

	QOpenGLShaderProgram *shaderProgramYCbCr, *shaderProgramOSD;

	qint32 texCoordYCbCrLoc, positionYCbCrLoc, texCoordOSDLoc, positionOSDLoc;
	float Contrast, Saturation, Brightness, Hue;
	float texCoordYCbCr[8];

	bool isPaused, isOK, hasImage, doReset;
	int subsX, subsY, W, H, subsW, subsH, outW, outH, verticesIdx;
	int glVer, doClear;

	double aspectRatio, zoom;

	QList< QByteArray > osdChecksums;
	QImage osdImg;

	QList< const QMPlay2_OSD * > osdList;
	QMutex osdMutex;
};

#endif // OPENGLCOMMON_HPP
