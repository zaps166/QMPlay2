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

#include <Functions.hpp>

#include <QMPlay2Extensions.hpp>
#include <QMPlay2OSD.hpp>
#include <DeintFilter.hpp>
#include <VideoFrame.hpp>
#include <Reader.hpp>

#include <QMimeData>
#include <QPainter>
#include <QDir>
#include <QUrl>
#include <QMessageBox>

#include <math.h>

static inline void swapArray(quint8 *a, quint8 *b, int size)
{
	quint8 t[size];
	memcpy(t, a, size);
	memcpy(a, b, size);
	memcpy(b, t, size);
}

/**/

QDate Functions::parseVersion(const QString &dateTxt)
{
	const QStringList l = dateTxt.split('.');
	int y = 0, m = 0, d = 0;
	if (l.count() == 3)
	{
		y = l[0].toInt() + 2000;
		m = l[1].toInt();
		d = l[2].toInt();
	}
	if (y < 2000 || m < 1 || m > 12 || d < 1 || d > 31)
		y = m = d = 0;
	return QDate(y, m, d);
}

QString Functions::Url(QString url, const QString &pth)
{
#ifdef Q_OS_WIN
	url.replace('\\', '/');
#endif
	const QString scheme = getUrlScheme(url);
#ifdef Q_OS_WIN
	if (url.startsWith("file:///")) //lokalnie na dysku
	{
		url.remove(7, 1);
		return url;
	}
	else if (url.startsWith("file://")) //adres sieciowy
	{
		url.replace("file://", "file:////");
		return url;
	}
	if (url.startsWith("//")) //adres sieciowy
	{
		url.prepend("file://");
		return url;
	}

	QStringList drives;
	QFileInfoList fIL = QDir::drives();
	foreach (const QFileInfo &fI, fIL)
		drives += getUrlScheme(fI.path());
	if (drives.contains(scheme))
	{
		url = "file://" + url;
		return url;
	}
#endif
	if (scheme.isEmpty())
	{
#ifdef Q_OS_WIN
		if (url.startsWith("/"))
			url.remove(0, 1);
#endif
#ifndef Q_OS_WIN
		if (!url.startsWith("/"))
#endif
		{
			QString addPth = pth.isEmpty() ? QDir::currentPath() : pth;
			if (!addPth.endsWith("/"))
				addPth += '/';
			url.prepend(addPth);
		}
		url.prepend("file://");
	}
	return url;
}
QString Functions::getUrlScheme(const QString &url)
{
	int idx = url.indexOf(':');
	if (idx > -1 && url[0] != '/')
		return url.left(idx);
	return QString();
}

