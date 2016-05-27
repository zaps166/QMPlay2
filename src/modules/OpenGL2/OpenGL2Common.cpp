#include <OpenGL2Common.hpp>
#include <Sphere.hpp>

#include <QMPlay2Core.hpp>
#include <VideoFrame.hpp>
#include <Functions.hpp>

#ifdef OPENGL_NEW_API
	#include <QOpenGLContext>
	#include <QOpenGLShader>
#else
	#include <QGLContext>
	#include <QGLShader>
	#define QOpenGLContext QGLContext
	#define QOpenGLShader QGLShader
#endif
#include <QResizeEvent>
#include <QMatrix4x4>
#include <QPainter>
#include <QWidget>

#include <Vertices.hpp>
#include <Shaders.hpp>

#include <math.h>

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
	glActiveTexture(NULL),
#endif
#ifdef Q_OS_WIN
	preventFullscreen(false),
#endif
#ifdef VSYNC_SETTINGS
	vSync(true),
#endif
	shaderProgramYCbCr(NULL), shaderProgramOSD(NULL),
	texCoordYCbCrLoc(-1), positionYCbCrLoc(-1), texCoordOSDLoc(-1), positionOSDLoc(-1),
	Contrast(-1), Saturation(-1), Brightness(-1), Hue(-1),
	isPaused(false), isOK(false), hasImage(false), doReset(true), setMatrix(true),
	subsX(-1), subsY(-1), W(-1), H(-1), subsW(-1), subsH(-1), outW(-1), outH(-1), verticesIdx(0),
	glVer(0), doClear(0),
	aspectRatio(0.0), zoom(0.0),
	sphericalView(false), buttonPressed(false), hasVbo(true), mouseWrapped(false), canWrapMouse(true),
	rotAnimation(*this),
	mouseTime(0.0)
{
	/* Initialize texCoordYCbCr array */
	texCoordYCbCr[0] = texCoordYCbCr[4] = texCoordYCbCr[5] = texCoordYCbCr[7] = 0.0f;
	texCoordYCbCr[1] = texCoordYCbCr[3] = 1.0f;

	/* Set 360° view */
	rotAnimation.setEasingCurve(QEasingCurve::OutQuint);
	rotAnimation.setDuration(1000.0);
}
OpenGL2Common::~OpenGL2Common()
{
	delete shaderProgramYCbCr;
	delete shaderProgramOSD;
}

void OpenGL2Common::deleteMe()
{
	delete this;
}

