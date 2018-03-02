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

#include <OpenGL2Common.hpp>

#include <Vertices.hpp>
#include <Sphere.hpp>

#include <HWAccelInterface.hpp>
#include <QMPlay2Core.hpp>
#include <VideoFrame.hpp>
#include <Functions.hpp>

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLShader>
#include <QResizeEvent>
#include <QMatrix4x4>
#include <QResource>
#include <QPainter>
#include <QWidget>

#include <cmath>

/* OpenGL|ES 2.0 doesn't have those definitions */
#ifndef GL_MAP_WRITE_BIT
	#define GL_MAP_WRITE_BIT 0x0002
#endif
#ifndef GL_MAP_INVALIDATE_BUFFER_BIT
	#define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
#endif
#ifndef GL_WRITE_ONLY
	#define GL_WRITE_ONLY 0x88B9
#endif
#ifndef GL_PIXEL_UNPACK_BUFFER
	#define GL_PIXEL_UNPACK_BUFFER 0x88EC
#endif
#ifndef GL_R8
	#define GL_R8 0x8229
#endif
#ifndef GL_RG8
	#define GL_RG8 0x822B
#endif
#ifndef GL_RED
	#define GL_RED 0x1903
#endif
#ifndef GL_RG
	#define GL_RG 0x8227
#endif
#ifndef GL_TEXTURE_RECTANGLE_ARB
	#define GL_TEXTURE_RECTANGLE_ARB 0x84F5
#endif

/* RotAnimation */

void RotAnimation::updateCurrentValue(const QVariant &value)
{
	if (!glCommon.buttonPressed)
	{
		const QPointF newRot = value.toPointF();
		glCommon.rot.setX(qBound<qreal>(0.0, newRot.x(), 180.0));
		glCommon.rot.setY(newRot.y());
		glCommon.setMatrix = true;
		glCommon.updateGL(true);
	}
}

/* OpenGLCommon implementation */

OpenGL2Common::OpenGL2Common() :
#ifndef OPENGL_ES2
	supportsShaders(false), canCreateNonPowerOfTwoTextures(false),
	glActiveTexture(nullptr),
#endif
	vSync(true),
	hwAccellnterface(nullptr),
	shaderProgramVideo(nullptr), shaderProgramOSD(nullptr),
	texCoordYCbCrLoc(-1), positionYCbCrLoc(-1), texCoordOSDLoc(-1), positionOSDLoc(-1),
	numPlanes(0),
	target(0),
	Deinterlace(0),
	allowPBO(true), hasPbo(false),
#ifdef Q_OS_WIN
	preventFullScreen(false),
#endif
	isPaused(false), isOK(false), hwAccelError(false), hasImage(false), doReset(true), setMatrix(true), correctLinesize(false), canUseHueSharpness(true),
	subsX(-1), subsY(-1), W(-1), H(-1), subsW(-1), subsH(-1), outW(-1), outH(-1), verticesIdx(0),
	glVer(0),
	aspectRatio(0.0), zoom(0.0),
	sphericalView(false), buttonPressed(false), hasVbo(true), mouseWrapped(false), canWrapMouse(true),
	rotAnimation(*this),
	nIndices(0),
	mouseTime(0.0)
{
	videoAdjustment.unset();

	/* Initialize texCoordYCbCr array */
	texCoordYCbCr[0] = texCoordYCbCr[4] = texCoordYCbCr[5] = texCoordYCbCr[7] = 0.0f;
	texCoordYCbCr[1] = texCoordYCbCr[3] = 1.0f;

	/* Set 360° view */
	rotAnimation.setEasingCurve(QEasingCurve::OutQuint);
	rotAnimation.setDuration(1000.0);
}
OpenGL2Common::~OpenGL2Common()
{
	contextAboutToBeDestroyed();
	delete shaderProgramVideo;
	delete shaderProgramOSD;
}

void OpenGL2Common::deleteMe()
{
	delete this;
}

bool OpenGL2Common::testGL()
{
	QOpenGLContext glCtx;
	if ((isOK = glCtx.create()))
	{
		QOffscreenSurface offscreenSurface;
		offscreenSurface.create();
		if ((isOK = glCtx.makeCurrent(&offscreenSurface)))
			testGLInternal();
	}
	return isOK;
}

void OpenGL2Common::newSize(const QSize &size)
{
	const bool canUpdate = !size.isValid();
	const QSize winSize = canUpdate ? widget()->size() : size;
	const qreal dpr = widget()->devicePixelRatioF();
	if (!isRotate90())
	{
		Functions::getImageSize(aspectRatio, zoom, winSize.width(), winSize.height(), W, H, &subsX, &subsY);
		Functions::getImageSize(aspectRatio, zoom, winSize.width() * dpr, winSize.height() * dpr, subsW, subsH, &subsX, &subsY);
	}
	else
	{
		Functions::getImageSize(aspectRatio, zoom, winSize.height(), winSize.width(), H, W);
		Functions::getImageSize(aspectRatio, zoom, winSize.width() * dpr, winSize.height() * dpr, subsW, subsH, &subsX, &subsY);
	}
	doReset = true;
	if (canUpdate)
	{
		if (isPaused)
			updateGL(false);
		else if (!updateTimer.isActive())
			updateTimer.start(40);
	}
}
void OpenGL2Common::clearImg()
{
	hasImage = false;
	osdImg = QImage();
	videoFrame.clear();
	osd_ids.clear();
}

