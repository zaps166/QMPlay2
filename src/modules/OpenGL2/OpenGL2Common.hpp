#ifndef OPENGLCOMMON_HPP
#define OPENGLCOMMON_HPP

#include <VideoFrame.hpp>

#ifdef OPENGL_NEW_API
	#include <QOpenGLShaderProgram>
#else
	#include <QGLShaderProgram>
	#define QOpenGLShaderProgram QGLShaderProgram
#endif

#include <QVariantAnimation>
#include <QCoreApplication>
#include <QImage>
#include <QMutex>

#if !defined OPENGL_ES2 && !defined Q_OS_MAC
	#include <GL/glext.h>
#endif

class OpenGL2Common;
class QMPlay2_OSD;
class QMouseEvent;

class RotAnimation : public QVariantAnimation
{
public:
	inline RotAnimation(OpenGL2Common &glCommon) :
		glCommon(glCommon)
	{}
private:
	void updateCurrentValue(const QVariant &value);

	OpenGL2Common &glCommon;
};

/**/

class OpenGL2Common
{
	Q_DECLARE_TR_FUNCTIONS(OpenGL2Common)
#ifndef OPENGL_ES2
	typedef    void  (APIENTRY *GLActiveTexture)(GLenum);
	typedef    void  (APIENTRY *GLGenBuffers)(GLsizei, GLuint *);
	typedef    void  (APIENTRY *GLBindBuffer)(GLenum, GLuint);
	typedef    void  (APIENTRY *GLBufferData)(GLenum, GLsizeiptr, const void *, GLenum);
	typedef    void  (APIENTRY *GLDeleteBuffers)(GLsizei, const GLuint *);
#endif
public:
	OpenGL2Common();
	virtual ~OpenGL2Common();

	virtual void deleteMe();

	virtual QWidget *widget() = 0;

	virtual bool testGL() = 0;
	virtual bool setVSync(bool enable) = 0;
	virtual void updateGL(bool requestDelayed) = 0;

	void newSize(const QSize &size = QSize());
	void clearImg();

	void setSpherical(bool spherical);

	inline bool isRotate90() const
	{
		return verticesIdx >= 4 && !sphericalView;
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
	GLGenBuffers glGenBuffers;
	GLBindBuffer glBindBuffer;
	GLBufferData glBufferData;
	GLDeleteBuffers glDeleteBuffers;
#endif

#ifdef Q_OS_WIN
	bool preventFullscreen;
#endif

#ifdef VSYNC_SETTINGS
	bool vSync;
#endif

	void dispatchEvent(QEvent *e, QObject *p);
private:
	inline bool checkLinesize(int p);

	/* Spherical view */
	void mousePress360(QMouseEvent *e);
	void mouseMove360(QMouseEvent *e);
	void mouseRelease360(QMouseEvent *e);
	void resetSphereVbo();
	void loadSphere();
public:
	VideoFrame videoFrame;

	QOpenGLShaderProgram *shaderProgramYCbCr, *shaderProgramOSD;

	qint32 texCoordYCbCrLoc, positionYCbCrLoc, texCoordOSDLoc, positionOSDLoc;
	float Contrast, Saturation, Brightness, Hue;
	float texCoordYCbCr[8];

	bool isPaused, isOK, hasImage, doReset, setMatrix;
	int subsX, subsY, W, H, subsW, subsH, outW, outH, verticesIdx;
	int glVer, doClear;

	double aspectRatio, zoom;

	QList<const QMPlay2_OSD *> osdList;
	QMutex osdMutex;

	QList<QByteArray> osdChecksums;
	QImage osdImg;

	/* Spherical view */
	bool sphericalView, buttonPressed, hasVbo, mouseWrapped, canWrapMouse;
	RotAnimation rotAnimation;
	quint32 sphereVbo[3];
	quint32 nIndices;
	double mouseTime;
	QPoint mousePos;
	QPointF rot;
};

#endif // OPENGLCOMMON_HPP
