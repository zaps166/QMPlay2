/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <InDockW.hpp>

#include <Functions.hpp>
#include <Settings.hpp>

#include <QCoreApplication>
#ifdef Q_OS_WIN
#   include <QApplication>
#endif
#include <QDockWidget>
#include <QPainter>
#include <QVariant>

#include <cmath>

InDockW::InDockW(const QColor &grad1, const QColor &grad2, const QColor &qmpTxt) :
    grad1(grad1), grad2(grad2), qmpTxt(qmpTxt),
    hasWallpaper(false),
    loseHeight(0),
    w(nullptr)
{
    connect(&QMPlay2Core, SIGNAL(wallpaperChanged(bool, double)), this, SLOT(wallpaperChanged(bool, double)));
    setAttribute(Qt::WA_OpaquePaintEvent);
    setFocusPolicy(Qt::StrongFocus);
    grabGesture(Qt::PinchGesture);
    setAutoFillBackground(true);
    setMouseTracking(true);
}

QWidget *InDockW::getWidget()
{
    return w;
}
void InDockW::setLoseHeight(int lh)
{
    loseHeight = lh;
}
void InDockW::setCustomPixmap(const QPixmap &pix)
{
    customPixmap = pix;
    if (customPixmap.isNull() || !QMPlay2Core.getSettings().getBool("BlurCovers"))
        customPixmapBlurred = QPixmap();
    else
    {
        const qreal blurRadius = qBound(10.0, sqrt(pix.width() * pix.width() + pix.height() * pix.height()) / 4.0, 300.0);
        blurredTransformation = (blurRadius < 80.0) ? Qt::SmoothTransformation : Qt::FastTransformation;
        customPixmapBlurred = Functions::applyBlur(pix, blurRadius);
        m_enlargeCovers = QMPlay2Core.getSettings().getBool("EnlargeCovers");
    }
    emit hasCoverImage(!customPixmap.isNull());
    update();
}

void InDockW::wallpaperChanged(bool wallpaper, double alpha)
{
    QColor c = Qt::black;
    hasWallpaper = wallpaper;
    setAttribute(Qt::WA_OpaquePaintEvent, !wallpaper);
    if (wallpaper)
        c.setAlphaF(alpha);
    setPalette(c);
}

void InDockW::setWidget(QWidget *newW)
{
    if (w == newW)
        return;
    if (w)
        w->hide();
    w = newW;
    if (w)
    {
        w->setMinimumSize(2, 2);
        w->setParent(this);
        resizeEvent(nullptr);
        w->setCursor(w->cursor()); // Force cursor shape
        w->show();
    }
}

void InDockW::resizeEvent(QResizeEvent *)
{
    if (w)
    {
        int X = 0;
        int Y = 0;
        int W = width();
        int H = height() + loseHeight;

        const int mappedY = mapToParent(QPoint()).y();
        if (mappedY < 0)
        {
            H += mappedY;
            Y -= mappedY;
        }

#ifdef Q_OS_WIN
        const int bypassCompositor = w->property("bypassCompositor").toInt();
        if ((bypassCompositor == -1 || (bypassCompositor == 1 && (loseHeight > 0 || QApplication::activePopupWidget()))) && window()->property("fullScreen").toBool())
        {
            X -= 1;
            W += 2;
        }
#endif

        if (w->geometry() != QRect(X, Y, W, H))
        {
            w->setGeometry(X, Y, W, H);
            emit resized(W, H);
        }
    }
}
void InDockW::paintEvent(QPaintEvent *)
{
    if (!w)
    {
        QPainter p(this);

        bool isFloating = false;
        if (QDockWidget *dW = qobject_cast<QDockWidget *>(parentWidget()))
            isFloating = dW->isFloating();

        const int fullHeight = height() + loseHeight;
        const bool drawBackground = (isFloating || !hasWallpaper);
        const bool drawBlurredImage = drawBackground && !customPixmapBlurred.isNull();

        if (drawBackground && !drawBlurredImage)
        {
            if (grad1 == grad2)
                p.fillRect(rect(), grad1);
            else
            {
                QLinearGradient lgrad(width() / 2, 0, width() / 2, fullHeight);
                lgrad.setColorAt(0.0, grad1);
                lgrad.setColorAt(0.5, grad2);
                lgrad.setColorAt(1.0, grad1);
                p.fillRect(rect(), lgrad);
            }
        }

        if (customPixmap.isNull())
        {
            const QSize size(128, 128);
            QPixmap qmp2Pixmap = Functions::getPixmapFromIcon(QMPlay2Core.getQMPlay2Icon(), size, this);

            p.drawPixmap(width() / 2 - size.width() / 2, fullHeight / 2 - size.height() / 2, qmp2Pixmap);

            QFont font = p.font();
            font.setPointSize(22);
            font.setItalic(true);
            p.setFont(font);
            p.setPen(qmpTxt);
            p.drawText(0, fullHeight / 2 + size.height() / 2, width(), 100, Qt::AlignHCenter | Qt::AlignTop, "QMPlay2");
        }
        else
        {
            const QSize drawSize(width(), fullHeight);
            qreal scale = 1.0;
            if (drawBlurredImage)
            {
                Functions::drawPixmap(p, customPixmapBlurred, this, blurredTransformation, Qt::KeepAspectRatioByExpanding, drawSize);
                scale = 0.8;
            }
            Functions::drawPixmap(p, customPixmap, this, Qt::SmoothTransformation, Qt::KeepAspectRatio, drawSize, scale, m_enlargeCovers);
        }
    }
}
void InDockW::enterEvent(QEvent *)
{
#ifdef Q_OS_WIN
    // For context menu
    resizeEvent(nullptr);
#endif
}
void InDockW::leaveEvent(QEvent *)
{
#ifdef Q_OS_WIN
    // For context menu
    resizeEvent(nullptr);
#endif
}
bool InDockW::event(QEvent *e)
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
