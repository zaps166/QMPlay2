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

#pragma once

#include <IOController.hpp>

#include <QStringList>
#include <QIcon>
#include <QDate>

struct AVDictionary;

class QHeaderView;
class QMPlay2OSD;
class VideoFrame;
class QMimeData;
class QPainter;
class QPixmap;
class QIcon;
class QRect;
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	class QWindow;
#endif

#ifdef Q_OS_WIN
	#include <windows.h>
#else
	#ifdef Q_OS_MAC
		#include <mach/mach_time.h>
	#else
		#include <time.h>
	#endif
	#include <unistd.h>
#endif

namespace Functions
{
	struct DemuxerInfo
	{
		QString name;
		QIcon icon;
		QStringList extensions;
	};
	using DemuxersInfo = QVector<DemuxerInfo>;
	using ChecksumList = QList<QByteArray>;

	QDate parseVersion(const QString &dateTxt);

	QString Url(QString, const QString &pth = QString());
	QString getUrlScheme(const QString &url);

	QString timeToStr(const double t, const bool decimals = false);

	QString filePath(const QString &);
	QString fileName(QString, bool extension = true);
	QString fileExt(const QString &);

	QString cleanPath(QString);
	QString cleanFileName(QString, const QString &replaced = "_");

	QString sizeString(quint64);

	static inline double gettime()
	{
#if defined Q_OS_WIN
		LARGE_INTEGER Frequency, Counter;
		QueryPerformanceFrequency(&Frequency);
		QueryPerformanceCounter(&Counter);
		return (double)Counter.QuadPart / (double)Frequency.QuadPart;
#elif defined Q_OS_MAC
		mach_timebase_info_data_t mach_base_info;
		mach_timebase_info(&mach_base_info);
		return ((double)mach_absolute_time() * (double)mach_base_info.numer) / (1000000000.0 * (double)mach_base_info.denom);
#else
		timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		return now.tv_sec + (now.tv_nsec / 1000000000.0);
#endif
	}
	static inline void s_wait(const double s)
	{
		if (s > 0.0)
		{
#ifndef Q_OS_WIN
			usleep(s * 1000000UL);
#else
			Sleep(s * 1000);
#endif
		}
	}

	template<typename T>
	static inline T aligned(const T val, const T alignment)
	{
		return (val + alignment - 1) & ~(alignment - 1);
	}
	static inline quint8 clip8(int val)
	{
		return val > 255 ? (quint8)255 : (val < 0 ? (quint8)0 : val);
	}

	void getImageSize(const double aspect_ratio, const double zoom, const int winW, const int winH, int &W, int &H, int *X = nullptr, int *Y = nullptr, QRect *dstRect = nullptr, const int *vidW = nullptr, const int *vidH = nullptr, QRect *srcRect = nullptr);

	QPixmap getPixmapFromIcon(const QIcon &icon, QSize size, QWidget *w = nullptr);
	void drawPixmap(QPainter &p, const QPixmap &pixmap, QWidget *w = nullptr, Qt::TransformationMode transformationMode = Qt::SmoothTransformation, Qt::AspectRatioMode aRatioMode = Qt::KeepAspectRatio, QSize size = QSize(), qreal scale = 1.0);

	bool mustRepaintOSD(const QList<const QMPlay2OSD *> &osd_list, const ChecksumList &osd_checksums, const qreal *scaleW = nullptr, const qreal *scaleH = nullptr, QRect *bounds = nullptr);
	void paintOSD(bool rgbSwapped, const QList<const QMPlay2OSD *> &osd_list, const qreal scaleW, const qreal scaleH, QPainter &painter, ChecksumList *osd_checksums = nullptr);
	void paintOSDtoYV12(quint8 *imageData, QImage &osdImg, int W, int H, int linesizeLuma, int linesizeChroma, const QList<const QMPlay2OSD *> &osd_list, ChecksumList &osd_checksums);

	QPixmap applyDropShadow(const QPixmap &input, const qreal blurRadius, const QPointF &offset, const QColor &color);
	QPixmap applyBlur(const QPixmap &input, const qreal blurRadius);

	void ImageEQ(int Contrast, int Brightness, quint8 *imageBits, unsigned bitsCount);
	int scaleEQValue(int val, int min, int max);

	int getField(const VideoFrame &videoFrame, int deinterlace, int fullFrame, int topField, int bottomField);

	QByteArray convertToASS(QString txt);

	bool chkMimeData(const QMimeData *);
	QStringList getUrlsFromMimeData(const QMimeData *);

	bool splitPrefixAndUrlIfHasPluginPrefix(const QString &entireUrl, QString *addressPrefixName, QString *url, QString *param = nullptr);
	void getDataIfHasPluginPrefix(const QString &entireUrl, QString *url = nullptr, QString *name = nullptr, QIcon *icon = nullptr, IOController<> *ioCtrl = nullptr, const DemuxersInfo &demuxersInfo = DemuxersInfo());

	QString prepareFFmpegUrl(QString url, AVDictionary *&options, bool setCookies = true, bool icy = true, const QByteArray &userAgent = QByteArray());

	void hFlip(quint8 *data, int linesize, int height, int width);
	void vFlip(quint8 *data, int linesize, int height);

	QString dBStr(double a);

	quint32 getBestSampleRate();

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	QWindow *getNativeWindow(const QWidget *w);
#endif

	bool wrapMouse(QWidget *widget, QPoint &mousePos, int margin = 0);

	void setHeaderSectionResizeMode(QHeaderView *header, int index, int resizeMode);

	QByteArray decryptAes256Cbc(const QByteArray &password, const QByteArray &salt, const QByteArray &ciphered);
}