void OpenGL2Common::setSpherical(bool spherical)
{
	const bool isSphericalView = (spherical && hasVbo);
	if (sphericalView != isSphericalView)
	{
		QWidget *w = widget();
		const bool isBlankCursor = (w->cursor().shape() == Qt::BlankCursor);
		sphericalView = isSphericalView;
		if (sphericalView)
		{
			w->setProperty("customCursor", (int)Qt::OpenHandCursor);
			if (!isBlankCursor)
				w->setCursor(Qt::OpenHandCursor);
			rot = QPointF(90.0, 90.0);
		}
		else
		{
			w->setProperty("customCursor", QVariant());
			if (!isBlankCursor)
				w->setCursor(Qt::ArrowCursor);
			buttonPressed = false;
		}
	}
}

void OpenGL2Common::initializeGL()
{
	initGLProc();
#ifndef OPENGL_ES2
	if (!glActiveTexture) //Be sure that "glActiveTexture" has valid pointer (don't check "supportsShaders" here)!
	{
		showOpenGLMissingFeaturesMessage();
		isOK = false;
		return;
	}
#endif

	delete shaderProgramVideo;
	delete shaderProgramOSD;
	shaderProgramVideo = new QOpenGLShaderProgram;
	shaderProgramOSD = new QOpenGLShaderProgram;

	/* YCbCr shader */
	shaderProgramVideo->addShaderFromSourceCode(QOpenGLShader::Vertex, readShader(":/Video.vert"));
	QByteArray videoFrag;
	if (numPlanes == 1)
	{
		videoFrag = readShader(":/VideoRGB.frag");
		if (canUseHueSharpness)
		{
			//Use sharpness only when OpenGL/OpenGL|ES version >= 3.0, because it can be slow on old hardware and/or buggy drivers and may increase CPU usage!
			videoFrag.prepend("#define Sharpness\n");
		}
	}
	else
	{
		videoFrag = readShader(":/VideoYCbCr.frag");
		if (canUseHueSharpness)
		{
			//Use hue and sharpness only when OpenGL/OpenGL|ES version >= 3.0, because it can be slow on old hardware and/or buggy drivers and may increase CPU usage!
			videoFrag.prepend("#define HueAndSharpness\n");
		}
		if (numPlanes == 2)
			videoFrag.prepend("#define NV12\n");
	}
	if (target == GL_TEXTURE_RECTANGLE_ARB)
		videoFrag.prepend("#define TEXTURE_RECTANGLE\n");
	if (hqScaling)
	{
		constexpr const char *getTexelDefine = "#define getTexel texture\n";
		Q_ASSERT(videoFrag.contains(getTexelDefine));
		videoFrag.replace(getTexelDefine, readShader(":/Bicubic.frag", true));
	}
	shaderProgramVideo->addShaderFromSourceCode(QOpenGLShader::Fragment, videoFrag);
	if (shaderProgramVideo->bind())
	{
		texCoordYCbCrLoc = shaderProgramVideo->attributeLocation("aTexCoord");
		positionYCbCrLoc = shaderProgramVideo->attributeLocation("aPosition");
		shaderProgramVideo->setUniformValue((numPlanes == 1) ? "uRGB" : "uY" , 0);
		if (numPlanes == 2)
			shaderProgramVideo->setUniformValue("uCbCr", 1);
		else if (numPlanes == 3)
		{
			shaderProgramVideo->setUniformValue("uCb", 1);
			shaderProgramVideo->setUniformValue("uCr", 2);
		}
		shaderProgramVideo->release();
	}
	else
	{
		QMPlay2Core.logError(tr("Shader compile/link error"));
		isOK = false;
		return;
	}

	/* OSD shader */
	shaderProgramOSD->addShaderFromSourceCode(QOpenGLShader::Vertex, readShader(":/OSD.vert"));
	shaderProgramOSD->addShaderFromSourceCode(QOpenGLShader::Fragment, readShader(":/OSD.frag"));
	if (shaderProgramOSD->bind())
	{
		texCoordOSDLoc = shaderProgramOSD->attributeLocation("aTexCoord");
		positionOSDLoc = shaderProgramOSD->attributeLocation("aPosition");
		shaderProgramOSD->setUniformValue("uTex", 3);
		shaderProgramOSD->release();
	}
	else
	{
		QMPlay2Core.logError(tr("Shader compile/link error"));
		isOK = false;
		return;
	}

	/* Set OpenGL parameters */
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_DITHER);

	/* Prepare textures */
	glGenTextures(numPlanes + 1, textures);
	for (int i = 0; i < numPlanes + 1; ++i)
	{
		const quint32 tmpTarget = (i == 0) ? GL_TEXTURE_2D : target;
		qint32 tmpParam  = (i == 0) ? GL_NEAREST : GL_LINEAR;
		glBindTexture(tmpTarget, textures[i]);
		glTexParameteri(tmpTarget, GL_TEXTURE_MIN_FILTER, tmpParam);
		glTexParameteri(tmpTarget, GL_TEXTURE_MAG_FILTER, tmpParam);
		glTexParameteri(tmpTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(tmpTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	if (hasPbo)
	{
		glGenBuffers(1 + (hwAccellnterface ? 0 : numPlanes), pbo);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	}

	setVSync(vSync);

	doReset = true;
	resetSphereVbo();
}

void OpenGL2Common::paintGL()
{
	const bool frameIsEmpty = videoFrame.isEmpty();

	if (updateTimer.isActive())
		updateTimer.stop();

	if (frameIsEmpty && !hasImage)
		return;

	const QSize winSize = widget()->size();

	bool resetDone = false;

	if (!frameIsEmpty && hwAccellPossibleLock())
	{
		const GLsizei widths[3] = {
			videoFrame.size.width,
			videoFrame.size.chromaWidth(),
			videoFrame.size.chromaWidth(),
		};
		const GLsizei heights[3] = {
			videoFrame.size.height,
			videoFrame.size.chromaHeight(),
			videoFrame.size.chromaHeight()
		};

		if (doReset)
		{
			if (hwAccellnterface)
			{
				/* Release HWAccell resources */
				hwAccellnterface->clear(false);

				if (hwAccellnterface->canInitializeTextures())
				{
					if (numPlanes == 2)
					{
						//NV12
						for (int p = 0; p < 2; ++p)
						{
							glBindTexture(target, textures[p + 1]);
							glTexImage2D(target, 0, !p ? GL_R8 : GL_RG8, widths[p], heights[p], 0, !p ? GL_RED : GL_RG, GL_UNSIGNED_BYTE, nullptr);
						}
					}
					else if (numPlanes == 1)
					{
						//RGB32
						glBindTexture(target, textures[1]);
						glTexImage2D(target, 0, GL_RGBA, widths[0], heights[0], 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
					}
				}

				m_textureSize = QSize(widths[0], heights[0]);

				if (hqScaling)
				{
					// Must be set before "HWAccelInterface::init()" and must have "m_textureSize"
					maybeSetMipmaps(widget()->devicePixelRatioF());
				}

				/* Prepare textures, register GL textures */
				const bool hasHwAccelError = hwAccelError;
				hwAccelError = !hwAccellnterface->init(&textures[1]);
				if (hwAccelError && !hasHwAccelError)
					QMPlay2Core.logError("OpenGL 2 :: " + tr("Can't init textures for") + " " + hwAccellnterface->name());


				/* Prepare texture coordinates */
				texCoordYCbCr[2] = texCoordYCbCr[6] = 1.0f;
			}
			else
			{
				/* Check linesize */
				const qint32 halfLinesize = (videoFrame.linesize[0] >> videoFrame.size.chromaShiftW);
				correctLinesize =
				(
					(halfLinesize == videoFrame.linesize[1] && videoFrame.linesize[1] == videoFrame.linesize[2]) &&
					(!sphericalView ? (videoFrame.linesize[1] == halfLinesize) : (videoFrame.linesize[0] == widths[0]))
				);

				/* Prepare textures */
				for (qint32 p = 0; p < 3; ++p)
				{
					const GLsizei w = correctLinesize ? videoFrame.linesize[p] : widths[p];
					const GLsizei h = heights[p];
					if (p == 0)
						m_textureSize = QSize(w, h);
					if (hasPbo)
					{
						glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[p + 1]);
						glBufferData(GL_PIXEL_UNPACK_BUFFER, w * h, nullptr, GL_DYNAMIC_DRAW);
					}
					glBindTexture(GL_TEXTURE_2D, textures[p + 1]);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);
				}

				/* Prepare texture coordinates */
				texCoordYCbCr[2] = texCoordYCbCr[6] = (videoFrame.linesize[0] == widths[0]) ? 1.0f : (widths[0] / (videoFrame.linesize[0] + 1.0f));

				if (hqScaling)
					maybeSetMipmaps(widget()->devicePixelRatioF());
			}
			resetDone = true;
			hasImage = false;
		}

		if (hwAccellnterface)
		{
			const HWAccelInterface::Field field = (HWAccelInterface::Field)Functions::getField(videoFrame, Deinterlace, HWAccelInterface::FullFrame, HWAccelInterface::TopField, HWAccelInterface::BottomField);
			bool imageReady = false;
			if (!hwAccelError)
			{
				const HWAccelInterface::CopyResult res = hwAccellnterface->copyFrame(videoFrame, field);
				if (res == HWAccelInterface::CopyOk)
					imageReady = true;
				else if (res == HWAccelInterface::CopyError)
				{
					QMPlay2Core.logError("OpenGL 2 :: " + hwAccellnterface->name() + " " + tr("texture copy error"));
					hwAccelError = true;
				}
			}
			hwAccellnterface->unlock();
			if (!imageReady && !hasImage)
				return;
			for (int p = 0; p < numPlanes; ++p)
			{
				glActiveTexture(GL_TEXTURE0 + p);
				glBindTexture(target, textures[p + 1]);
				if (m_useMipmaps)
					glGenerateMipmap(target);
			}
		}
		else
		{
			/* Load textures */
			for (qint32 p = 0; p < 3; ++p)
			{
				const quint8 *data = videoFrame.buffer[p].constData();
				const GLsizei w = correctLinesize ? videoFrame.linesize[p] : widths[p];
				const GLsizei h = heights[p];
				if (hasPbo)
				{
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[p + 1]);
					quint8 *dst;
					if (glMapBufferRange)
						dst = (quint8 *)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, w * h, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
					else
						dst = (quint8 *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
					if (!dst)
						glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
					else
					{
						if (correctLinesize)
							memcpy(dst, data, w * h);
						else for (int y = 0; y < h; ++y)
						{
							memcpy(dst, data, w);
							data += videoFrame.linesize[p];
							dst  += w;
						}
						glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
						data = nullptr;
					}
				}
				glActiveTexture(GL_TEXTURE0 + p);
				glBindTexture(GL_TEXTURE_2D, textures[p + 1]);
				if (hasPbo || correctLinesize)
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
				else for (int y = 0; y < h; ++y)
				{
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, w, 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
					data += videoFrame.linesize[p];
				}
				if (m_useMipmaps)
					glGenerateMipmap(GL_TEXTURE_2D);
			}
			if (hasPbo)
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		}

		videoFrame.clear();
		hasImage = true;
	}

	if (!sphericalView)
	{
		deleteSphereVbo();
		shaderProgramVideo->setAttributeArray(positionYCbCrLoc, verticesYCbCr[verticesIdx], 2);
		shaderProgramVideo->setAttributeArray(texCoordYCbCrLoc, texCoordYCbCr, 2);
	}
	else
	{
		if (nIndices == 0)
			loadSphere();

		glBindBuffer(GL_ARRAY_BUFFER, sphereVbo[0]);
		shaderProgramVideo->setAttributeBuffer(positionYCbCrLoc, GL_FLOAT, 0, 3);

		glBindBuffer(GL_ARRAY_BUFFER, sphereVbo[1]);
		shaderProgramVideo->setAttributeBuffer(texCoordYCbCrLoc, GL_FLOAT, 0, 2);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	shaderProgramVideo->enableAttributeArray(positionYCbCrLoc);
	shaderProgramVideo->enableAttributeArray(texCoordYCbCrLoc);

	shaderProgramVideo->bind();
	if (doReset)
	{
		const float brightness = videoAdjustment.brightness / 100.0f;
		const float contrast   = (videoAdjustment.contrast + 100) / 100.0f;
		const float sharpness  = videoAdjustment.sharpness / 50.0f;
		if (hwAccellnterface && numPlanes == 1)
		{
			hwAccellnterface->setVideAdjustment(videoAdjustment);
			const bool hasBrightness = videoAdjustmentKeys.contains("Brightness");
			const bool hasContrast   = videoAdjustmentKeys.contains("Contrast");
			const bool hasSharpness  = videoAdjustmentKeys.contains("Sharpness");
			shaderProgramVideo->setUniformValue
			(
				"uVideoAdj",
				hasBrightness ? 0.0f : brightness,
				hasContrast   ? 1.0f : contrast,
				hasSharpness  ? 0.0f : sharpness
			);
		}
		else
		{
			const float saturation = (videoAdjustment.saturation + 100) / 100.0f;
			const float hue = videoAdjustment.hue / -31.831f;
			shaderProgramVideo->setUniformValue("uVideoEq", brightness, contrast, saturation, hue);
			shaderProgramVideo->setUniformValue("uSharpness", sharpness);
		}
		if (hqScaling)
		{
			const qreal dpr = widget()->devicePixelRatioF();
			if (!resetDone)
				maybeSetMipmaps(dpr);
			const bool useBicubic = (W * dpr > m_textureSize.width() || H * dpr > m_textureSize.height());
			shaderProgramVideo->setUniformValue("uBicubic", useBicubic ? 1 : 0);
		}
		shaderProgramVideo->setUniformValue("uTextureSize", m_textureSize);

		doReset = !resetDone;
		setMatrix = true;
	}
	if (setMatrix)
	{
		QMatrix4x4 matrix;
		if (!sphericalView)
		{
			matrix.scale(W / (qreal)winSize.width(), H / (qreal)winSize.height());
			if (!videoOffset.isNull())
				matrix.translate(-videoOffset.x(), videoOffset.y());
		}
		else
		{
			const double z = qBound(-1.0, (zoom > 1.0 ? log10(zoom) : zoom - 1.0), 0.99);
			matrix.perspective(68.0, (qreal)winSize.width() / (qreal)winSize.height(), 0.001, 2.0);
			matrix.translate(0.0, 0.0, z);
			matrix.rotate(rot.x(), 1.0, 0.0, 0.0);
			matrix.rotate(rot.y(), 0.0, 0.0, 1.0);
		}
		shaderProgramVideo->setUniformValue("uMatrix", matrix);
		setMatrix = false;
	}
	if (!sphericalView)
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	else
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereVbo[2]);
		glDrawElements(GL_TRIANGLE_STRIP, nIndices, GL_UNSIGNED_SHORT, nullptr);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	shaderProgramVideo->release();

	shaderProgramVideo->disableAttributeArray(texCoordYCbCrLoc);
	shaderProgramVideo->disableAttributeArray(positionYCbCrLoc);

	glActiveTexture(GL_TEXTURE3);

	/* OSD */
	osdMutex.lock();
	if (!osdList.isEmpty())
	{
		glBindTexture(GL_TEXTURE_2D, textures[0]);

		QRect bounds;
		const qreal scaleW = (qreal)subsW / outW, scaleH = (qreal)subsH / outH;
		bool mustRepaint = Functions::mustRepaintOSD(osdList, osd_ids, &scaleW, &scaleH, &bounds);
		bool hasNewSize = false;
		if (!mustRepaint)
			mustRepaint = osdImg.size() != bounds.size();
		if (mustRepaint)
		{
			if (osdImg.size() != bounds.size())
			{
				osdImg = QImage(bounds.size(), QImage::Format_ARGB32);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bounds.width(), bounds.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
				hasNewSize = true;
			}
			osdImg.fill(0);
			QPainter p(&osdImg);
			p.translate(-bounds.topLeft());
			Functions::paintOSD(false, osdList, scaleW, scaleH, p, &osd_ids);
			const quint8 *data = osdImg.constBits();
			if (hasPbo)
			{
				const GLsizeiptr dataSize = (osdImg.width() * osdImg.height()) << 2;
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[0]);
				if (hasNewSize)
					glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, nullptr, GL_DYNAMIC_DRAW);
				quint8 *dst;
				if (glMapBufferRange)
					dst = (quint8 *)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
				else
					dst = (quint8 *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
				if (!dst)
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
				else
				{
					memcpy(dst, data, dataSize);
					glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
					data = nullptr;
				}
			}
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bounds.width(), bounds.height(), GL_RGBA, GL_UNSIGNED_BYTE, data);
			if (hasPbo && !data)
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		}

		const QSizeF winSizeSubs = winSize * widget()->devicePixelRatioF();
		const float left   = (bounds.left() + subsX) * 2.0f / winSizeSubs.width() - osdOffset.x();
		const float right  = (bounds.right() + subsX + 1) * 2.0f / winSizeSubs.width() - osdOffset.x();
		const float top    = (bounds.top() + subsY) * 2.0f / winSizeSubs.height() - osdOffset.y();
		const float bottom = (bounds.bottom() + subsY + 1) * 2.0f / winSizeSubs.height() - osdOffset.y();
		const float verticesOSD[8] = {
			left  - 1.0f, -bottom + 1.0f,
			right - 1.0f, -bottom + 1.0f,
			left  - 1.0f, -top    + 1.0f,
			right - 1.0f, -top    + 1.0f,
		};

		shaderProgramOSD->setAttributeArray(positionOSDLoc, verticesOSD, 2);
		shaderProgramOSD->setAttributeArray(texCoordOSDLoc, texCoordOSD, 2);
		shaderProgramOSD->enableAttributeArray(positionOSDLoc);
		shaderProgramOSD->enableAttributeArray(texCoordOSDLoc);

		glEnable(GL_BLEND);
		shaderProgramOSD->bind();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		shaderProgramOSD->release();
		glDisable(GL_BLEND);

		shaderProgramOSD->disableAttributeArray(texCoordOSDLoc);
		shaderProgramOSD->disableAttributeArray(positionOSDLoc);
	}
	osdMutex.unlock();

	glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGL2Common::contextAboutToBeDestroyed()
{
	if (hwAccellnterface && hwAccellnterface->lock())
	{
		hwAccellnterface->clear(true);
		hwAccellnterface->unlock();
	}
	deleteSphereVbo();
	if (hasPbo)
		glDeleteBuffers(1 + (hwAccellnterface ? 0 : numPlanes), pbo);
	glDeleteTextures(numPlanes + 1, textures);
}

