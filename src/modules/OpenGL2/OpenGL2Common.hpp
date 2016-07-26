/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
#include <QTimer>

#if !defined OPENGL_ES2 && !defined Q_OS_MAC
	#include <GL/glext.h>
#endif

#if defined OPENGL_ES2 && !defined APIENTRY
	#define APIENTRY
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
	typedef    void *(APIENTRY *GLMapBufferRange)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
	typedef    void *(APIENTRY *GLMapBuffer)(GLenum, GLbitfield);
	typedef GLboolean(APIENTRY *GLUnmapBuffer)(GLenum);
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
protected:
	void initializeGL();
	void paintGL();

	void testGLInternal();

	void initGLProc();
#ifndef OPENGL_ES2
	void showOpenGLMissingFeaturesMessage();

	bool supportsShaders, canCreateNonPowerOfTwoTextures;
	GLActiveTexture glActiveTexture;
	GLGenBuffers glGenBuffers;
	GLBindBuffer glBindBuffer;
	GLBufferData glBufferData;
	GLDeleteBuffers glDeleteBuffers;
#endif
	GLMapBufferRange glMapBufferRange;
	GLMapBuffer glMapBuffer;
	GLUnmapBuffer glUnmapBuffer;

#ifdef Q_OS_WIN
	bool preventFullscreen;
#endif

#ifdef VSYNC_SETTINGS
	bool vSync;
#endif

	void dispatchEvent(QEvent *e, QObject *p);
private:
	inline bool isRotate90() const;

	QByteArray readShader(const QString &fileName);

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
	float Contrast, Saturation, Brightness, Hue, Sharpness;
	float texCoordYCbCr[8];
	QVector2D pixelStep;
	quint32 textures[4];

	quint32 pbo[4];
	bool hasPbo;

	bool isPaused, isOK, hasImage, doReset, setMatrix;
	int subsX, subsY, W, H, subsW, subsH, outW, outH, verticesIdx;
	int glVer;

	double aspectRatio, zoom;

	QList<const QMPlay2_OSD *> osdList;
	QMutex osdMutex;

	QList<QByteArray> osdChecksums;
	QImage osdImg;

	QTimer updateTimer;

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
