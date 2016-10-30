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

#include <OpenGL2Writer.hpp>

#ifdef OPENGL_NEW_API
	#include <OpenGL2Window.hpp>
	#include <OpenGL2Widget.hpp>
#else
	#include <OpenGL2OldWidget.hpp>
#endif

#include <HWAccellInterface.hpp>
#include <VideoFrame.hpp>

#ifdef OPENGL_NEW_API
	#include <QGuiApplication>
#endif

OpenGL2Writer::OpenGL2Writer(Module &module) :
	drawable(NULL),
	allowPBO(true)
#ifdef OPENGL_NEW_API
	, forceRtt(false)
#endif
{
	addParam("W");
	addParam("H");
	addParam("AspectRatio");
	addParam("Zoom");
	addParam("Spherical");
	addParam("Flip");
	addParam("Rotate90");
	addParam("Saturation");
	addParam("Brightness");
	addParam("Contrast");

	SetModule(module);
}
OpenGL2Writer::~OpenGL2Writer()
{
	if (drawable)
		drawable->deleteMe();
}

bool OpenGL2Writer::set()
{
	bool doReset = false;
	bool newAllowPBO = sets().getBool("AllowPBO");
	if (newAllowPBO != allowPBO)
	{
		allowPBO = newAllowPBO;
		doReset = true;
	}
#ifdef VSYNC_SETTINGS
	vSync = sets().getBool("VSync");
	if (drawable && !drawable->setVSync(vSync))
		doReset = true;
#endif
#ifdef OPENGL_NEW_API
	bool newForceRtt = sets().getBool("ForceRtt");
	if (forceRtt != newForceRtt)
		doReset = true;
	forceRtt = newForceRtt;
#endif
	return !doReset && sets().getBool("Enabled");
}

bool OpenGL2Writer::readyWrite() const
{
	return drawable->isOK;
}

bool OpenGL2Writer::processParams(bool *)
{
	bool doResizeEvent = false;

	const double aspectRatio = getParam("AspectRatio").toDouble();
	const double zoom = getParam("Zoom").toDouble();
	const bool spherical = getParam("Spherical").toBool();
	const int flip = getParam("Flip").toInt();
	const bool rotate90 = getParam("Rotate90").toBool();
	const float Contrast = (getParam("Contrast").toInt() + 100) / 100.0f;
	const float Saturation = (getParam("Saturation").toInt() + 100) / 100.0f;
	const float Brightness = getParam("Brightness").toInt() / 100.0f;
	const float Hue = getParam("Hue").toInt() / -31.831f;
	const float Sharpness = getParam("Sharpness").toInt() / 50.0f;
	const int verticesIdx = rotate90 * 4 + flip;
	drawable->Deinterlace = getParam("Deinterlace").toInt();
	if (drawable->aspectRatio != aspectRatio || drawable->zoom != zoom || drawable->sphericalView != spherical || drawable->verticesIdx != verticesIdx || drawable->Contrast != Contrast || drawable->Brightness != Brightness || drawable->Saturation != Saturation || drawable->Hue != Hue || drawable->Sharpness != Sharpness)
	{
		drawable->zoom = zoom;
		drawable->aspectRatio = aspectRatio;
		drawable->verticesIdx = verticesIdx;
		drawable->Contrast = Contrast;
		drawable->Brightness = Brightness;
		drawable->Saturation = Saturation;
		drawable->Hue = Hue;
		drawable->Sharpness = Sharpness;
		drawable->setSpherical(spherical);
		doResizeEvent = drawable->widget()->isVisible();
	}

	const int outW = getParam("W").toInt();
	const int outH = getParam("H").toInt();
	if (outW != drawable->outW || outH != drawable->outH)
	{
		drawable->clearImg();
		if (outW > 0 && outH > 0)
		{
			drawable->outW = outW;
			drawable->outH = outH;
		}
		emit QMPlay2Core.dockVideo(drawable->widget());
	}

	if (doResizeEvent)
		drawable->newSize();
	else
		drawable->doReset = true;

	return readyWrite();
}

QMPlay2PixelFormats OpenGL2Writer::supportedPixelFormats() const
{
	return QMPlay2PixelFormats()
			<< QMPLAY2_PIX_FMT_YUV420P
			<< QMPLAY2_PIX_FMT_YUV422P
			<< QMPLAY2_PIX_FMT_YUV444P
			<< QMPLAY2_PIX_FMT_YUV410P
			<< QMPLAY2_PIX_FMT_YUV411P
			<< QMPLAY2_PIX_FMT_YUV440P
	;
}

void OpenGL2Writer::writeVideo(const VideoFrame &videoFrame)
{
	drawable->isPaused = false;
	drawable->videoFrame = videoFrame;
	drawable->updateGL(drawable->sphericalView);
}
void OpenGL2Writer::writeOSD(const QList<const QMPlay2_OSD *> &osds)
{
	QMutexLocker mL(&drawable->osdMutex);
	drawable->osdList = osds;
}

void OpenGL2Writer::setHWAccellInterface(HWAccellInterface *hwAccellInterface)
{
	addParam("Deinterlace");
	addParam("PrepareForHWBobDeint", true);
	VideoWriter::setHWAccellInterface(hwAccellInterface);
}

void OpenGL2Writer::pause()
{
	drawable->isPaused = true;
}

QString OpenGL2Writer::name() const
{
	QString glStr = drawable->glVer ? QString("%1.%2").arg(drawable->glVer / 10).arg(drawable->glVer % 10) : "2";
	if (drawable->hwAccellInterface)
		glStr += " " + drawable->hwAccellInterface->name();
#ifdef OPENGL_NEW_API
	if (useRtt)
		glStr += " (render-to-texture)";
#endif
#ifdef OPENGL_ES2
	return "OpenGL|ES " + glStr;
#else
	return "OpenGL " + glStr;
#endif
}

bool OpenGL2Writer::open()
{
#ifdef OPENGL_NEW_API
	static const QString platformName = QGuiApplication::platformName();
	if (platformName == "wayland" || platformName == "android")
		useRtt = true;
	else
		useRtt = forceRtt;
	if (useRtt)
	{
		//Don't use rtt when videoDock has native window
		const QWidget *videoDock = QMPlay2Core.getVideoDock();
		useRtt = !videoDock->internalWinId() || (videoDock == videoDock->window());
	}
	if (useRtt)
		drawable = new OpenGL2Widget;
	else
		drawable = new OpenGL2Window;
#else
	drawable = new OpenGL2OldWidget;
#endif
	drawable->hwAccellInterface = m_hwAccellInterface;
	drawable->setAllowPBO(allowPBO);
	if (drawable->testGL())
	{
#ifdef VSYNC_SETTINGS
		drawable->setVSync(vSync);
#endif
		if (drawable->glVer >= 30)
		{
			addParam("Hue");
			addParam("Sharpness");
		}
		return true;
	}
	return false;
}
