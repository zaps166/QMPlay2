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

#include <QPainterWriter.hpp>

#include <QMPlay2OSD.hpp>
#include <VideoFrame.hpp>
#include <Functions.hpp>

#include <QCoreApplication>
#include <QPainter>

/**/

Drawable::Drawable(QPainterWriter &writer) :
	writer(writer)
{
	grabGesture(Qt::PinchGesture);
	setAutoFillBackground(true);
	setMouseTracking(true);
	setPalette(Qt::black);
}
Drawable::~Drawable()
{}

void Drawable::draw(const VideoFrame &newVideoFrame, bool canRepaint, bool entireScreen)
{
	if (!newVideoFrame.isEmpty())
		videoFrame = newVideoFrame;
	else if (videoFrame.isEmpty())
	{
		update();
		return;
	}
	m_scaleByQt = (imgW > videoFrame.size.width) || (imgH > videoFrame.size.height);
	if (imgScaler.create(videoFrame.size, m_scaleByQt ? videoFrame.size.width : imgW, m_scaleByQt ? videoFrame.size.height : imgH))
	{
		if (m_scaleByQt && (img.width() != videoFrame.size.width || img.height() != videoFrame.size.height))
		{
			img = QImage(videoFrame.size.width, videoFrame.size.height, QImage::Format_RGB32);
		}
		else if (!m_scaleByQt && (img.width() != imgW || img.height() != imgH))
		{
			img = QImage(imgW, imgH, QImage::Format_RGB32);
		}
		imgScaler.scale(videoFrame, img.bits());
		if (writer.flip)
			img = img.mirrored(writer.flip & Qt::Horizontal, writer.flip & Qt::Vertical);
		if (Brightness != 0 || Contrast != 100)
			Functions::ImageEQ(Contrast, Brightness, img.bits(), img.bytesPerLine() * img.height());
	}
	if (canRepaint && !entireScreen)
		update(X, Y, W, H);
	else if (canRepaint && entireScreen)
		update();
}

void Drawable::resizeEvent(QResizeEvent *e)
{
	const qreal dpr = devicePixelRatioF();
	Functions::getImageSize(writer.aspect_ratio, writer.zoom, width(), height(), W, H, &X, &Y);
	Functions::getImageSize(writer.aspect_ratio, writer.zoom, width() * dpr, height() * dpr, imgW, imgH);
	imgW = Functions::aligned(imgW, 8);

	imgScaler.destroy();
	img = QImage();

	draw(VideoFrame(), e ? false : true, true);
}
void Drawable::paintEvent(QPaintEvent *)
{
	QPainter p(this);

	if (m_scaleByQt)
		p.setRenderHint(QPainter::SmoothPixmapTransform);
	p.translate(X, Y);
	p.drawImage(QRect(0, 0, W, H), img);

	osd_mutex.lock();
	if (!osd_list.isEmpty())
	{
		const qreal dpr = devicePixelRatioF();
		if (!qFuzzyCompare(dpr, 1.0))
			p.scale(1.0 / dpr, 1.0 / dpr);
		p.setClipRect(0, 0, imgW, imgH);
		Functions::paintOSD(true, osd_list, (qreal)W / writer.outW, (qreal)H / writer.outH, p);
	}
	osd_mutex.unlock();
}
bool Drawable::event(QEvent *e)
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

/**/

QPainterWriter::QPainterWriter(Module &module) :
	outW(-1), outH(-1), flip(0),
	aspect_ratio(0.0), zoom(0.0),
	drawable(nullptr)
{
	addParam("W");
	addParam("H");
	addParam("AspectRatio");
	addParam("Zoom");
	addParam("Flip");
	addParam("Brightness");
	addParam("Contrast");

	SetModule(module);
}
QPainterWriter::~QPainterWriter()
{
	delete drawable;
}

bool QPainterWriter::set()
{
	return sets().getBool("Enabled");
}

bool QPainterWriter::readyWrite() const
{
	return drawable;
}

bool QPainterWriter::processParams(bool *)
{
	if (!drawable)
		drawable = new Drawable(*this);

	bool doResizeEvent = false;

	const double _aspect_ratio = getParam("AspectRatio").toDouble();
	const double _zoom = getParam("Zoom").toDouble();
	const int _flip = getParam("Flip").toInt();
	const int Contrast = getParam("Contrast").toInt() + 100;
	const int Brightness = getParam("Brightness").toInt() * 256 / 100;
	if (_aspect_ratio != aspect_ratio || _zoom != zoom || _flip != flip || Contrast != drawable->Contrast || Brightness != drawable->Brightness)
	{
		zoom = _zoom;
		aspect_ratio = _aspect_ratio;
		flip = _flip;
		drawable->Contrast = Contrast;
		drawable->Brightness = Brightness;
		doResizeEvent = drawable->isVisible();
	}

	const int _outW = getParam("W").toInt();
	const int _outH = getParam("H").toInt();
	if (_outW != outW || _outH != outH)
	{
		drawable->videoFrame.clear();
		if (_outW > 0 && _outH > 0)
		{
			outW = _outW;
			outH = _outH;
			emit QMPlay2Core.dockVideo(drawable);
		}
	}

	if (doResizeEvent)
		drawable->resizeEvent(nullptr);

	return readyWrite();
}

QMPlay2PixelFormats QPainterWriter::supportedPixelFormats() const
{
	return QMPlay2PixelFormats()
			<< QMPlay2PixelFormat::YUV420P
			<< QMPlay2PixelFormat::YUV422P
			<< QMPlay2PixelFormat::YUV444P
			<< QMPlay2PixelFormat::YUV410P
			<< QMPlay2PixelFormat::YUV411P
			<< QMPlay2PixelFormat::YUV440P
	;
}

void QPainterWriter::writeVideo(const VideoFrame &videoFrame)
{
	drawable->draw(videoFrame, true, false);
}
void QPainterWriter::writeOSD(const QList<const QMPlay2OSD *> &osds)
{
	drawable->osd_mutex.lock();
	drawable->osd_list = osds;
	drawable->osd_mutex.unlock();
}

QString QPainterWriter::name() const
{
	return QPainterWriterName;
}

bool QPainterWriter::open()
{
	return true;
}
