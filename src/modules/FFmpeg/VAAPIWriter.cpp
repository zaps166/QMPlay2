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

#include <VAAPIWriter.hpp>
#include <FFCommon.hpp>

#include <QMPlay2OSD.hpp>
#include <VideoFrame.hpp>
#include <Functions.hpp>
#include <ImgScaler.hpp>

#include <QCoreApplication>
#include <QPainter>

#include <va/va_x11.h>

VAAPIWriter::VAAPIWriter(Module &module, VAAPI *vaapi) :
	vaapi(vaapi),
	rgbImgFmt(NULL),
	aspect_ratio(0.0), zoom(0.0),
	Hue(0), Saturation(0), Brightness(0), Contrast(0)
{
	unsigned numSubpicFmts = vaMaxNumSubpictureFormats(vaapi->VADisp);
	VAImageFormat subpicFmtList[numSubpicFmts];
	unsigned subpic_flags[numSubpicFmts];
	if (vaQuerySubpictureFormats(vaapi->VADisp, subpicFmtList, subpic_flags, &numSubpicFmts) == VA_STATUS_SUCCESS)
	{
		for (unsigned i = 0; i < numSubpicFmts; ++i)
			if (!qstrncmp((const char *)&subpicFmtList[i].fourcc, "RGBA", 4))
			{
				subpict_dest_is_screen_coord = subpic_flags[i] & VA_SUBPICTURE_DESTINATION_IS_SCREEN_COORD;
				rgbImgFmt = new VAImageFormat(subpicFmtList[i]);
				break;
			}
	}

	vaImg.image_id = vaSubpicID = 0;

	setAttribute(Qt::WA_PaintOnScreen);
	grabGesture(Qt::PinchGesture);
	setMouseTracking(true);

	connect(&drawTim, SIGNAL(timeout()), this, SLOT(draw()));
	drawTim.setSingleShot(true);

	SetModule(module);
}
VAAPIWriter::~VAAPIWriter()
{
	clearRGBImage();
	delete rgbImgFmt;
	delete vaapi;
}

bool VAAPIWriter::set()
{
	return true;
}

bool VAAPIWriter::readyWrite() const
{
	return vaapi->ok;
}

bool VAAPIWriter::processParams(bool *)
{
	zoom = getParam("Zoom").toDouble();
	deinterlace = getParam("Deinterlace").toInt();
	aspect_ratio = getParam("AspectRatio").toDouble();

	const int _Hue = getParam("Hue").toInt();
	const int _Saturation = getParam("Saturation").toInt();
	const int _Brightness = getParam("Brightness").toInt();
	const int _Contrast = getParam("Contrast").toInt();
	if (_Hue != Hue || _Saturation != Saturation || _Brightness != Brightness || _Contrast != Contrast)
	{
		Hue = _Hue;
		Saturation = _Saturation;
		Brightness = _Brightness;
		Contrast = _Contrast;
		vaapi->applyVideoAdjustment(Brightness, Contrast, Saturation, Hue);
	}

	if (!isVisible())
		emit QMPlay2Core.dockVideo(this);
	else
	{
		resizeEvent(NULL);
		if (!drawTim.isActive())
			drawTim.start(paused ? 1 : drawTimeout);
	}

	return readyWrite();
}
void VAAPIWriter::writeVideo(const VideoFrame &videoFrame)
{
	VASurfaceID id;
	int field;
	if (vaapi->writeVideo(videoFrame, deinterlace, id, field))
		draw(id, field);
	paused = false;
}
void VAAPIWriter::writeOSD(const QList<const QMPlay2OSD *> &osds)
{
	if (rgbImgFmt)
	{
		osd_mutex.lock();
		osd_list = osds;
		osd_mutex.unlock();
	}
}
void VAAPIWriter::pause()
{
	paused = true;
}

bool VAAPIWriter::hwAccelGetImg(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) const
{
	return vaapi->getImage(videoFrame, dest, nv12ToRGB32);
}

QString VAAPIWriter::name() const
{
	return VAAPIWriterName;
}

bool VAAPIWriter::open()
{
	addParam("Zoom");
	addParam("AspectRatio");
	addParam("Deinterlace");
	addParam("PrepareForHWBobDeint", true);
	addParam("Hue");
	addParam("Saturation");
	addParam("Brightness");
	addParam("Contrast");
	return true;
}

void VAAPIWriter::init()
{
	clearRGBImage();
	id = VA_INVALID_SURFACE;
	field = -1;

	if (vaapi->isXvBA) //Not tested for many years...
	{
		QWidget::destroy();
		QWidget::create();
	}
}

