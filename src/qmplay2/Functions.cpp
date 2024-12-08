/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

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
#include <Frame.hpp>
#include <Version.hpp>
#include <Reader.hpp>
#include <StreamInfo.hpp>

#include <QGraphicsDropShadowEffect>
#include <QGraphicsBlurEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QHeaderView>
#include <QMimeData>
#include <QPainter>
#include <QDir>
#include <QUrl>
#include <QWindow>
#include <QLibrary>
#include <QTextCodec>
#include <QMessageBox>
#include <QStyleOption>
#include <QGuiApplication>
#include <QRegularExpression>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
# include <QInputDevice>
#else
# include <QTouchDevice>
#endif

extern "C"
{
    #include <libavformat/version.h>
    #include <libavutil/dict.h>
}

#include <cmath>
#include <vector>

using namespace std;

static inline void swapArray(quint8 *a, quint8 *b, int size)
{
    quint8 t[size];
    memcpy(t, a, size);
    memcpy(a, b, size);
    memcpy(b, t, size);
}

static inline QWindow *getNativeWindow(const QWidget *w)
{
    if (w)
    {
        if (QWidget *winW = w->window())
            return winW->windowHandle();
    }
    return nullptr;
}

/**/

QDate Functions::parseVersion(const QString &dateTxt)
{
    const QStringList l = dateTxt.split(QRegularExpression(R"(\D)"));
    int y = 0, m = 0, d = 0;
    if (l.count() >= 3)
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

    const auto oldErrorMode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    auto driveBits = GetLogicalDrives() & 0x3ffffff;
    ::SetErrorMode(oldErrorMode);

    QStringList drives;
    char driveLetter[] = "A";
    while (driveBits)
    {
        if (driveBits & 1)
            drives.append(driveLetter);
        ++driveLetter[0];
        driveBits >>= 1;
    }

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
#else
        const bool hasBackslash = url.contains('\\');
        if (!url.startsWith("/"))
#endif
        {
            QString addPth = pth.isEmpty() ? QDir::currentPath() : pth;
            if (!addPth.endsWith("/"))
                addPth += '/';
            url.prepend(addPth);
        }
#ifndef Q_OS_WIN
        if (hasBackslash && !QFileInfo::exists(url))
            url.replace("\\", "/");
#endif
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

QString Functions::timeToStr(const double t, const bool decimals, const bool milliseconds)
{
    if (t < 0.0)
        return QString();

    const int intT = t;
    const int h = intT / 3600;
    const int m = intT % 3600 / 60;
    const int s = intT % 60;

    QString timStr;
    if (h > 0)
        timStr = QString("%1:").arg(h, 2, 10, QChar('0'));
    timStr += QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
    if (milliseconds)
        timStr += QString(".%1").arg(qRound((t - floor(t)) * 1000.0), 1, 10);
    else if (decimals)
        timStr += QString(".%1").arg(qRound((t - floor(t)) * 10.0), 1, 10);

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
    if (splitPrefixAndUrlIfHasPluginPrefix(f, nullptr, &real_url))
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
#ifdef Q_OS_ANDROID
    if (!extension && f.startsWith("content://"))
        return QFileInfo(f).completeBaseName();
#endif
    while (f.endsWith("/"))
        f.chop(1);
    const QString n = f.right(f.length() - f.lastIndexOf('/') - 1);
    if (extension || (!f.startsWith("QMPlay2://") && !f.startsWith("file://") && f.contains("://")))
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
QString Functions::cleanFileName(QString f, const QString &replaced)
{
    if (f.length() > 200)
        f.resize(200);
    f.replace("/", replaced);
#ifdef Q_OS_WIN
    f.replace("\\", replaced);
    f.replace(":",  replaced);
    f.replace("*",  replaced);
    f.replace("?",  replaced);
    f.replace("\"", replaced);
    f.replace("<",  replaced);
    f.replace(">",  replaced);
    f.replace("|",  replaced);
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

QPixmap Functions::getPixmapFromIcon(const QIcon &icon, QSize size, QWidget *w)
{
    if (icon.isNull() || (size.width() <= 0 && size.height() <= 0))
        return QPixmap();

    const QList<QSize> sizes = icon.availableSizes();

    QSize imgSize;
    if (sizes.isEmpty() || sizes.contains(size))
        imgSize = size;
    else
    {
        imgSize = icon.availableSizes().value(0);
        imgSize.scale(size, size.isEmpty() ? Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio);
    }

    return icon.pixmap(getNativeWindow(w), imgSize);
}
void Functions::drawPixmap(QPainter &p, const QPixmap &pixmap, const QWidget *w, Qt::TransformationMode transformationMode, Qt::AspectRatioMode aRatioMode, QSize size, qreal scale, bool alwaysAllowEnlarge)
{
    if (Q_UNLIKELY(scale <= 0.0))
        return;

    Q_ASSERT(w);

    if (!size.isValid())
        size = w->size();

    QPixmap pixmapToDraw;
    if (!w->isEnabled())
    {
        QStyleOption opt;
        opt.initFrom(w);
        pixmapToDraw = w->style()->generatedIconPixmap(QIcon::Disabled, pixmap, &opt);
    }
    else
    {
        pixmapToDraw = pixmap;
    }

    QSize pixmapSize = size * scale;
    if (!alwaysAllowEnlarge && aRatioMode == Qt::KeepAspectRatio)
    {
        if (pixmapSize.width() > pixmap.width())
            pixmapSize.setWidth(pixmap.width());
        if (pixmapSize.height() > pixmap.height())
            pixmapSize.setHeight(pixmap.height());
    }
    pixmapSize = pixmap.size().scaled(pixmapSize, aRatioMode);

    const bool isSmooth = (transformationMode == Qt::SmoothTransformation);
    const QPoint pixmapPos {
        size.width()  / 2 - pixmapSize.width()  / 2,
        size.height() / 2 - pixmapSize.height() / 2
    };
    if (isSmooth && (pixmapSize.width() < pixmap.width() / 2 || pixmapSize.height() < pixmap.height() / 2))
    {
        const qreal dpr = w->devicePixelRatioF();
        pixmapToDraw = pixmapToDraw.scaled(pixmapSize * dpr, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        pixmapToDraw.setDevicePixelRatio(dpr);
        p.drawPixmap(pixmapPos, pixmapToDraw);
    }
    else
    {
        p.save();
        p.setRenderHint(QPainter::SmoothPixmapTransform, isSmooth);
        p.drawPixmap(QRect(pixmapPos, pixmapSize), pixmapToDraw);
        p.restore();
    }
}

bool Functions::mustRepaintOSD(const QMPlay2OSDList &osd_list, const OsdIdList &osd_ids, const qreal *scaleW, const qreal *scaleH, QRect *bounds)
{
    bool mustRepaint = (osd_list.count() != osd_ids.count());
    for (auto &&osd : osd_list)
    {
        auto locker = osd->lock();
        if (!mustRepaint)
            mustRepaint = !osd_ids.contains(osd->id());
        if (scaleW && scaleH && bounds)
        {
            osd->iterate([&](const QMPlay2OSD::Image &img) {
                if (osd->needsRescale())
                {
                    const auto imgRect = osd->getRect(img);
                    *bounds |= QRectF(
                        imgRect.x() * *scaleW,
                        imgRect.y() * *scaleH,
                        imgRect.width() * *scaleW,
                        imgRect.height() * *scaleH
                    ).toAlignedRect();
                }
                else
                {
                    *bounds |= img.rect.toAlignedRect();
                }
            });
        }
    }
    return mustRepaint;
}
void Functions::paintOSD(bool rgbSwapped, const QMPlay2OSDList &osd_list, const qreal scaleW, const qreal scaleH, QPainter &painter, OsdIdList *osd_ids)
{
    if (osd_ids)
        osd_ids->clear();
    for (auto &&osd : osd_list)
    {
        auto locker = osd->lock();
        if (osd_ids)
            osd_ids->append(osd->id());
        if (osd->needsRescale())
        {
            painter.save();
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            painter.scale(scaleW, scaleH);
        }
        osd->iterate([&](const QMPlay2OSD::Image &img) {
            const QImage qImg = QImage(
                (const uchar *)img.rgba.constData(),
                img.size.width(),
                img.size.height(),
                rgbSwapped ? QImage::Format_RGBA8888 : QImage::Format_ARGB32
            );
            if (osd->needsRescale())
            {
                painter.drawImage(osd->getRect(img), qImg);
            }
            else
            {
                painter.drawImage(img.rect.topLeft(), qImg);
            }
        });
        if (osd->needsRescale())
            painter.restore();
    }
}
void Functions::paintOSDtoYV12(quint8 *imageData, QImage &osdImg, int W, int H, int linesizeLuma, int linesizeChroma, const QMPlay2OSDList &osd_list, OsdIdList &osd_ids)
{
    QRect bounds;
    const int osdW = osdImg.width();
    const int imgH = osdImg.height();
    const qreal iScaleW = (qreal)osdW / W, iScaleH = (qreal)imgH / H;
    const qreal scaleW = (qreal)W / osdW, scaleH = (qreal)H / imgH;
    const bool mustRepaint = Functions::mustRepaintOSD(osd_list, osd_ids, &scaleW, &scaleH, &bounds);
    bounds = QRect(floor(bounds.x() * iScaleW), floor(bounds.y() * iScaleH), ceil(bounds.width() * iScaleW), ceil(bounds.height() * iScaleH)) & QRect(0, 0, osdW, imgH);
    quint32 *osdImgData = (quint32 *)osdImg.constBits();
    if (mustRepaint)
    {
        for (int h = bounds.top(); h <= bounds.bottom(); ++h)
            memset(osdImgData + (h * osdW + bounds.left()), 0, bounds.width() << 2);
        QPainter p(&osdImg);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.scale(iScaleW, iScaleH);
        Functions::paintOSD(false, osd_list, scaleW, scaleH, p, &osd_ids);
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

QPixmap Functions::applyDropShadow(const QPixmap &input, const qreal blurRadius, const QPointF &offset, const QColor &color)
{
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(blurRadius);
    shadow->setOffset(offset);
    shadow->setColor(color);

    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(input);
    item->setGraphicsEffect(shadow);

    QGraphicsScene scene;
    scene.addItem(item);

    QPixmap output(input.size());
    output.fill(Qt::transparent);

    QPainter p(&output);
    scene.render(&p);

    return output;
}
QPixmap Functions::applyBlur(const QPixmap &input, const qreal blurRadius)
{
    QGraphicsBlurEffect *blur = new QGraphicsBlurEffect;
    blur->setBlurHints(QGraphicsBlurEffect::PerformanceHint);
    blur->setBlurRadius(blurRadius);

    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(input);
    item->setGraphicsEffect(blur);

    QGraphicsScene scene;
    scene.addItem(item);

    QPixmap blurred(input.size());
    blurred.fill(Qt::black);

    QPainter p(&blurred);
    scene.render(&p);

    return blurred;
}

void Functions::ImageEQ(int Contrast, int Brightness, quint8 *imageBits, unsigned bitsCount)
{
    auto clip8 = [](int val)->quint8 {
        return val > 255 ? (quint8)255 : (val < 0 ? (quint8)0 : val);
    };
    for (unsigned i = 0; i < bitsCount; i += 4)
    {
        imageBits[i+0] = clip8((imageBits[i+0] - 127) * Contrast / 100 + 127 + Brightness);
        imageBits[i+1] = clip8((imageBits[i+1] - 127) * Contrast / 100 + 127 + Brightness);
        imageBits[i+2] = clip8((imageBits[i+2] - 127) * Contrast / 100 + 127 + Brightness);
    }
}
int Functions::scaleEQValue(int val, int min, int max)
{
    return (val + 100) * ((abs(min) + abs(max))) / 200 - abs(min);
}

QByteArray Functions::convertToASS(QString txt)
{
    txt.replace("&nbsp;", " ", Qt::CaseInsensitive);
    txt.replace("&lt;", "<", Qt::CaseInsensitive);
    txt.replace("&gt;", ">", Qt::CaseInsensitive);
    txt.replace("&amp;", "&", Qt::CaseInsensitive);
    txt.replace("&quot;", "\"", Qt::CaseInsensitive);
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

    // Colors
    const QRegularExpression colorRegExp(
        R"(<font\s+color\s*=\s*\"?\#?(\w{6})\"?\s*>(.*)<\/font\s*>)",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::InvertedGreedinessOption
    );
    int pos = 0;
    for (;;)
    {
        const auto match = colorRegExp.match(txt, pos);
        if (!match.hasMatch())
            break;

        pos = match.capturedStart();

        QString rgb = match.captured(1);
        rgb = rgb.mid(4, 2) + rgb.mid(2, 2) + rgb.mid(0, 2);

        const QString replaced = "{\\1c&" + rgb + "&}" + match.captured(2) + "{\\1c}";

        txt.replace(pos, match.capturedLength(), replaced);
        pos += replaced.length();
    }

    return txt.toUtf8();
}

bool Functions::chkMimeData(const QMimeData *mimeData)
{
    return mimeData && ((mimeData->hasUrls() && !mimeData->urls().isEmpty()) || (mimeData->hasText() && !mimeData->text().isEmpty()));
}
QStringList Functions::getUrlsFromMimeData(const QMimeData *mimeData, const bool checkExtensionsForUrl)
{
    QStringList urls;
    if (mimeData->hasUrls())
    {
        for (const QUrl &url : mimeData->urls())
        {
            const bool isLocalFile = url.isLocalFile();
            QString u = isLocalFile ? url.toLocalFile() : url.toString();
            if (isLocalFile && u.length() > 1 && u.endsWith("/"))
                u.chop(1);
            if (!u.isEmpty())
                urls += u;
        }
    }
    else if (mimeData->hasText())
    {
        urls = mimeData->text().remove('\r').split('\n', Qt::SkipEmptyParts);
    }
    if (checkExtensionsForUrl)
    {
        for (QString &url : urls)
            url = Functions::maybeExtensionAddress(url);
    }
    return urls;
}

QString Functions::maybeExtensionAddress(const QString &url)
{
    for (const QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
    {
        const QString prefix = QMPlay2Ext->matchAddress(url);
        if (!prefix.isEmpty())
            return prefix + "://{" + url + "}";
    }
    return url;
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
void Functions::getDataIfHasPluginPrefix(const QString &entireUrl, QString *url, QString *name, QIcon *icon, IOController<> *ioCtrl, const DemuxersInfo &demuxersInfo)
{
    QString addressPrefixName, realUrl, param;
    if ((url || icon) && splitPrefixAndUrlIfHasPluginPrefix(entireUrl, &addressPrefixName, &realUrl, &param))
    {
        for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
            if (QMPlay2Ext->addressPrefixList(false).contains(addressPrefixName))
            {
                QMPlay2Ext->convertAddress(addressPrefixName, realUrl, param, url, name, icon, nullptr, ioCtrl);
                return;
            }
    }
    if (icon)
    {
        const QString scheme = getUrlScheme(entireUrl);
        const QString extension = fileExt(entireUrl).toLower();
        if (demuxersInfo.isEmpty())
        {
            for (const Module *module : QMPlay2Core.getPluginsInstance())
                for (const Module::Info &mod : module->getModulesInfo())
                    if (mod.type == Module::DEMUXER && (mod.name == scheme || mod.extensions.contains(extension)))
                    {
                        *icon = !mod.icon.isNull() ? mod.icon : module->icon();
                        return;
                    }
        }
        else
        {
            for (const DemuxerInfo &demuxerInfo : demuxersInfo)
                if (demuxerInfo.name == scheme || demuxerInfo.extensions.contains(extension))
                {
                    *icon = demuxerInfo.icon;
                    return;
                }
        }
    }
}
bool Functions::isResourcePlaylist(const QString &url)
{
    return url.startsWith("QMPlay2://") && url.endsWith(".pls") && (url.indexOf("/", 10) > 0);
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
    return (!a ? "-∞" : QString::number(20.0 * log10(a), 'f', 1)) + " dB";
}

quint32 Functions::getBestSampleRate()
{
    quint32 srate = 48000; //Use 48kHz as default
    if (QMPlay2Core.getSettings().getBool("ForceSamplerate"))
    {
        const quint32 chosenSrate = QMPlay2Core.getSettings().getUInt("Samplerate");
        if ((chosenSrate % 11025) == 0)
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

QByteArray Functions::getUserAgent(bool withMozilla)
{
    const auto customUserAgent = QMPlay2Core.getSettings().getString("CustomUserAgent");
    if (!customUserAgent.isEmpty())
        return customUserAgent.toUtf8();
    return withMozilla ? Version::userAgentWithMozilla() : Version::userAgent();
}
QString Functions::prepareFFmpegUrl(QString url, AVDictionary *&options, bool defUserAgentWithMozilla, bool setCookies, bool setRawHeaders, bool icy, const QByteArray &userAgentArg)
{
    if (url.startsWith("file://"))
    {
        url.remove(0, 7);
    }
    else
    {
        const QByteArray cookies = setCookies ? QMPlay2Core.getCookies(url) : QByteArray();
        const QByteArray rawHeaders = setRawHeaders ? QMPlay2Core.getRawHeaders(url) : QByteArray();
        const QByteArray userAgent = [&] {
            if (!userAgentArg.isNull())
                return userAgentArg;
            return getUserAgent(defUserAgentWithMozilla);
        }();

        if (url.startsWith("mms:"))
            url.insert(3, 'h');

        if (url.startsWith("http"))
            av_dict_set(&options, "icy", icy ? "1" : "0", 0);
        av_dict_set(&options, "user_agent", userAgent, 0);

        if (!cookies.isEmpty())
            av_dict_set(&options, "headers", QByteArray("Cookie: " + cookies + "\r\n").constData(), 0);
        if (!rawHeaders.isEmpty())
            av_dict_set(&options, "headers", rawHeaders, 0);

        av_dict_set(&options, "reconnect", "1", 0);
    }
    return url;
}

QByteArray Functions::textWithFallbackEncoding(const QByteArray &data)
{
    QTextCodec *codec = QTextCodec::codecForUtfText(data, QTextCodec::codecForName(QMPlay2Core.getSettings().getByteArray("FallbackSubtitlesEncoding")));
    if (codec && codec->name() != "UTF-8")
    {
        QTextCodec *utf8Codec = QTextCodec::codecForName("UTF-8");
        QTextCodec::ConverterState state;
        if (utf8Codec)
            utf8Codec->toUnicode(data, data.size(), &state); //Try to detect if it is a UTF-8 text
        if (!utf8Codec || state.invalidChars > 0) //Not a UTF-8 text, use a fallback text codec
            return codec->toUnicode(data).toUtf8();
    }
    return data;
}

QMatrix4x4 Functions::getYUVtoRGBmatrix(AVColorSpace colorSpace)
{
    const auto rbCoeff = [&]()->pair<float, float> {
        switch (colorSpace)
        {
            case AVCOL_SPC_BT709:
                return {0.2126f, 0.0722f};
            case AVCOL_SPC_SMPTE170M:
                return {0.299f, 0.114f};
            case AVCOL_SPC_SMPTE240M:
                return {0.212f, 0.087f};
            case AVCOL_SPC_BT2020_CL:
            case AVCOL_SPC_BT2020_NCL:
                return {0.2627f, 0.0593f};
            default:
                break;
        }
        return {0.299f, 0.114f}; // AVCOL_SPC_BT470BG
    }();

    const float cR = rbCoeff.first;
    const float cB = rbCoeff.second;
    const float cG = 1.0f - cR - cB;

    const float bscale = 0.5f / (cB - 1.0f);
    const float rscale = 0.5f / (cR - 1.0f);

    auto mat = QMatrix4x4(
       cR,          cG,          cB,          0.0f,
       bscale * cR, bscale * cG, 0.5f,        0.0f,
       0.5f,        rscale * cG, rscale * cB, 0.0f,
       0.0f,        0.0f,        0.0f,        1.0f
    ).inverted();

    return mat;
}

bool Functions::isColorPrimariesSupported(AVColorPrimaries colorPrimaries)
{
    switch (colorPrimaries)
    {
        case AVCOL_PRI_BT709:
        case AVCOL_PRI_BT2020:
            return true;
        default:
            break;
    }
    return false;
}
bool Functions::fillColorPrimariesData(AVColorPrimaries colorPrimaries, QVector2D &wp, array<QVector2D, 3> &primaries)
{
    switch (colorPrimaries)
    {
        case AVCOL_PRI_BT709:
        {
            primaries[0] = QVector2D(0.64f, 0.33f);
            primaries[1] = QVector2D(0.30f, 0.60f);
            primaries[2] = QVector2D(0.15f, 0.06f);
            break;
        }
        case AVCOL_PRI_BT2020:
        {
            primaries[0] = QVector2D(0.708f, 0.292f);
            primaries[1] = QVector2D(0.170f, 0.797f);
            primaries[2] = QVector2D(0.131f, 0.046f);
            break;
        }
        default:
        {
            return false;
        }
    }
    wp = QVector2D(0.31271f, 0.32902f); // D65
    return true;
}

QMatrix4x4 Functions::getColorPrimariesTo709Matrix(const QVector2D &wp, array<QVector2D, 3> &primaries)
{
    // D65 is only supported for now

    auto getRgbToXyzMatrix = [](const QVector2D &wp, const QVector2D &r, const QVector2D &g, const QVector2D &b) {
        auto xyToXyz = [](const QVector2D &v) {
            return QVector3D(
                v[0] / v[1],
                1.0f,
                (1.0f - v[0] - v[1]) / v[1]
            );
        };

        const auto wpXyz = xyToXyz(wp);

        const auto ry = r[1];
        const auto gy = g[1];
        const auto by = b[1];

        const auto Xr = r[0] / ry;
        const auto Yr = 1.0f;
        const auto Zr = (1.0f - r[0] - r[1]) / ry;

        const auto Xg = g[0] / gy;
        const auto Yg = 1.0f;
        const auto Zg = (1.0f - g[0] - g[1]) / gy;

        const auto Xb = b[0] / by;
        const auto Yb = 1.0f;
        const auto Zb = (1.0f - b[0] - b[1]) / by;

        const auto S = QMatrix4x4(
            Xr,   Xg,   Xb,   0.0f,
            Yr,   Yg,   Yb,   0.0f,
            Zr,   Zg,   Zb,   0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        ).inverted().map(wpXyz);

        return QMatrix4x4(
            S[0] * Xr, S[1] * Xg, S[2] * Xb, 0.0f,
            S[0] * Yr, S[1] * Yg, S[2] * Yb, 0.0f,
            S[0] * Zr, S[1] * Zg, S[2] * Zb, 0.0f,
            0.0f,      0.0f,      0.0f,      1.0f
        );
    };

    QVector2D dstWp;
    array<QVector2D, 3> dstPrimaries;
    fillColorPrimariesData(AVCOL_PRI_BT709, dstWp, dstPrimaries);

    const auto xyzTo709 = getRgbToXyzMatrix(dstWp, dstPrimaries[0], dstPrimaries[1], dstPrimaries[2]).inverted();
    const auto srcToXyz = getRgbToXyzMatrix(wp, primaries[0], primaries[1], primaries[2]);
    return xyzTo709 * srcToXyz;
}
QMatrix4x4 Functions::getColorPrimariesTo709Matrix(AVColorPrimaries colorPrimaries)
{
    if (colorPrimaries == AVCOL_PRI_BT709)
        return QMatrix4x4();

    QVector2D wp;
    array<QVector2D, 3> primaries;
    const bool ok = Functions::fillColorPrimariesData(
        colorPrimaries,
        wp,
        primaries
    );
    if (!ok)
        return QMatrix4x4();

    return getColorPrimariesTo709Matrix(wp, primaries);
}

bool Functions::compareText(const QString &a, const QString &b)
{
    QRegularExpression rx(R"(\d+)");

    auto fillMatches = [](auto &&matchIt, auto &&matches) {
        while (matchIt.hasNext())
        {
            const auto match = matchIt.next();
            matches.emplace_back(match.capturedStart(), match.captured().length());
        }
    };

    std::vector<std::pair<int, int>> matchesA, matchesB;
    fillMatches(rx.globalMatch(a), matchesA);
    fillMatches(rx.globalMatch(b), matchesB);

    const int n = qMin(matchesA.size(), matchesB.size());
    if (n > 0)
    {
        auto newA = a;
        auto newB = b;
        for (int i = n - 1; i >= 0; --i)
        {
            const auto &matchA = matchesA[i];
            const auto &matchB = matchesB[i];

            const int lenA = matchA.second;
            const int lenB = matchB.second;

            const int diff = qAbs(lenA - lenB);

            if (diff > 0)
            {
                const QString zeros(diff, QChar('0'));
                if (lenA > lenB)
                {
                    newB.insert(matchB.first, zeros);
                }
                else if (lenB > lenA)
                {
                    newA.insert(matchA.first, zeros);
                }
            }
        }
        return (newA.toLower() < newB.toLower());
    }

    return (a.toLower() < b.toLower());
}

bool Functions::hasTouchScreen()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const auto inputDevs = QInputDevice::devices();
    for (auto &&inputDev : inputDevs)
    {
        if (inputDev->type() == QInputDevice::DeviceType::TouchScreen)
        {
            return true;
        }
    }
#else
    const auto touchDevs = QTouchDevice::devices();
    for (auto &&touchDev : touchDevs)
    {
        if (touchDev->type() == QTouchDevice::TouchScreen)
        {
            return true;
        }
    }
#endif
    return false;
}

QString Functions::getBitrateStr(const int64_t bitRate)
{
    if (bitRate <= 0)
        return QString();
    if (bitRate < 1000)
        return QString("%1 bps").arg(bitRate);
    if (bitRate < 1000000)
        return QString("%1 kbps").arg(qRound64(bitRate / 1000.0));
    return QString("%1 Mbps").arg(bitRate / 1000000.0, 0, 'f', 3);
}

QString Functions::getSeqFile(const QString &dir, const QString &ext, const QString &frag)
{
    quint16 num = 0;
    for (const QString &f : QDir(dir).entryList({QString("QMPlay2_%1_?????%2").arg(frag, ext)}, QDir::Files, QDir::Name))
    {
        const quint16 n = QStringView(f).mid(8 + frag.size() + 1, 5).toUShort();
        if (n > num)
            num = n;
    }
    return QString("QMPlay2_%1_%2%3").arg(frag).arg(++num, 5, 10, QChar('0')).arg(ext);
}

std::pair<QString, QString> Functions::determineExtFmt(const QList<StreamInfo *> &streamsInfo)
{
    QString ext("mkv");
    QString fmt("matroska");

    if (streamsInfo.size() == 1)
    {
        switch (streamsInfo.at(0)->params->codec_id)
        {
            case AV_CODEC_ID_MP3:
                ext = fmt = "mp3";
                break;
            case AV_CODEC_ID_AAC:
                ext = "aac";
                fmt = "adts";
                break;
            case AV_CODEC_ID_AC3:
                ext = fmt = "ac3";
                break;
            case AV_CODEC_ID_VORBIS:
                ext = fmt = "ogg";
                break;
            case AV_CODEC_ID_OPUS:
                ext = "opus";
                fmt  = "ogg";
                break;
            case AV_CODEC_ID_H264:
            case AV_CODEC_ID_HEVC:
                ext = "ts";
                fmt = "mpegts";
                break;
            default:
                break;
        }
    }
    else if (streamsInfo.size() == 2)
    {
        const QSet<AVCodecID> codecIDs {
            streamsInfo.at(0)->params->codec_id,
            streamsInfo.at(1)->params->codec_id,
        };
        if ((codecIDs.contains(AV_CODEC_ID_AAC) || codecIDs.contains(AV_CODEC_ID_MP3) || codecIDs.contains(AV_CODEC_ID_AC3))
                && (codecIDs.contains(AV_CODEC_ID_H264) || codecIDs.contains(AV_CODEC_ID_HEVC)))
        {
            ext = "ts";
            fmt = "mpegts";
        }
    }

    return {ext, fmt};
}
