#include <OpenGL2Common.hpp>

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
#include <QPainter>
#include <QWidget>

#if !defined OPENGL_ES2 && !defined Q_OS_MAC
	#include <GL/glext.h>
#endif

static const char vShaderYCbCrSrc[] =
#ifdef OPENGL_ES2
	"precision lowp float;"
#endif
	"attribute vec4 vPosition;"
	"attribute vec2 aTexCoord;"
	"varying vec2 vTexCoord;"
	"uniform vec2 scale;"
	"void main() {"
		"vTexCoord = aTexCoord;"
		"gl_Position = vPosition * vec4(scale.xy, 1, 1);"
	"}";
static const char fShaderYCbCrSrc[] =
#ifdef OPENGL_ES2
	"precision lowp float;"
#endif
	"varying vec2 vTexCoord;"
	"uniform vec4 videoEq;"
	"uniform sampler2D Ytex, Utex, Vtex;"
	"void main() {"
		"float brightness = videoEq[0];"
		"float contrast = videoEq[1];"
		"float saturation = videoEq[2];"
		"vec3 YCbCr = vec3("
			"texture2D(Ytex, vTexCoord)[0] - 0.0625,"
			"texture2D(Utex, vTexCoord)[0] - 0.5,"
			"texture2D(Vtex, vTexCoord)[0] - 0.5"
		");"
		"%1"
		"YCbCr.yz *= saturation;"
		"vec3 rgb = mat3"
		"("
			"1.16430,  1.16430, 1.16430,"
			"0.00000, -0.39173, 2.01700,"
			"1.59580, -0.81290, 0.00000"
		") * YCbCr * contrast + brightness;"
		"gl_FragColor = vec4(rgb, 1.0);"
	"}";
static const char fShaderYCbCrHueSrc[] =
	"float hueAdj = videoEq[3];"
	"if (hueAdj != 0.0) {"
		"float hue = atan(YCbCr[2], YCbCr[1]) + hueAdj;"
		"float chroma = sqrt(YCbCr[1] * YCbCr[1] + YCbCr[2] * YCbCr[2]);"
		"YCbCr[1] = chroma * cos(hue);"
		"YCbCr[2] = chroma * sin(hue);"
	"}";

static const char vShaderOSDSrc[] =
#ifdef OPENGL_ES2
	"precision lowp float;"
#endif
	"attribute vec4 vPosition;"
	"attribute vec2 aTexCoord;"
	"varying vec2 vTexCoord;"
	"void main() {"
		"vTexCoord = aTexCoord;"
		"gl_Position = vPosition;"
	"}";
static const char fShaderOSDSrc[] =
#ifdef OPENGL_ES2
	"precision lowp float;"
#endif
	"varying vec2 vTexCoord;"
	"uniform sampler2D tex;"
	"void main() {"
		"gl_FragColor = texture2D(tex, vTexCoord);"
	"}";

static const float verticesYCbCr[4][8] = {
	/* Normal */
	{
		-1.0f, -1.0f, //0. Left-bottom
		+1.0f, -1.0f, //1. Right-bottom
		-1.0f, +1.0f, //2. Left-top
		+1.0f, +1.0f, //3. Right-top
	},
	/* Horizontal flip */
	{
		+1.0f, -1.0f, //1. Right-bottom
		-1.0f, -1.0f, //0. Left-bottom
		+1.0f, +1.0f, //3. Right-top
		-1.0f, +1.0f, //2. Left-top
	},
	/* Vertical flip */
	{
		-1.0f, +1.0f, //2. Left-top
		+1.0f, +1.0f, //3. Right-top
		-1.0f, -1.0f, //0. Left-bottom
		+1.0f, -1.0f, //1. Right-bottom
	},
	/* Rotated */
	{
		+1.0f, +1.0f, //3. Right-top
		-1.0f, +1.0f, //2. Left-top
		+1.0f, -1.0f, //1. Right-bottom
		-1.0f, -1.0f, //0. Left-bottom
	}
};
static const float texCoordOSD[8] = {
	0.0f, 1.0f,
	1.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
};

/* OpenGLCommon implementation */

OpenGL2Common::OpenGL2Common() :
#ifndef OPENGL_ES2
	supportsShaders(false), canCreateNonPowerOfTwoTextures(false),
	glActiveTexture(NULL),
#endif
#ifdef Q_OS_WIN
	preventFullscreen(false),