void VAAPIWriter::draw(VASurfaceID _id, int _field)
{
	if (_id != VA_INVALID_SURFACE && _field > -1)
	{
		if (id != _id || _field == field)
			vaSyncSurface(vaapi->VADisp, _id);
		id = _id;
		field = _field;
	}
	if (id == VA_INVALID_SURFACE)
		return;

	bool associated = false;

	osd_mutex.lock();
	if (!osd_list.isEmpty())
	{
		QRect bounds;
		const qreal scaleW = (qreal)W / vaapi->outW, scaleH = (qreal)H / vaapi->outH;
		bool mustRepaint = Functions::mustRepaintOSD(osd_list, osd_checksums, &scaleW, &scaleH, &bounds);
		if (!mustRepaint)
			mustRepaint = vaImgSize != bounds.size();
		bool canAssociate = !mustRepaint;
		if (mustRepaint)
		{
			if (vaImgSize != bounds.size())
			{
				clearRGBImage();
				vaImgSize = QSize();
				if (vaCreateImage(vaapi->VADisp, rgbImgFmt, bounds.width(), bounds.height(), &vaImg) == VA_STATUS_SUCCESS)
				{
					if (vaCreateSubpicture(vaapi->VADisp, vaImg.image_id, &vaSubpicID) == VA_STATUS_SUCCESS)
						vaImgSize = bounds.size();
					else
						clearRGBImage();
				}
			}
			if (vaSubpicID)
			{
				quint8 *buff;
				if (vaMapBuffer(vaapi->VADisp, vaImg.buf, (void **)&buff) == VA_STATUS_SUCCESS)
				{
					QImage osdImg(buff += vaImg.offsets[0], vaImg.pitches[0] >> 2, bounds.height(), QImage::Format_ARGB32);
					osdImg.fill(0);
					QPainter p(&osdImg);
					p.translate(-bounds.topLeft());
					Functions::paintOSD(false, osd_list, scaleW, scaleH, p, &osd_checksums);
					vaUnmapBuffer(vaapi->VADisp, vaImg.buf);
					canAssociate = true;
				}
			}
		}
		if (vaSubpicID && canAssociate)
		{
			if (subpict_dest_is_screen_coord)
			{
				associated = vaAssociateSubpicture
				(
					vaapi->VADisp, vaSubpicID, &id, 1,
					0,              0,              bounds.width(), bounds.height(),
					bounds.x() + X, bounds.y() + Y, bounds.width(), bounds.height(),
					VA_SUBPICTURE_DESTINATION_IS_SCREEN_COORD
				) == VA_STATUS_SUCCESS;
			}
			else
			{
				const double sW = (double)vaapi->outW / dstQRect.width(), sH = (double)vaapi->outH / dstQRect.height();
				const int Xoffset = dstQRect.width() == width() ? X : 0;
				const int Yoffset = dstQRect.height() == height() ? Y : 0;
				associated = vaAssociateSubpicture
				(
					vaapi->VADisp, vaSubpicID, &id, 1,
					0,                           0,                           bounds.width(),      bounds.height(),
					(bounds.x() + Xoffset) * sW, (bounds.y() + Yoffset) * sH, bounds.width() * sW, bounds.height() * sH,
					0
				) == VA_STATUS_SUCCESS;
			}
		}
	}
	osd_mutex.unlock();

	for (int i = 0; i <= 1; ++i)
	{
		const int err = vaPutSurface
		(
			vaapi->VADisp, id, winId(),
			srcQRect.x(), srcQRect.y(), srcQRect.width(), srcQRect.height(),
			dstQRect.x(), dstQRect.y(), dstQRect.width(), dstQRect.height(),
			NULL, 0, field | VA_CLEAR_DRAWABLE
		);
		if (err != VA_STATUS_SUCCESS)
			QMPlay2Core.log(QString("vaPutSurface() - ") + vaErrorStr(err));
		VASurfaceStatus status;
		if (!vaapi->isVDPAU || i || vaQuerySurfaceStatus(vaapi->VADisp, id, &status) != VA_STATUS_SUCCESS || status != VASurfaceReady)
			break;
	}

	if (associated)
		vaDeassociateSubpicture(vaapi->VADisp, vaSubpicID, &id, 1);

	if (drawTim.isActive())
		drawTim.stop();
}

void VAAPIWriter::resizeEvent(QResizeEvent *)
{
	Functions::getImageSize(aspect_ratio, zoom, width(), height(), W, H, &X, &Y, &dstQRect, &vaapi->outW, &vaapi->outH, &srcQRect);
}
void VAAPIWriter::paintEvent(QPaintEvent *)
{
	if (!drawTim.isActive())
		drawTim.start(paused ? 1 : drawTimeout);
}
bool VAAPIWriter::event(QEvent *e)
{
	/* Pass gesture and touch event to the parent */
	switch (e->type())
	{
		case QEvent::TouchBegin:
		case QEvent::TouchUpdate:
		case QEvent::TouchEnd:
		case QEvent::Gesture:
			return QCoreApplication::sendEvent(parent(), e);
		default:
			return QWidget::event(e);
	}
}

QPaintEngine *VAAPIWriter::paintEngine() const
{
	return NULL;
}

void VAAPIWriter::clearRGBImage()
{
	if (vaSubpicID)
		vaDestroySubpicture(vaapi->VADisp, vaSubpicID);
	if (vaImg.image_id)
		vaDestroyImage(vaapi->VADisp, vaImg.image_id);
	vaImg.image_id = vaSubpicID = 0;
}