void OpenGL2Common::newSize(const QSize &size)
{
	const bool canUpdate = !size.isValid();
	const QSize winSize = canUpdate ? widget()->size() : size;
	if (!isRotate90())
	{
		Functions::getImageSize(aspectRatio, zoom, winSize.width(), winSize.height(), W, H, &subsX, &subsY);
		subsW = W;
		subsH = H;
	}
	else
	{
		Functions::getImageSize(aspectRatio, zoom, winSize.height(), winSize.width(), H, W);
		Functions::getImageSize(aspectRatio, zoom, winSize.width(), winSize.height(), subsW, subsH, &subsX, &subsY);
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
	osdChecksums.clear();
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

void OpenGL2Common::resetClearCounter()
{
	doClear = 6;
}

void OpenGL2Common::initializeGL()
{
#ifndef OPENGL_ES2
	initGLProc();
	if (!glActiveTexture) //Be sure that "glActiveTexture" has valid pointer (don't check "supportsShaders" here)!
	{
		showOpenGLMissingFeaturesMessage();
		isOK = false;
		return;
	}
#endif

#ifndef DONT_RECREATE_SHADERS
	delete shaderProgramYCbCr;
	delete shaderProgramOSD;
	shaderProgramYCbCr = shaderProgramOSD = NULL;
#endif
	if (!shaderProgramYCbCr)
		shaderProgramYCbCr = new QOpenGLShaderProgram;
	if (!shaderProgramOSD)
		shaderProgramOSD = new QOpenGLShaderProgram;

	/* YCbCr shader */
	if (shaderProgramYCbCr->shaders().isEmpty())
	{
		shaderProgramYCbCr->addShaderFromSourceCode(QOpenGLShader::Vertex, vShaderYCbCrSrc);
		/* Use hue only when OpenGL/OpenGL|ES version >= 3.0, because it can be slow on old hardware and/or buggy drivers and may increase CPU usage! */
		shaderProgramYCbCr->addShaderFromSourceCode(QOpenGLShader::Fragment, QString(fShaderYCbCrSrc).arg((glVer >= 30) ? fShaderYCbCrHueSrc : ""));
	}
	if (shaderProgramYCbCr->bind())
	{
		const qint32 newTexCoordLoc = shaderProgramYCbCr->attributeLocation("aTexCoord");
		const qint32 newPositionLoc = shaderProgramYCbCr->attributeLocation("aPosition");
		if (newTexCoordLoc != newPositionLoc) //If new locations are invalid, just leave them untouched...
		{
			texCoordYCbCrLoc = newTexCoordLoc;
			positionYCbCrLoc = newPositionLoc;
		}
		shaderProgramYCbCr->setUniformValue("Ytex", 0);
		shaderProgramYCbCr->setUniformValue("Utex", 1);
		shaderProgramYCbCr->setUniformValue("Vtex", 2);
		shaderProgramYCbCr->release();
	}
	else
	{
		QMPlay2Core.logError(tr("Shader compile/link error"));
		isOK = false;
		return;
	}

	/* OSD shader */
	if (shaderProgramOSD->shaders().isEmpty())
	{
		shaderProgramOSD->addShaderFromSourceCode(QOpenGLShader::Vertex, vShaderOSDSrc);
		shaderProgramOSD->addShaderFromSourceCode(QOpenGLShader::Fragment, fShaderOSDSrc);
	}
	if (shaderProgramOSD->bind())
	{
		const qint32 newTexCoordLoc = shaderProgramOSD->attributeLocation("aTexCoord");
		const qint32 newPositionLoc = shaderProgramOSD->attributeLocation("aPosition");
		if (newTexCoordLoc != newPositionLoc) //If new locations are invalid, just leave them untouched...
		{
			texCoordOSDLoc = newTexCoordLoc;
			positionOSDLoc = newPositionLoc;
		}
		shaderProgramOSD->setUniformValue("tex", 3);
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
	glGenTextures(4, textures);
	for (int i = 0; i < 4; ++i)
	{
		glBindTexture(GL_TEXTURE_2D, textures[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, i == 0 ? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, i == 0 ? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	setVSync(vSync);

	doReset = true;
	resetSphereVbo();
}

void OpenGL2Common::paintGL()
{
	if (videoFrame.isEmpty() && !hasImage)
		return;

	const QSize winSize = widget()->size();

	bool resetDone = false;

	if (!videoFrame.isEmpty())
	{
		const GLsizei widths[3] = {
			videoFrame.size.width,
			videoFrame.size.chromaWidth,
			videoFrame.size.chromaWidth,
		};
		const GLsizei heights[3] = {
			videoFrame.size.height,
			videoFrame.size.chromaHeight,
			videoFrame.size.chromaHeight
		};

		if (doReset)
		{
			/* Prepare textures */
			for (qint32 p = 0; p < 3; ++p)
			{
				glBindTexture(GL_TEXTURE_2D, textures[p + 1]);
				if (checkLinesize(p))
					glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, videoFrame.linesize[p], heights[p], 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
				else
					glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, widths[p], heights[p], 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
			}

			/* Prepare texture coordinates */
			texCoordYCbCr[2] = texCoordYCbCr[6] = (videoFrame.linesize[0] == widths[0]) ? 1.0f : (widths[0] / (videoFrame.linesize[0] + 1.0f));

			resetDone = true;
		}

		/* Load textures */
		for (qint32 p = 0; p < 3; ++p)
		{
			glActiveTexture(GL_TEXTURE0 + p);
			glBindTexture(GL_TEXTURE_2D, textures[p + 1]);
			if (checkLinesize(p) || videoFrame.linesize[p] == widths[p])
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, videoFrame.linesize[p], heights[p], GL_LUMINANCE, GL_UNSIGNED_BYTE, videoFrame.buffer[p].constData());
			else
			{
				const quint8 *data = videoFrame.buffer[p].constData();
				for (int y = 0; y < heights[p]; ++y)
				{
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, widths[p], 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
					data += videoFrame.linesize[p];
				}
			}
		}

		videoFrame.clear();
		hasImage = true;
	}

	if (!sphericalView)
	{
		if (nIndices > 0)
		{
			glDeleteBuffers(3, sphereVbo);
			resetSphereVbo();
		}
		shaderProgramYCbCr->setAttributeArray(positionYCbCrLoc, verticesYCbCr[verticesIdx], 2);
		shaderProgramYCbCr->setAttributeArray(texCoordYCbCrLoc, texCoordYCbCr, 2);
	}
	else
	{
		if (nIndices == 0)
			loadSphere();

		glBindBuffer(GL_ARRAY_BUFFER, sphereVbo[0]);
		shaderProgramYCbCr->setAttributeBuffer(positionYCbCrLoc, GL_FLOAT, 0, 3);

		glBindBuffer(GL_ARRAY_BUFFER, sphereVbo[1]);
		shaderProgramYCbCr->setAttributeBuffer(texCoordYCbCrLoc, GL_FLOAT, 0, 2);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	shaderProgramYCbCr->enableAttributeArray(positionYCbCrLoc);
	shaderProgramYCbCr->enableAttributeArray(texCoordYCbCrLoc);

	shaderProgramYCbCr->bind();
	if (doReset)
	{
		shaderProgramYCbCr->setUniformValue("videoEq", Brightness, Contrast, Saturation, Hue);
		doReset = !resetDone;
		setMatrix = true;
	}
	if (setMatrix)
	{
		QMatrix4x4 matrix;
		if (!sphericalView)
			matrix.scale(W / (qreal)winSize.width(), H / (qreal)winSize.height());
		else
		{
			const double z = qBound(-1.0, (zoom > 1.0 ? log10(zoom) : zoom - 1.0), 0.99);
			matrix.perspective(68.0, (qreal)winSize.width() / (qreal)winSize.height(), 0.001, 2.0);
			matrix.translate(0.0, 0.0, z);
			matrix.rotate(rot.x(), 1.0, 0.0, 0.0);
			matrix.rotate(rot.y(), 0.0, 0.0, 1.0);
		}
		shaderProgramYCbCr->setUniformValue("matrix", matrix);
		setMatrix = false;
	}
	if (!sphericalView)
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	else
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereVbo[2]);
		glDrawElements(GL_TRIANGLE_STRIP, nIndices, GL_UNSIGNED_SHORT, NULL);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	shaderProgramYCbCr->release();

	shaderProgramYCbCr->disableAttributeArray(texCoordYCbCrLoc);
	shaderProgramYCbCr->disableAttributeArray(positionYCbCrLoc);

	glActiveTexture(GL_TEXTURE3);

	/* OSD */
	osdMutex.lock();
	if (!osdList.isEmpty())
	{
		glBindTexture(GL_TEXTURE_2D, textures[0]);

		QRect bounds;
		const qreal scaleW = (qreal)subsW / outW, scaleH = (qreal)subsH / outH;
		bool mustRepaint = Functions::mustRepaintOSD(osdList, osdChecksums, &scaleW, &scaleH, &bounds);
		if (!mustRepaint)
			mustRepaint = osdImg.size() != bounds.size();
		if (mustRepaint)
		{
			if (osdImg.size() != bounds.size())
				osdImg = QImage(bounds.size(), QImage::Format_ARGB32);
			osdImg.fill(0);
			QPainter p(&osdImg);
			p.translate(-bounds.topLeft());
			Functions::paintOSD(false, osdList, scaleW, scaleH, p, &osdChecksums);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bounds.width(), bounds.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, osdImg.bits());
		}

		const float left   = (bounds.left() + subsX) * 2.0f / winSize.width();
		const float right  = (bounds.right() + subsX + 1) * 2.0f / winSize.width();
		const float top    = (bounds.top() + subsY) * 2.0f / winSize.height();
		const float bottom = (bounds.bottom() + subsY + 1) * 2.0f / winSize.height();
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

	if (updateTimer.isActive())
		updateTimer.stop();
}

void OpenGL2Common::testGLInternal()
{
	int glMajor = 0, glMinor = 0;
#ifndef OPENGL_ES2
	glGetIntegerv(GL_MAJOR_VERSION, &glMajor);
	glGetIntegerv(GL_MINOR_VERSION, &glMinor);
#endif
	if (!glMajor)
	{
		const QString glVersionStr = (const char *)glGetString(GL_VERSION);
		const int dotIdx = glVersionStr.indexOf('.');
		if (dotIdx > 0)
		{
			const int vIdx = glVersionStr.lastIndexOf(' ', dotIdx);
			if (sscanf(glVersionStr.mid(vIdx < 0 ? 0 : vIdx).toLatin1().data(), "%d.%d", &glMajor, &glMinor) != 2)
				glMajor = glMinor = 0;
		}
	}
	if (glMajor)
		glVer = glMajor * 10 + glMinor;

#ifndef OPENGL_ES2
	initGLProc();
	if (!canCreateNonPowerOfTwoTextures || !supportsShaders || !glActiveTexture)
	{
		showOpenGLMissingFeaturesMessage();
		isOK = false;
	}
	/* Reset variables */
	supportsShaders = canCreateNonPowerOfTwoTextures = false;
	glActiveTexture = NULL;
#endif

	QWidget *w = widget();
	w->grabGesture(Qt::PinchGesture);
	w->setMouseTracking(true);
#ifdef Q_OS_WIN
	/*
	 * This property is read by QMPlay2 and it ensures that toolbar will be visible
	 * on fullscreen in Windows Vista and newer on nVidia and AMD drivers.
	*/
	if (preventFullscreen && QSysInfo::windowsVersion() >= QSysInfo::WV_6_0)
		w->setProperty("PreventFullscreen", true);
#endif
}

#ifndef OPENGL_ES2
void OpenGL2Common::initGLProc()
{
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
	hasVbo = (glGenBuffers && glBindBuffer && glBufferData && glDeleteBuffers);

}
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
			break;
		case QEvent::MouseButtonRelease:
			if (sphericalView)
				mouseRelease360((QMouseEvent *)e);
			break;
		case QEvent::MouseMove:
			if (sphericalView)
				mouseMove360((QMouseEvent *)e);
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

inline bool OpenGL2Common::checkLinesize(int p)
{
	return !sphericalView && (p == 0 || ((videoFrame.linesize[0] >> 1) == videoFrame.linesize[p]));
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
		if (canWrapMouse)
			mouseWrapped = Functions::wrapMouse(widget(), mousePos);
		else
			canWrapMouse = true;

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
void OpenGL2Common::resetSphereVbo()
{
	memset(sphereVbo, 0, sizeof sphereVbo);
	nIndices = 0;
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