void OpenGL2Common::testGLInternal()
{
	int glMajor = 0, glMinor = 0;
#ifndef OPENGL_ES2
	glGetIntegerv(GL_MAJOR_VERSION, &glMajor);
	glGetIntegerv(GL_MINOR_VERSION, &glMinor);
#endif
#ifndef Q_OS_MACOS //On macOS I have always OpenGL 2.1...
	if (!glMajor)
	{
		const QString glVersionStr = (const char *)glGetString(GL_VERSION);
		const int dotIdx = glVersionStr.indexOf('.');
		if (dotIdx > 0)
		{
			const int vIdx = glVersionStr.lastIndexOf(' ', dotIdx);
			if (sscanf(glVersionStr.mid(vIdx < 0 ? 0 : vIdx).toLatin1().constData(), "%d.%d", &glMajor, &glMinor) != 2)
				glMajor = glMinor = 0;
		}
	}
	if (glMajor)
		glVer = glMajor * 10 + glMinor;
	canUseHueSharpness = (glVer >= 30);
#endif

#ifndef OPENGL_ES2
	initGLProc(); //No need to call it here for OpenGL|ES
	if (!canCreateNonPowerOfTwoTextures || !supportsShaders || !glActiveTexture)
	{
		showOpenGLMissingFeaturesMessage();
		isOK = false;
	}
	/* Reset variables */
	supportsShaders = canCreateNonPowerOfTwoTextures = false;
	glActiveTexture = nullptr;
#endif

	numPlanes = 3;
	target = GL_TEXTURE_2D;
	if (hwAccellnterface)
	{
		switch (hwAccellnterface->getFormat())
		{
			case HWAccelInterface::NV12:
				numPlanes = 2;
				break;
			case HWAccelInterface::RGB32:
				numPlanes = 1;
				break;
		}

		if (hwAccellnterface->isTextureRectangle())
		{
			target = GL_TEXTURE_RECTANGLE_ARB;
			if (numPlanes == 1)
				isOK = false; // Not used and not supported
			hqScaling = false; // Not yet supported
		}

		if (isOK)
		{
			quint32 textures[numPlanes];
			memset(textures, 0, sizeof textures);
			glGenTextures(numPlanes, textures);
			if (hwAccellnterface->canInitializeTextures())
			{
				for (int p = 0; p < numPlanes; ++p)
				{
					glBindTexture(target, textures[p]);
					if (numPlanes == 2)
						glTexImage2D(target, 0, !p ? GL_R8 : GL_RG8, 1, 1, 0, !p ? GL_RED : GL_RG, GL_UNSIGNED_BYTE, nullptr);
					else if (numPlanes == 1)
						glTexImage2D(target, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
				}
			}

			if (!hwAccellnterface->lock())
				isOK = false;
			else
			{
				if (!hwAccellnterface->init(textures))
					isOK = false;
				if (numPlanes == 1) //For RGB32 format, HWAccel should be able to adjust the video
				{
					VideoAdjustment videoAdjustmentCap;
					hwAccellnterface->getVideAdjustmentCap(videoAdjustmentCap);
					if (videoAdjustmentCap.brightness)
						videoAdjustmentKeys += "Brightness";
					if (videoAdjustmentCap.contrast)
						videoAdjustmentKeys += "Contrast";
					if (videoAdjustmentCap.saturation)
						videoAdjustmentKeys += "Saturation";
					if (videoAdjustmentCap.hue)
						videoAdjustmentKeys += "Hue";
					if (videoAdjustmentCap.sharpness)
						videoAdjustmentKeys += "Sharpness";
				}
				hwAccellnterface->clear(true);
				hwAccellnterface->unlock();
			}

			glDeleteTextures(numPlanes, textures);
		}
	}

	QWidget *w = widget();
	w->grabGesture(Qt::PinchGesture);
	w->setMouseTracking(true);
#ifdef Q_OS_WIN
	/*
	 * This property is read by QMPlay2 and it ensures that toolbar will be visible
	 * on fullscreen in Windows Vista and newer on nVidia and AMD drivers.
	*/
	const bool canPreventFullScreen = (qstrcmp(w->metaObject()->className(), "QOpenGLWidget") != 0);
	const QSysInfo::WinVersion winVer = QSysInfo::windowsVersion();
	if (canPreventFullScreen && winVer >= QSysInfo::WV_6_0)
	{
		Qt::CheckState compositionEnabled;
		if (!preventFullScreen)
			compositionEnabled = Qt::PartiallyChecked;
		else
		{
			compositionEnabled = Qt::Checked;
			if (winVer <= QSysInfo::WV_6_1) //Windows 8 and 10 can't disable DWM composition
			{
				using DwmIsCompositionEnabledProc = HRESULT (WINAPI *)(BOOL *pfEnabled);
				DwmIsCompositionEnabledProc DwmIsCompositionEnabled = (DwmIsCompositionEnabledProc)GetProcAddress(GetModuleHandleA("dwmapi.dll"), "DwmIsCompositionEnabled");
				if (DwmIsCompositionEnabled)
				{
					BOOL enabled = false;
					if (DwmIsCompositionEnabled(&enabled) == S_OK && !enabled)
						compositionEnabled = Qt::PartiallyChecked;
				}
			}
		}
		w->setProperty("preventFullScreen", (int)compositionEnabled);
	}
#endif
}

void OpenGL2Common::initGLProc()
{
#ifndef OPENGL_ES2
	const char *glExtensions = (const char *)glGetString(GL_EXTENSIONS);
	if (glExtensions)
	{
		supportsShaders = !!strstr(glExtensions, "GL_ARB_vertex_shader") && !!strstr(glExtensions, "GL_ARB_fragment_shader") && !!strstr(glExtensions, "GL_ARB_shader_objects");
		canCreateNonPowerOfTwoTextures = !!strstr(glExtensions, "GL_ARB_texture_non_power_of_two");
	}
	glActiveTexture = (GLActiveTexture)QOpenGLContext::currentContext()->getProcAddress("glActiveTexture");
	glGenBuffers = (GLGenBuffers)QOpenGLContext::currentContext()->getProcAddress("glGenBuffers");
	glBindBuffer = (GLBindBuffer)QOpenGLContext::currentContext()->getProcAddress("glBindBuffer");
	glBufferData = (GLBufferData)QOpenGLContext::currentContext()->getProcAddress("glBufferData");
	glDeleteBuffers = (GLDeleteBuffers)QOpenGLContext::currentContext()->getProcAddress("glDeleteBuffers");
	if (hqScaling)
		glGenerateMipmap = (GLGenerateMipmap)QOpenGLContext::currentContext()->getProcAddress("glGenerateMipmap");
	hasVbo = glGenBuffers && glBindBuffer && glBufferData && glDeleteBuffers;
#endif
	if (allowPBO)
	{
		glMapBufferRange = (GLMapBufferRange)QOpenGLContext::currentContext()->getProcAddress("glMapBufferRange");
		glMapBuffer = (GLMapBuffer)QOpenGLContext::currentContext()->getProcAddress("glMapBuffer");
		glUnmapBuffer = (GLUnmapBuffer)QOpenGLContext::currentContext()->getProcAddress("glUnmapBuffer");
	}
	hasPbo = hasVbo && (glMapBufferRange || glMapBuffer) && glUnmapBuffer;
}
#ifndef OPENGL_ES2
void OpenGL2Common::showOpenGLMissingFeaturesMessage()
{
	fprintf
	(
		stderr,
		"GL_ARB_texture_non_power_of_two : %s\n"
		"Vertex & fragment shader: %s\n"
		"glActiveTexture: %s\n",
		canCreateNonPowerOfTwoTextures ? "yes" : "no",
		supportsShaders ? "yes" : "no",
		glActiveTexture ? "yes" : "no"
	);
	QMPlay2Core.logError("OpenGL 2 :: " + tr("Driver must support multitexturing, shaders and Non-Power-Of-Two texture size"), true, true);
}
#endif

void OpenGL2Common::dispatchEvent(QEvent *e, QObject *p)
{
	switch (e->type())
	{
		case QEvent::MouseButtonPress:
			if (sphericalView)
				mousePress360((QMouseEvent *)e);
			else
				mousePress((QMouseEvent *)e);
			break;
		case QEvent::MouseButtonRelease:
			if (sphericalView)
				mouseRelease360((QMouseEvent *)e);
			else
				mouseRelease((QMouseEvent *)e);
			break;
		case QEvent::MouseMove:
			if (sphericalView)
				mouseMove360((QMouseEvent *)e);
			else
				mouseMove((QMouseEvent *)e);
			break;
		case QEvent::Resize:
			newSize(((QResizeEvent *)e)->size());
			break;
		case QEvent::TouchBegin:
		case QEvent::TouchUpdate:
			canWrapMouse = false;
			//Pass through
		case QEvent::TouchEnd:
		case QEvent::Gesture:
			/* Pass gesture and touch event to the parent */
			QCoreApplication::sendEvent(p, e);
			break;
		default:
			break;
	}
}

void OpenGL2Common::maybeSetMipmaps(qreal dpr)
{
	const bool lastUseMipmaps = m_useMipmaps;
	m_useMipmaps = (W * dpr < m_textureSize.width() || H * dpr < m_textureSize.height());
#ifndef OPENGL_ES2
	if (m_useMipmaps && !glGenerateMipmap)
	{
		QMPlay2Core.logError("OpenGL 2 :: Mipmaps requested, but driver doesn't support it!", true, true);
		m_useMipmaps = false;
	}
#endif
	if (m_useMipmaps != lastUseMipmaps)
	{
		for (int p = 0; p < numPlanes; ++p)
		{
			glBindTexture(target, textures[p + 1]);
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, m_useMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
			if (m_useMipmaps)
				glGenerateMipmap(target);
		}
	}
}

inline bool OpenGL2Common::isRotate90() const
{
	return verticesIdx >= 4 && !sphericalView;
}

inline bool OpenGL2Common::hwAccellPossibleLock()
{
	if (hwAccellnterface && !hwAccellnterface->lock())
	{
		QMPlay2Core.logError("OpenGL 2 :: " + hwAccellnterface->name() + " " + tr("error"));
		hwAccelError = true;
		return false;
	}
	return true;
}

QByteArray OpenGL2Common::readShader(const QString &fileName, bool pure)
{
	QResource res(fileName);
	QByteArray shader;
	if (!pure)
	{
#ifdef OPENGL_ES2
		shader = "precision highp float;\n";
#endif
		shader.append("#line 1\n");
	}
	shader.append((const char *)res.data(), res.size());
	return shader;
}

void OpenGL2Common::mousePress(QMouseEvent *e)
{
	if (e->buttons() & Qt::LeftButton)
	{
		moveVideo = (e->modifiers() & Qt::ShiftModifier);
		moveOSD = (e->modifiers() & Qt::ControlModifier);
		if (moveVideo || moveOSD)
		{
			QWidget *w = widget();
			w->setProperty("customCursor", (int)Qt::ArrowCursor);
			w->setCursor(Qt::ClosedHandCursor);
			mousePos = e->pos();
		}
	}
}
void OpenGL2Common::mouseMove(QMouseEvent *e)
{
	if ((moveVideo || moveOSD) && (e->buttons() & Qt::LeftButton))
	{
		const QPoint newMousePos = e->pos();
		const QPointF mouseDiff = mousePos - newMousePos;

		if (moveVideo)
			videoOffset += QPointF(mouseDiff.x() * 2.0 / W, mouseDiff.y() * 2.0 / H);
		if (moveOSD)
		{
			QWidget *w = widget();
			osdOffset += QPointF(mouseDiff.x() * 2.0 / w->width(), mouseDiff.y() * 2.0 / w->height());
		}

		mousePos = newMousePos;

		setMatrix = true;
		updateGL(true);
	}
}
void OpenGL2Common::mouseRelease(QMouseEvent *e)
{
	if ((moveVideo || moveOSD) && e->button() == Qt::LeftButton)
	{
		QWidget *w = widget();
		w->unsetCursor();
		w->setProperty("customCursor", QVariant());
		moveVideo = moveOSD = false;
	}
}

/* 360 */

void OpenGL2Common::mousePress360(QMouseEvent *e)
{
	if (e->buttons() & Qt::LeftButton)
	{
		widget()->setCursor(Qt::ClosedHandCursor);
		mouseTime = Functions::gettime();
		buttonPressed = true;
		rotAnimation.stop();
		mousePos = e->pos();
	}
}
void OpenGL2Common::mouseMove360(QMouseEvent *e)
{
	if (mouseWrapped)
		mouseWrapped = false;
	else if (buttonPressed && (e->buttons() & Qt::LeftButton))
	{
		const QPoint newMousePos = e->pos();
		const QPointF mouseDiff = QPointF(mousePos - newMousePos) / 10.0;

		rot.setX(qBound<qreal>(0.0, (rot.rx() += mouseDiff.y()), 180.0));
		rot.ry() -= mouseDiff.x();

		const double currTime = Functions::gettime();
		const double mouseTimeDiff = qMax(currTime - mouseTime, 0.001);
		const QPointF movPerSec(mouseDiff.y() / mouseTimeDiff / 5.0, -mouseDiff.x() / mouseTimeDiff / 5.0);
		if (rotAnimation.state() != QAbstractAnimation::Stopped)
			rotAnimation.stop();
		rotAnimation.setEndValue(rot + movPerSec);
		mouseTime = currTime;

		mousePos = newMousePos;
		if (e->source() == Qt::MouseEventNotSynthesized)
		{
			if (canWrapMouse)
				mouseWrapped = Functions::wrapMouse(widget(), mousePos, 1);
			else
				canWrapMouse = true;
		}

		setMatrix = true;
		updateGL(true);
	}
}
void OpenGL2Common::mouseRelease360(QMouseEvent *e)
{
	if (buttonPressed && e->button() == Qt::LeftButton)
	{
		if ((Functions::gettime() - mouseTime) >= 0.075)
			rotAnimation.stop();
		else
		{
			rotAnimation.setStartValue(rot);
			rotAnimation.start();
		}
		widget()->setCursor(Qt::OpenHandCursor);
		buttonPressed = false;
	}
}
inline void OpenGL2Common::resetSphereVbo()
{
	memset(sphereVbo, 0, sizeof sphereVbo);
	nIndices = 0;
}
inline void OpenGL2Common::deleteSphereVbo()
{
	if (nIndices > 0)
	{
		glDeleteBuffers(3, sphereVbo);
		resetSphereVbo();
	}
}
void OpenGL2Common::loadSphere()
{
	const quint32 slices = 50;
	const quint32 stacks = 50;
	const GLenum targets[3] = {
		GL_ARRAY_BUFFER,
		GL_ARRAY_BUFFER,
		GL_ELEMENT_ARRAY_BUFFER
	};
	void *pointers[3];
	quint32 sizes[3];
	nIndices = Sphere::getSizes(slices, stacks, sizes[0], sizes[1], sizes[2]);
	glGenBuffers(3, sphereVbo);
	for (qint32 i = 0; i < 3; ++i)
		pointers[i] = malloc(sizes[i]);
	Sphere::generate(1.0f, slices, stacks, (float *)pointers[0], (float *)pointers[1], (quint16 *)pointers[2]);
	for (qint32 i = 0; i < 3; ++i)
	{
		glBindBuffer(targets[i], sphereVbo[i]);
		glBufferData(targets[i], sizes[i], pointers[i], GL_STATIC_DRAW);
		free(pointers[i]);
	}
}
