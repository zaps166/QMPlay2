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

#include <VisWidget.hpp>
#include <QMPlay2Core.hpp>
#include <DockWidget.hpp>
#include <Functions.hpp>

#include <QMouseEvent>
#include <QPainter>
#include <QMenu>

#include <cmath>

void VisWidget::setValue(qreal &out, qreal in, qreal tDiffScaled)
{
    if (in < out)
        out -= sqrt(out) * tDiffScaled;
    else
        out = in;
}
void VisWidget::setValue(QPair<qreal, double> &out, qreal in, qreal tDiffScaled)
{
    if (in < out.first)
        out.first -= (Functions::gettime() - out.second) * tDiffScaled;
    else
    {
        out.first = in;
        out.second = Functions::gettime();
    }
}

VisWidget::VisWidget()
    : stopped(true)
    , dw(new DockWidget)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    connect(&tim, SIGNAL(timeout()), this, SLOT(updateVisualization()));
    connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));
    connect(&QMPlay2Core, SIGNAL(wallpaperChanged(bool, double)), this, SLOT(wallpaperChanged(bool, double)));
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenu(const QPoint &)));
}

bool VisWidget::canStart() const
{
    return ((dockWidgetVisible && isVisible()) || parentWidget() != dw);
}

void VisWidget::stop()
{
#ifdef USE_OPENGL
    if (glW)
        m_pendingUpdate = true;
#endif
    updateVisualization();
}

void VisWidget::setUseOpenGL(bool b)
{
#ifdef USE_OPENGL
    m_pendingUpdate = false;
    if (b && !glW)
    {
        glW = new QOpenGLWidget(this);
        glW->setAttribute(Qt::WA_TransparentForMouseEvents);
        glW->setContextMenuPolicy(Qt::NoContextMenu);
        glW->setFocusPolicy(Qt::NoFocus);
        glW->setAutoFillBackground(true);
        glW->show();
        glW->installEventFilter(this);
        glW->setGeometry(QRect(QPoint(), size()));
    }
    else if (!b && glW)
    {
        delete glW;
        glW = nullptr;
    }
#else
    Q_UNUSED(b)
#endif
}

void VisWidget::resizeEvent(QResizeEvent *e)
{
#ifdef USE_OPENGL
    if (glW)
        glW->setGeometry(QRect(QPoint(), size()));
#endif
    QWidget::resizeEvent(e);
}

void VisWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (parent() == dw && (e->buttons() & Qt::LeftButton))
        emit doubleClicked();
    else
        QWidget::mouseDoubleClickEvent(e);
}
void VisWidget::paintEvent(QPaintEvent *)
{
#ifdef USE_OPENGL
    if (glW)
        return;
#endif
    QPainter p(this);
    p.fillRect(rect(), Qt::black);
    paint(p);
}
void VisWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::ParentChange && !parent())
        dw->setWidget(this);
    QWidget::changeEvent(event);
}
void VisWidget::wheelEvent(QWheelEvent *e)
{
    QMPlay2Core.processWheelEvent(e);
    QWidget::wheelEvent(e);
}

bool VisWidget::eventFilter(QObject *watched, QEvent *event)
{
#ifdef USE_OPENGL
    if (glW && watched == glW && event->type() == QEvent::Paint)
    {
        QPainter p(glW);
        p.fillRect(rect(), Qt::black);
        paint(p);
        m_pendingUpdate = false;
        return true;
    }
#endif
    return QWidget::eventFilter(watched, event);
}

void VisWidget::wallpaperChanged(bool hasWallpaper, double alpha)
{
    QColor c = Qt::black;
    if (hasWallpaper)
        c.setAlphaF(alpha);
    setPalette(c);
}
void VisWidget::contextMenu(const QPoint &point)
{
    QMenu *menu = new QMenu(this);
    connect(menu, SIGNAL(aboutToHide()), menu, SLOT(deleteLater()));
    connect(menu->addAction(tr("Settings")), SIGNAL(triggered()), this, SLOT(showSettings()));
    menu->popup(mapToGlobal(point));
}
void VisWidget::visibilityChanged(bool v)
{
    dockWidgetVisible = v;
    if (!canStart())
        stop();
    else if (!stopped)
        start();
#ifdef USE_OPENGL
    else if (dockWidgetVisible && m_pendingUpdate)
        updateVisualization();
#endif
}
void VisWidget::updateVisualization()
{
#ifdef USE_OPENGL
    if (glW)
    {
        glW->update();
        return;
    }
#endif
    update();
}
void VisWidget::showSettings()
{
    emit QMPlay2Core.showSettings("Visualizations");
}