QString Functions::timeToStr(double t, bool space)
{
	if (t < 0.0)
		return QString();

	const QString separator = (space ? " : " : ":");

	int h, m, s;
	getHMS(t + 0.5, h, m, s);

	QString timStr;
	if (h)
		timStr = QString("%1" + separator).arg(h, 2, 10, QChar('0'));
	timStr += QString("%1" + separator + "%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));

	return timStr;
}

QString Functions::filePath(const QString &f)
{
	return f.left(f.lastIndexOf('/') + 1);
}
QString Functions::fileName(QString f, bool extension)
{
	/* Tylko dla adresów do przekonwertowania */
	QString real_url;
	if (splitPrefixAndUrlIfHasPluginPrefix(f, NULL, &real_url))
	{
		if (real_url.startsWith("file://"))
			return fileName(real_url, extension);
		return real_url;
	}
	/**/
#ifndef Q_OS_WIN
	if (f == "file:///")
		return "/";
#endif
	while (f.endsWith("/"))
		f.chop(1);
	const QString n = f.right(f.length() - f.lastIndexOf('/') - 1);
	if (extension || (!f.startsWith("file://") && f.contains("://")))
		return n;
	return n.mid(0, n.lastIndexOf('.'));
}
QString Functions::fileExt(const QString &f)
{
	const int idx = f.lastIndexOf('.');
	if (idx > -1)
		return f.mid(idx+1);
	return QString();
}

QString Functions::cleanPath(QString p)
{
#ifndef Q_OS_WIN
	if (p == "file:///")
		return p;
#endif
	if (!p.endsWith("/"))
		return p + "/";
	while (p.endsWith("//"))
		p.chop(1);
	return p;
}
QString Functions::cleanFileName(QString f)
{
	if (f.length() > 200)
		f.resize(200);
	f.replace('/', '_');
#ifdef Q_OS_WIN
	f.replace('\\', '_');
	f.replace(':', '_');
	f.replace('*', '_');
	f.replace('?', '_');
	f.replace('"', '_');
	f.replace('<', '_');
	f.replace('>', '_');
	f.replace('|', '_');
#endif
	return f;
}

QString Functions::sizeString(quint64 B)
{
	if (B < 1024ULL)
		return QString::number(B) + " B";
	else if (B < 1048576ULL)
		return QString::number(B / 1024.0, 'f', 2) + " KiB";
	else if (B < 1073741824ULL)
		return QString::number(B / 1048576.0, 'f', 2) + " MiB";
	else if (B  < 1099511627776ULL)
		return QString::number(B / 1073741824.0, 'f', 2) + " GiB";
	return QString();
}

void Functions::getImageSize(const double aspect_ratio, const double zoom, const int winW, const int winH, int &W, int &H, int *X, int *Y, QRect *dstRect, const int *vidW, const int *vidH, QRect *srcRect)
{
	W = winW;
	H = winH;
	if (aspect_ratio > 0.0)
	{
		if (W / aspect_ratio > H)
			W = H * aspect_ratio;
		else
			H = W / aspect_ratio;
	}
	if (zoom != 1.0 && zoom > 0.0)
	{
		W *= zoom;
		H *= zoom;
	}
	if (X)
		*X = (winW - W) / 2;
	if (Y)
		*Y = (winH - H) / 2;
	if (X && Y && dstRect)
	{
		*dstRect = QRect(*X, *Y, W, H) & QRect(0, 0, winW, winH);
		if (vidW && vidH && srcRect)
		{
			if (W > 0 && H > 0)
				srcRect->setCoords
				(
					(dstRect->x() - *X) * *vidW / W,
					(dstRect->y() - *Y) * *vidH / H,
					*vidW - (*X + W - 1 - dstRect->right() ) * *vidW / W - 1,
					*vidH - (*Y + H - 1 - dstRect->bottom()) * *vidH / H - 1
				);
			else
				srcRect->setCoords(0, 0, 0, 0);
		}
	}
}

bool Functions::mustRepaintOSD(const QList<const QMPlay2OSD *> &osd_list, const ChecksumList &osd_checksums, const qreal *scaleW, const qreal *scaleH, QRect *bounds)
{
	bool mustRepaint = (osd_list.count() != osd_checksums.count());
	foreach (const QMPlay2OSD *osd, osd_list)
	{
		osd->lock();
		if (!mustRepaint)
			mustRepaint = !osd_checksums.contains(osd->getChecksum());
		if (scaleW && scaleH && bounds)
		{
			for (int j = 0; j < osd->imageCount(); j++)
			{
				const QMPlay2OSD::Image &img = osd->getImage(j);
				if (!osd->needsRescale())
					*bounds |= img.rect;
				else
				{
					const qreal scaledW = *scaleW;
					const qreal scaledH = *scaleH;
					*bounds |= QRect(img.rect.x() * scaledW, img.rect.y() * scaledH, img.rect.width() * scaledW, img.rect.height() * scaledH);
				}
			}
		}
		osd->unlock();
	}
	return mustRepaint;
}
void Functions::paintOSD(bool rgbSwapped, const QList<const QMPlay2OSD *> &osd_list, const qreal scaleW, const qreal scaleH, QPainter &painter, ChecksumList *osd_checksums)
{
	if (osd_checksums)
		osd_checksums->clear();
	foreach (const QMPlay2OSD *osd, osd_list)
	{
		osd->lock();
		if (osd_checksums)
			osd_checksums->append(osd->getChecksum());
		if (osd->needsRescale())
		{
			painter.save();
			painter.setRenderHint(QPainter::SmoothPixmapTransform);
			painter.scale(scaleW, scaleH);
		}
		for (int j = 0; j < osd->imageCount(); j++)
		{
			const QMPlay2OSD::Image &img = osd->getImage(j);
			const QImage qImg = QImage((uchar *)img.data.data(), img.rect.width(), img.rect.height(), QImage::Format_ARGB32);
			painter.drawImage(img.rect.topLeft(), rgbSwapped ? qImg.rgbSwapped() : qImg);
		}
		if (osd->needsRescale())
			painter.restore();
		osd->unlock();
	}
}
void Functions::paintOSDtoYV12(quint8 *imageData, QImage &osdImg, int W, int H, int linesizeLuma, int linesizeChroma, const QList<const QMPlay2OSD *> &osd_list, ChecksumList &osd_checksums)
{
	QRect bounds;
	const int osdW = osdImg.width();
	const int imgH = osdImg.height();
	const qreal iScaleW = (qreal)osdW / W, iScaleH = (qreal)imgH / H;
	const qreal scaleW = (qreal)W / osdW, scaleH = (qreal)H / imgH;
	const bool mustRepaint = Functions::mustRepaintOSD(osd_list, osd_checksums, &scaleW, &scaleH, &bounds);
	bounds = QRect(floor(bounds.x() * iScaleW), floor(bounds.y() * iScaleH), ceil(bounds.width() * iScaleW), ceil(bounds.height() * iScaleH)) & QRect(0, 0, osdW, imgH);
	quint32 *osdImgData = (quint32 *)osdImg.constBits();
	if (mustRepaint)
	{
		for (int h = bounds.top(); h <= bounds.bottom(); ++h)
			memset(osdImgData + (h * osdW + bounds.left()), 0, bounds.width() << 2);
		QPainter p(&osdImg);
		p.setRenderHint(QPainter::SmoothPixmapTransform);
		p.scale(iScaleW, iScaleH);
		Functions::paintOSD(false, osd_list, scaleW, scaleH, p, &osd_checksums);
	}

	quint8 *data[3];
	data[0] = imageData;
	data[2] = data[0] + linesizeLuma * imgH;
	data[1] = data[2] + linesizeChroma * (imgH >> 1);

	for (int h = bounds.top(); h <= bounds.bottom(); ++h)
	{
		const int fullLine = h * linesizeLuma;
		const int halfLine = (h >> 1) * linesizeChroma;
		const int line = h * osdW;

		for (int w = bounds.left(); w <= bounds.right(); ++w)
		{
			const int pixelPos = fullLine + w;
			const quint32 pixel = osdImgData[line + w];
			const quint8 A = (pixel >> 24) & 0xFF;
			if (A)
			{
				const quint8 iA = ~A & 0xFF;
				const quint8  B = (pixel >> 16) & 0xFF;
				const quint8  G = (pixel >> 8) & 0xFF;
				const quint8  R = pixel & 0xFF;

				const quint8 Y = ((R * 66) >> 8) + (G >> 1) + ((B * 25) >> 8) + 16;
				if (A == 0xFF)
					data[0][pixelPos] = Y;
				else
					data[0][pixelPos] = data[0][fullLine + w] * iA / 255 + Y * A / 255;

				if (!(w & 1) && !(h & 1))
				{
					const int pixelPos = halfLine + (w >> 1);
					const quint8 cB = -((R * 38 ) >> 8) - ((G * 74) >> 8) + ((B * 112) >> 8) + 128;
					const quint8 cR =  ((R * 112) >> 8) - ((G * 94) >> 8) - ((B * 18 ) >> 8) + 128;
					if (A == 0xFF)
					{
						data[1][pixelPos] = cB;
						data[2][pixelPos] = cR;
					}
					else
					{
						const int pixelPosAlpha = halfLine + (w >> 1);
						data[1][pixelPos] = data[1][pixelPosAlpha] * iA / 255 + cB * A / 255;
						data[2][pixelPos] = data[2][pixelPosAlpha] * iA / 255 + cR * A / 255;
					}
				}
			}
		}
	}
}

void Functions::ImageEQ(int Contrast, int Brightness, quint8 *imageBits, unsigned bitsCount)
{
	for (unsigned i = 0; i < bitsCount; i += 4)
	{
		imageBits[i+0] = clip8(imageBits[i+0] * Contrast / 100 + Brightness);
		imageBits[i+1] = clip8(imageBits[i+1] * Contrast / 100 + Brightness);
		imageBits[i+2] = clip8(imageBits[i+2] * Contrast / 100 + Brightness);
	}
}
int Functions::scaleEQValue(int val, int min, int max)
{
	return (val + 100) * ((abs(min) + abs(max))) / 200 - abs(min);
}

int Functions::getField(const VideoFrame &videoFrame, int deinterlace, int fullFrame, int topField, int bottomField)
{
	if (deinterlace)
	{
		const quint8 deintFlags = deinterlace >> 1;
		if (videoFrame.interlaced || !(deintFlags & DeintFilter::AutoDeinterlace))
		{
			bool topFieldFirst;
			if ((deintFlags & DeintFilter::DoubleFramerate) || ((deintFlags & DeintFilter::AutoParity) && videoFrame.interlaced))
				topFieldFirst = videoFrame.tff;
			else
				topFieldFirst = deintFlags & DeintFilter::TopFieldFirst;
			return topFieldFirst ? topField : bottomField;
		}
	}
	return fullFrame;
}

QByteArray Functions::convertToASS(QString txt)
{
	txt.replace("<i>", "{\\i1}", Qt::CaseInsensitive);
	txt.replace("</i>", "{\\i0}", Qt::CaseInsensitive);
	txt.replace("<b>", "{\\b1}", Qt::CaseInsensitive);
	txt.replace("</b>", "{\\b0}", Qt::CaseInsensitive);
	txt.replace("<u>", "{\\u1}", Qt::CaseInsensitive);
	txt.replace("</u>", "{\\u0}", Qt::CaseInsensitive);
	txt.replace("<s>", "{\\s1}", Qt::CaseInsensitive);
	txt.replace("</s>", "{\\s0}", Qt::CaseInsensitive);
	txt.remove('\r');
	txt.replace('\n', "\\N", Qt::CaseInsensitive);

	//Colors
	QRegExp colorRegExp("<font\\s+color\\s*=\\s*\\\"?\\#?(\\w{6})\\\"?>(.*)</font>", Qt::CaseInsensitive);
	colorRegExp.setMinimal(true);
	int pos = 0;
	while ((pos = colorRegExp.indexIn(txt, pos)) != -1)
	{
		QString rgb = colorRegExp.cap(1);
		rgb = rgb.mid(4, 2) + rgb.mid(2, 2) + rgb.mid(0, 2);

		const QString replaced = "{\\1c&" + rgb + "&}" + colorRegExp.cap(2) + "{\\1c}";

		txt.replace(pos, colorRegExp.matchedLength(), replaced);
		pos += replaced.length();
	}

	return txt.toUtf8();
}

bool Functions::chkMimeData(const QMimeData *mimeData)
{
	return mimeData && ((mimeData->hasUrls() && !mimeData->urls().isEmpty()) || (mimeData->hasText() && !mimeData->text().isEmpty()));
}
QStringList Functions::getUrlsFromMimeData(const QMimeData *mimeData)
{
	QStringList urls;
	if (mimeData->hasUrls())
	{
		foreach (const QUrl &url, mimeData->urls())
		{
			QString u = url.toLocalFile();
			if (u.length() > 1 && u.endsWith("/"))
				u.chop(1);
			if (!u.isEmpty())
				urls += u;
		}
	}
	else if (mimeData->hasText())
		urls = mimeData->text().remove('\r').split('\n', QString::SkipEmptyParts);
	return urls;
}

bool Functions::splitPrefixAndUrlIfHasPluginPrefix(const QString &entireUrl, QString *addressPrefixName, QString *url, QString *param)
{
	int idx = entireUrl.indexOf("://{");
	if (idx > -1)
	{
		if (addressPrefixName)
			*addressPrefixName = entireUrl.mid(0, idx);
		if (url || param)
		{
			idx += 4;
			int idx2 = entireUrl.indexOf("}", idx);
			if (idx2 > -1)
			{
				if (param)
					*param = entireUrl.mid(idx2+1, entireUrl.length()-(idx2+1));
				if (url)
					*url = entireUrl.mid(idx, idx2-idx);
			}
		}
		return (!addressPrefixName || !addressPrefixName->isEmpty()) && (!url || !url->isNull());
	}
	return false;
}
void Functions::getDataIfHasPluginPrefix(const QString &entireUrl, QString *url, QString *name, QImage *img, IOController<> *ioCtrl, const DemuxersInfo &demuxersInfo)
{
	QString addressPrefixName, realUrl, param;
	if ((url || img) && splitPrefixAndUrlIfHasPluginPrefix(entireUrl, &addressPrefixName, &realUrl, &param))
	{
		foreach (QMPlay2Extensions *QMPlay2Ext, QMPlay2Extensions::QMPlay2ExtensionsList())
			if (QMPlay2Ext->addressPrefixList(false).contains(addressPrefixName))
			{
				QMPlay2Ext->convertAddress(addressPrefixName, realUrl, param, url, name, img, NULL, ioCtrl);
				return;
			}
	}
	if (img)
	{
		const QString scheme = getUrlScheme(entireUrl);
		const QString extension = fileExt(entireUrl).toLower();
		if (demuxersInfo.isEmpty())
		{
			foreach (const Module *module, QMPlay2Core.getPluginsInstance())
				foreach (const Module::Info &mod, module->getModulesInfo())
					if (mod.type == Module::DEMUXER && (mod.name == scheme || mod.extensions.contains(extension)))
					{
						*img = !mod.img.isNull() ? mod.img : module->image();
						return;
					}
		}
		else
		{
			foreach (const DemuxerInfo &demuxerInfo, demuxersInfo)
				if (demuxerInfo.name == scheme || demuxerInfo.extensions.contains(extension))
				{
					*img = demuxerInfo.img;
					return;
				}
		}
	}
}

void Functions::hFlip(quint8 *data, int linesize, int height, int width)
{
	int h, w, width_div_2 = width/2, linesize_div_2 = linesize/2, width_div_4 = width/4, offset = 0;
	for (h = 0; h < height; ++h)
	{
		for (w = 0; w < width_div_2; ++w)
			qSwap(data[offset + w], data[offset + width-1-w]);
		offset += linesize;
	}
	for (h = 0; h < height; ++h)
	{
		for (w = 0; w < width_div_4; ++w)
			qSwap(data[offset + w], data[offset + width_div_2-1-w]);
		offset += linesize_div_2;
	}
}
void Functions::vFlip(quint8 *data, int linesize, int height)
{
	int size = linesize*height, chroma_size = linesize*height/4, chroma_width = linesize/2;
	quint8 *data_s;

	for (data_s = data + size; data_s > data;)
	{
		swapArray(data, data_s -= linesize, linesize);
		data += linesize;
	}
	data += size/2;

	for (data_s = data + chroma_size; data_s > data;)
	{
		swapArray(data, data_s -= chroma_width, chroma_width);
		data += chroma_width;
	}
	data += chroma_size/2;

	for (data_s = data + chroma_size; data_s > data;)
	{
		swapArray(data, data_s -= chroma_width, chroma_width);
		data += chroma_width;
	}
}

QString Functions::dBStr(double a)
{
	return (!a ? "-∞" : QString::number(20.0 * log10(a), 'g', 2)) + " dB";
}

quint32 Functions::getBestSampleRate()
{
	quint32 srate = 48000; //Use 48kHz as default
	if (QMPlay2Core.getSettings().getBool("ForceSamplerate"))
	{
		const quint32 choosenSrate = QMPlay2Core.getSettings().getUInt("Samplerate");
		if ((choosenSrate % 11025) == 0)
			srate = 44100;
	}
	return srate;
}

bool Functions::wrapMouse(QWidget *widget, QPoint &mousePos, int margin)
{
	const QSize winSize = widget->size();
	bool doWrap = false;

	if (mousePos.x() >= winSize.width() - (margin + 1))
	{
		mousePos.setX(margin + 1);
		doWrap = true;
	}
	else if (mousePos.x() <= margin)
	{
		mousePos.setX(winSize.width() - (margin + 2));
		doWrap = true;
	}

	if (mousePos.y() >= winSize.height() - (margin + 1))
	{
		mousePos.setY(margin + 1);
		doWrap = true;
	}
	else if (mousePos.y() <= margin)
	{
		mousePos.setY(winSize.height() - (margin + 2));
		doWrap = true;
	}

	if (doWrap)
		QCursor::setPos(widget->mapToGlobal(mousePos));

	return doWrap;
}