#endif
	shaderProgramYCbCr(NULL), shaderProgramOSD(NULL),
	texCoordYCbCrLoc(-1), positionYCbCrLoc(-1), texCoordOSDLoc(-1), positionOSDLoc(-1),
	Contrast(-1), Saturation(-1), Brightness(-1), Hue(-1),
	isPaused(false), isOK(false), hasImage(false), doReset(true),
	X(-1), Y(-1), W(-1), H(-1), outW(-1), outH(-1), flip(0),
	glVer(0),
	aspectRatio(0.0), zoom(0.0)
{
	/* Initialize texCoordYCbCr array */
	texCoordYCbCr[0] = texCoordYCbCr[4] = texCoordYCbCr[5] = texCoordYCbCr[7] = 0.0f;
	texCoordYCbCr[1] = texCoordYCbCr[3] = 1.0f;
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
	Functions::getImageSize(aspectRatio, zoom, winSize.width(), winSize.height(), W, H, &X, &Y);
	doReset = true;
	if (canUpdate && isPaused)
		updateGL();
}
void OpenGL2Common::clearImg()
{
	hasImage = false;
	osdImg = QImage();
	osdChecksums.clear();
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
		const qint32 newPositionLoc = shaderProgramYCbCr->attributeLocation("vPosition");
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
		const qint32 newPositionLoc = shaderProgramOSD->attributeLocation("vPosition");
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
	for (int i = 1; i <= 4; ++i)
	{
		glBindTexture(GL_TEXTURE_2D, i);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, i == 1 ? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, i == 1 ? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	doReset = true;
}

void OpenGL2Common::paintGL()
{
	if (videoFrame.isEmpty() && !hasImage)
		return;

	const QSize winSize = widget()->size();

	bool resetDone = false;

	if (!videoFrame.isEmpty())
	{
		if (doReset)
		{
			/* Prepare textures */
			glBindTexture(GL_TEXTURE_2D, 2);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, videoFrame.linesize[0], outH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

			glBindTexture(GL_TEXTURE_2D, 3);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, videoFrame.linesize[1], outH >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

			glBindTexture(GL_TEXTURE_2D, 4);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, videoFrame.linesize[2], outH >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

			/* Prepare texture coordinates */
			texCoordYCbCr[2] = texCoordYCbCr[6] = (videoFrame.linesize[0] == outW) ? 1.0f : (outW / (videoFrame.linesize[0] + 1.0f));

			resetDone = true;
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 2);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, videoFrame.linesize[0], outH, GL_LUMINANCE, GL_UNSIGNED_BYTE, videoFrame.buffer[0].constData());

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 3);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, videoFrame.linesize[1], outH >> 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, videoFrame.buffer[1].constData());

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, 4);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, videoFrame.linesize[2], outH >> 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, videoFrame.buffer[2].constData());

		videoFrame.clear();
		hasImage = true;
	}

	shaderProgramYCbCr->setAttributeArray(positionYCbCrLoc, verticesYCbCr[flip], 2);
	shaderProgramYCbCr->setAttributeArray(texCoordYCbCrLoc, texCoordYCbCr, 2);
	shaderProgramYCbCr->enableAttributeArray(positionYCbCrLoc);
	shaderProgramYCbCr->enableAttributeArray(texCoordYCbCrLoc);

	shaderProgramYCbCr->bind();
	if (doReset)
	{
		shaderProgramYCbCr->setUniformValue("scale", W / (float)winSize.width(), H / (float)winSize.height());
		shaderProgramYCbCr->setUniformValue("videoEq", Brightness, Contrast, Saturation, Hue);
		doReset = !resetDone;
	}
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	shaderProgramYCbCr->release();

	shaderProgramYCbCr->disableAttributeArray(texCoordYCbCrLoc);
	shaderProgramYCbCr->disableAttributeArray(positionYCbCrLoc);

	glActiveTexture(GL_TEXTURE3);

	/* OSD */
	osdMutex.lock();
	if (!osdList.isEmpty())
	{
		glBindTexture(GL_TEXTURE_2D, 1);

		QRect bounds;
		const qreal scaleW = (qreal)W / outW, scaleH = (qreal)H / outH;
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

		const float left   = (bounds.left() + X) * 2.0f / winSize.width();
		const float right  = (bounds.right() + X + 1) * 2.0f / winSize.width();
		const float top    = (bounds.top() + Y) * 2.0f / winSize.height();
		const float bottom = (bounds.bottom() + Y + 1) * 2.0f / winSize.height();
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
		case QEvent::Resize:
			newSize(((QResizeEvent *)e)->size());
			break;
		case QEvent::Gesture:
			/* Pass gesture event to the parent */
			QCoreApplication::sendEvent(p, e);
			break;
		default:
			break;
	}
}
