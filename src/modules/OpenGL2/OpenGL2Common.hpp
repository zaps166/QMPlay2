/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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

#pragma once

#include <VideoFrame.hpp>
#include <VideoAdjustment.hpp>

#include <QOpenGLShaderProgram>

#include <QVariantAnimation>
#include <QCoreApplication>
#include <QImage>
#include <QMutex>
#include <QTimer>

#if !defined OPENGL_ES2 && !defined Q_OS_MACOS
	#include <GL/glext.h>
#endif

#if defined OPENGL_ES2 && !defined APIENTRY
	#define APIENTRY
#endif

class HWAccelInterface;
class OpenGL2Common;
class QMPlay2OSD;
class QMouseEvent;

class RotAnimation final : public QVariantAnimation
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
	using GLGenerateMipmap = void  (APIENTRY *)(GLenum);
#endif
	using GLMapBufferRange = void *(APIENTRY *)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
	using GLMapBuffer      = void *(APIENTRY *)(GLenum, GLbitfield);
	using GLUnmapBuffer    = GLboolean(APIENTRY *)(GLenum);
public:
	OpenGL2Common();
	virtual ~OpenGL2Common();

	virtual void deleteMe();

	virtual QWidget *widget() = 0;

	bool testGL();
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
	GLGenerateMipmap glGenerateMipmap = nullptr;
#endif
	GLMapBufferRange glMapBufferRange = nullptr;
	GLMapBuffer glMapBuffer = nullptr;
	GLUnmapBuffer glUnmapBuffer = nullptr;

	bool vSync;

	void dispatchEvent(QEvent *e, QObject *p);
private:
	void maybeSetMipmaps(qreal dpr);

	inline bool isRotate90() const;

	inline bool hwAccellPossibleLock();

	QByteArray readShader(const QString &fileName, bool pure = false);

	void mousePress(QMouseEvent *e);
	void mouseMove(QMouseEvent *e);
	void mouseRelease(QMouseEvent *e);

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
	quint32 textures[4];
	QSize m_textureSize;
	qint32 numPlanes;
	quint32 target;
	int Deinterlace;

	quint32 pbo[4];
	bool allowPBO, hasPbo, hqScaling = false;

#ifdef Q_OS_WIN
	bool preventFullScreen;
#endif

	bool isPaused, isOK, hwAccelError, hasImage, doReset, setMatrix, correctLinesize, canUseHueSharpness, m_useMipmaps = false;
	int subsX, subsY, W, H, subsW, subsH, outW, outH, verticesIdx;
	int glVer;

	double aspectRatio, zoom;

	bool moveVideo = false, moveOSD = false;
	QPointF videoOffset, osdOffset;

	QList<const QMPlay2OSD *> osdList;
	QMutex osdMutex;

	QVector<quint64> osd_ids;
	QImage osdImg;

	QTimer updateTimer;

	/* Spherical view */
	bool sphericalView, buttonPressed, hasVbo, mouseWrapped, canWrapMouse;
	RotAnimation rotAnimation;
	quint32 sphereVbo[3];
	quint32 nIndices;
	double mouseTime;
	QPoint mousePos; // also for moving video/subtitles
	QPointF rot;
};
