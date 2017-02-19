/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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
#include <VideoAdjustment.hpp>

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

class HWAccelInterface;
class OpenGL2Common;
class QMPlay2OSD;
class QMouseEvent;

class RotAnimation : public QVariantAnimation
{
public:
	inline RotAnimation(OpenGL2Common &glCommon) :
		glCommon(glCommon)
	{}
private:
	void updateCurrentValue(const QVariant &value) override;

	OpenGL2Common &glCommon;
};

/**/

class OpenGL2Common
{
	Q_DECLARE_TR_FUNCTIONS(OpenGL2Common)
#ifndef OPENGL_ES2
	using GLActiveTexture  = void  (APIENTRY *)(GLenum);
	using GLGenBuffers     = void  (APIENTRY *)(GLsizei, GLuint *);
	using GLBindBuffer     = void  (APIENTRY *)(GLenum, GLuint);
	using GLBufferData     = void  (APIENTRY *)(GLenum, GLsizeiptr, const void *, GLenum);
	using GLDeleteBuffers  = void  (APIENTRY *)(GLsizei, const GLuint *);
#endif
	using GLMapBufferRange = void *(APIENTRY *)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
	using GLMapBuffer      = void *(APIENTRY *)(GLenum, GLbitfield);
	using GLUnmapBuffer    = GLboolean(APIENTRY *)(GLenum);
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

	void contextAboutToBeDestroyed();

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

#ifdef VSYNC_SETTINGS
	bool vSync;
#endif

	void dispatchEvent(QEvent *e, QObject *p);
private:
	inline bool isRotate90() const;

	inline bool hwAccellPossibleLock();

	QByteArray readShader(const QString &fileName);

	/* Spherical view */
	void mousePress360(QMouseEvent *e);
	void mouseMove360(QMouseEvent *e);
	void mouseRelease360(QMouseEvent *e);
	inline void resetSphereVbo();
	inline void deleteSphereVbo();
	void loadSphere();
public:
	HWAccelInterface *hwAccellnterface;
	QStringList videoAdjustmentKeys;
	VideoFrame videoFrame;

	QOpenGLShaderProgram *shaderProgramVideo, *shaderProgramOSD;

	qint32 texCoordYCbCrLoc, positionYCbCrLoc, texCoordOSDLoc, positionOSDLoc;
	VideoAdjustment videoAdjustment;
	float texCoordYCbCr[8];
	QVector2D pixelStep;
	quint32 textures[4];
	qint32 numPlanes;
	int Deinterlace;

	quint32 pbo[4];
	bool allowPBO, hasPbo;

#ifdef Q_OS_WIN
	bool preventFullScreen;
#endif

	bool isPaused, isOK, hasImage, doReset, setMatrix, correctLinesize, canUseHueSharpness;
	int subsX, subsY, W, H, subsW, subsH, outW, outH, verticesIdx;
	int glVer;

	double aspectRatio, zoom;

	QList<const QMPlay2OSD *> osdList;
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
