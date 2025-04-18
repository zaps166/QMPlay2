/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2025  Błażej Szczygieł

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
#include <QMPlay2Lib.hpp>
#include <QMPlay2OSD.hpp>

#include <QStringList>
#include <QMatrix4x4>
#include <QVector2D>
#include <QIcon>
#include <QDate>

#include <array>

extern "C" {
    #include <libavutil/pixfmt.h>
}

struct AVDictionary;

class QHeaderView;
class StreamInfo;
class QMimeData;
class QPainter;
class QPixmap;
class Frame;
class QIcon;
class QRect;

#ifdef Q_OS_WIN
    #include <windows.h>
#else
    #ifdef Q_OS_MACOS
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
    using OsdIdList = QVector<quint64>;

    QMPLAY2SHAREDLIB_EXPORT QDate parseVersion(const QString &dateTxt);

    QMPLAY2SHAREDLIB_EXPORT QString Url(QString, const QString &pth = QString());
    QMPLAY2SHAREDLIB_EXPORT QString getUrlScheme(const QString &url);

    QMPLAY2SHAREDLIB_EXPORT QString timeToStr(const double t, const bool decimals = false, const bool milliseconds = false);

    QMPLAY2SHAREDLIB_EXPORT QString filePath(const QString &);
    QMPLAY2SHAREDLIB_EXPORT QString fileName(QString, bool extension = true);
    QMPLAY2SHAREDLIB_EXPORT QString fileExt(const QString &);

    QMPLAY2SHAREDLIB_EXPORT QString cleanPath(QString);
    QMPLAY2SHAREDLIB_EXPORT QString cleanFileName(QString, const QString &replaced = "_");

    QMPLAY2SHAREDLIB_EXPORT QString sizeString(quint64);

    static inline double gettime()
    {
#if defined Q_OS_WIN
        LARGE_INTEGER Frequency, Counter;
        QueryPerformanceFrequency(&Frequency);
        QueryPerformanceCounter(&Counter);
        return (double)Counter.QuadPart / (double)Frequency.QuadPart;
#elif defined Q_OS_MACOS
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

    QMPLAY2SHAREDLIB_EXPORT void getImageSize(const double aspect_ratio, const double zoom, const int winW, const int winH, int &W, int &H, int *X = nullptr, int *Y = nullptr, QRect *dstRect = nullptr, const int *vidW = nullptr, const int *vidH = nullptr, QRect *srcRect = nullptr);

    QMPLAY2SHAREDLIB_EXPORT QPixmap getPixmapFromIcon(const QIcon &icon, QSize size, QWidget *w = nullptr);
    QMPLAY2SHAREDLIB_EXPORT void drawPixmap(QPainter &p, const QPixmap &pixmap, const QWidget *w, Qt::TransformationMode transformationMode = Qt::SmoothTransformation, Qt::AspectRatioMode aRatioMode = Qt::KeepAspectRatio, QSize size = QSize(), qreal scale = 1.0, bool alwaysAllowEnlarge = false);

    QMPLAY2SHAREDLIB_EXPORT bool mustRepaintOSD(const QMPlay2OSDList &osd_list, const OsdIdList &osd_ids, const qreal *scaleW = nullptr, const qreal *scaleH = nullptr, QRect *bounds = nullptr);
    QMPLAY2SHAREDLIB_EXPORT void paintOSD(bool rgbSwapped, const QMPlay2OSDList &osd_list, const qreal scaleW, const qreal scaleH, QPainter &painter, OsdIdList *osd_ids = nullptr);
    QMPLAY2SHAREDLIB_EXPORT void paintOSDtoYV12(quint8 *imageData, QImage &osdImg, int W, int H, int linesizeLuma, int linesizeChroma, const QMPlay2OSDList &osd_list, OsdIdList &osd_ids);

    QMPLAY2SHAREDLIB_EXPORT QPixmap applyDropShadow(const QPixmap &input, const qreal blurRadius, const QPointF &offset, const QColor &color);
    QMPLAY2SHAREDLIB_EXPORT QPixmap applyBlur(const QPixmap &input, const qreal blurRadius);

    QMPLAY2SHAREDLIB_EXPORT void ImageEQ(int Contrast, int Brightness, quint8 *imageBits, unsigned bitsCount);
    QMPLAY2SHAREDLIB_EXPORT int scaleEQValue(int val, int min, int max);

    QMPLAY2SHAREDLIB_EXPORT QByteArray convertToASS(QString txt);

    QMPLAY2SHAREDLIB_EXPORT bool chkMimeData(const QMimeData *mimeData);
    QMPLAY2SHAREDLIB_EXPORT QStringList getUrlsFromMimeData(const QMimeData *mimeData, const bool checkExtensionsForUrl = true);

    QMPLAY2SHAREDLIB_EXPORT QString maybeExtensionAddress(const QString &url);

    QMPLAY2SHAREDLIB_EXPORT bool splitPrefixAndUrlIfHasPluginPrefix(const QString &entireUrl, QString *addressPrefixName, QString *url, QString *param = nullptr);
    QMPLAY2SHAREDLIB_EXPORT void getDataIfHasPluginPrefix(const QString &entireUrl, QString *url = nullptr, QString *name = nullptr, QIcon *icon = nullptr, IOController<> *ioCtrl = nullptr, const DemuxersInfo &demuxersInfo = DemuxersInfo());
    QMPLAY2SHAREDLIB_EXPORT bool isResourcePlaylist(const QString &url);

    QMPLAY2SHAREDLIB_EXPORT QByteArray getUserAgent(bool withMozilla = true);
    QMPLAY2SHAREDLIB_EXPORT QString prepareFFmpegUrl(QString url, AVDictionary *&options, bool defUserAgentWithMozilla = true, bool setCookies = true, bool setRawHeaders = true, bool icy = true, const QByteArray &userAgentArg = QByteArray());

    QMPLAY2SHAREDLIB_EXPORT void hFlip(quint8 *data, int linesize, int height, int width);
    QMPLAY2SHAREDLIB_EXPORT void vFlip(quint8 *data, int linesize, int height);

    QMPLAY2SHAREDLIB_EXPORT QString dBStr(double a);

    QMPLAY2SHAREDLIB_EXPORT quint32 getBestSampleRate();

    QMPLAY2SHAREDLIB_EXPORT bool wrapMouse(QWidget *widget, QPoint &mousePos, int margin = 0);

    QMPLAY2SHAREDLIB_EXPORT QByteArray textWithFallbackEncoding(const QByteArray &data);

    QMPLAY2SHAREDLIB_EXPORT QMatrix4x4 getYUVtoRGBmatrix(AVColorSpace colorSpace);

    QMPLAY2SHAREDLIB_EXPORT bool isColorPrimariesSupported(AVColorPrimaries colorPrimaries);
    QMPLAY2SHAREDLIB_EXPORT bool fillColorPrimariesData(AVColorPrimaries colorPrimaries, QVector2D &wp, std::array<QVector2D, 3> &primaries);

    QMPLAY2SHAREDLIB_EXPORT QMatrix4x4 getColorPrimariesTo709Matrix(const QVector2D &wp, std::array<QVector2D, 3> &primaries);
    QMPLAY2SHAREDLIB_EXPORT QMatrix4x4 getColorPrimariesTo709Matrix(AVColorPrimaries colorPrimaries);

    QMPLAY2SHAREDLIB_EXPORT bool compareText(const QString &a, const QString &b);

    QMPLAY2SHAREDLIB_EXPORT bool hasTouchScreen();

    QMPLAY2SHAREDLIB_EXPORT QString getBitrateStr(const int64_t bitRate);

    QMPLAY2SHAREDLIB_EXPORT QString getSeqFile(const QString &dir, const QString &ext, const QString &frag);

    QMPLAY2SHAREDLIB_EXPORT std::pair<QString, QString> determineExtFmt(const QList<StreamInfo *> &streamsInfo);

    QMPLAY2SHAREDLIB_EXPORT void getUserDoubleValue(QWidget *parent, const QString &title, const QString &label, double value, double min, double max, int decimals, double step, const std::function<void(double)> &onChanged);
}
