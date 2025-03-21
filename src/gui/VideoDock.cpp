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

#include <VideoDock.hpp>

#include <VideoAdjustmentW.hpp>
#include <Settings.hpp>
#include <MenuBar.hpp>
#include <Main.hpp>

#include <Functions.hpp>
#include <SubsDec.hpp>

#include <QVersionNumber>
#include <QMouseEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QPainter>
#include <QGesture>
#include <QMenu>

#include <cmath>

constexpr int g_hideCursorTimeout = 750;

VideoDock::VideoDock() :
    isTouch(false), touchEnded(false),
    iDW(QMPlay2GUI.grad1, QMPlay2GUI.grad2, QMPlay2GUI.qmpTxt),
    pixels(0),
    canPopup(true), is_floating(false),
    touchZoom(0.0)
{
    setWindowTitle(tr("Video"));

    popupMenu = new QMenu(this);
    popupMenu->addMenu(QMPlay2GUI.menuBar->window);
    popupMenu->addMenu(QMPlay2GUI.menuBar->widgets);
    popupMenu->addMenu(QMPlay2GUI.menuBar->playlist);
    popupMenu->addMenu(QMPlay2GUI.menuBar->player);
    popupMenu->addMenu(QMPlay2GUI.menuBar->playback);
    popupMenu->addMenu(QMPlay2GUI.menuBar->options);
    popupMenu->addMenu(QMPlay2GUI.menuBar->help);

    /* Menu actions which will be available in fullscreen or compact mode */
    for (QAction *act : QMPlay2GUI.menuBar->window->actions())
        addAction(act);
    for (QAction *act : QMPlay2GUI.menuBar->playlist->actions())
        addAction(act);
    for (QAction *act : QMPlay2GUI.menuBar->player->actions())
        addAction(act);
    for (QAction *act : QMPlay2GUI.menuBar->playback->actions())
        addAction(act);
    for (QAction *act : QMPlay2GUI.menuBar->options->actions())
        addAction(act);
    for (QAction *act : QMPlay2GUI.menuBar->help->actions())
        addAction(act);
    QMPlay2GUI.videoAdjustment->addActionsToWidget(this);
    /**/

    setMouseTracking(true);
    setAcceptDrops(true);
    grabGesture(Qt::PinchGesture);
    setContextMenuPolicy(Qt::CustomContextMenu);

    setWidget(&iDW);

    hideCursorTim.setSingleShot(true);
    leftButtonPlayTim.setSingleShot(true);

    connect(&hideCursorTim, SIGNAL(timeout()), this, SLOT(hideCursor()));
    connect(&leftButtonPlayTim, SIGNAL(timeout()), QMPlay2GUI.menuBar->player->togglePlay, SLOT(trigger()));
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popup(const QPoint &)));
    connect(&iDW, &InDockW::resized, this, &VideoDock::resized);
    connect(&iDW, SIGNAL(hasCoverImage(bool)), this, SLOT(hasCoverImage(bool)));
    connect(this, SIGNAL(dockVisibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));
    connect(&QMPlay2Core, &QMPlay2CoreClass::dockVideo, this, [this](QWidget *w) {
        iDW.setWidget(w);
        mouseMoveEvent(nullptr);
    });

    canHideIDWCursor = false;
    doubleClicked = false;
}

void VideoDock::scheduleEnterEventWorkaround()
{
    if (QVersionNumber::fromString(qVersion()) >= QVersionNumber(5, 12, 0))
    {
        // Something is wrong with enter/leave events after going full screen  or maximized in some configurations since Qt 5.12.
        // Create a new temporary widget which fills entire parent widget to trigger enter/leave events.
        QTimer::singleShot(0, this, [this] {
            QTimer::singleShot(0, this, [this] {
                if (!underMouse())
                {
                    QWidget tmp(this);
                    tmp.setGeometry(rect());
                    tmp.show();
                }
            });
        });
    }
}
void VideoDock::maybeTriggerWidgetVisibility()
{
    if (auto w = iDW.getWidget(); w && w->property("loseHeight").isValid())
    {
        // Hide/show widget to not hide mouse cursor outside the window when native widgets are not used (workaround)
        QTimer::singleShot(0, w, [w] {
            w->setVisible(false);
            w->setVisible(true);
        });
    }
}

void VideoDock::fullScreen(bool b)
{
    if (b)
    {
        is_floating = isFloating();

        setTitleBarVisible(false);
        setFeatures(DockWidget::NoDockWidgetFeatures);
        setFloating(false);

        m_contentMarginsBackup = contentsMargins();
        setContentsMargins(0, 0, 0, 0);

        // FIXME: Is it still needed for something?
        setStyle(&commonStyle);
    }
    else
    {
        /* Visualizations on full screen */
        QWidget *dockedW = widget();
        if (dockedW != &iDW)
        {
            if (dockedW)
            {
                unsetCursor(dockedW);
                dockedW->setParent(nullptr);
            }
            setWidget(&iDW);
        }

        setTitleBarVisible(true);
        setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        setFloating(is_floating);

        setContentsMargins(m_contentMarginsBackup);

        setStyle(nullptr);
    }
}

QWidget *VideoDock::internalWidget()
{
    QWidget *w = widget();
    if (w == &iDW) //Not a visualization
    {
        QWidget *dw = iDW.getWidget();
        if (!dw && canHideIDWCursor)
            return w;
        return dw;
    }
    return w;
}

void VideoDock::unsetCursor(QWidget *w)
{
    bool ok;
    const int cursorShape = w->property("customCursor").toInt(&ok);
    if (ok && cursorShape >= 0 && cursorShape <= Qt::LastCursor)
        w->setCursor((Qt::CursorShape)cursorShape);
    else
        w->setCursor(Qt::ArrowCursor);
}

void VideoDock::dragEnterEvent(QDragEnterEvent *e)
{
    if (e)
    {
        e->accept();
        QWidget::dragEnterEvent(e);
    }
}
void VideoDock::dropEvent(QDropEvent *e)
{
    if (e)
    {
        const QMimeData *mimeData = e->mimeData();
        if (Functions::chkMimeData(mimeData))
        {
            const QStringList urls = Functions::getUrlsFromMimeData(mimeData, false);
            if (urls.size() == 1)
            {
                QString url = Functions::Url(urls[0]);
#ifdef Q_OS_WIN
                if (url.startsWith("file://"))
                    url.remove(0, 7);
#endif
                if (!QFileInfo(url.mid(7)).isDir())
                {
                    bool subtitles = false;
                    QString fileExt = Functions::fileExt(url).toLower();
                    if (!fileExt.isEmpty() && (fileExt == "ass" || fileExt == "ssa" || SubsDec::extensions().contains(fileExt)))
                        subtitles = true;
#ifdef Q_OS_WIN
                    if (subtitles && !url.contains("://"))
                        url.prepend("file://");
#endif
                    emit itemDropped(url, subtitles);
                    e->accept();
                }
            }
        }
        QWidget::dropEvent(e);
    }
}
void VideoDock::mouseMoveEvent(QMouseEvent *e)
{
    if (QWidget *internalW = internalWidget())
    {
        if (internalW->cursor().shape() == Qt::BlankCursor && ++pixels == 25)
            unsetCursor(internalW);
        hideCursorTim.start(g_hideCursorTimeout);
    }
    if (e)
        DockWidget::mouseMoveEvent(e);
}
void VideoDock::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->buttons() == Qt::LeftButton)
    {
#ifndef Q_OS_MACOS
        QMPlay2GUI.menuBar->window->toggleFullScreen->trigger();
#else
        // On macOS if full screen is toggled to fast after double click, mouse remains in clicked state...
        QTimer::singleShot(200, QMPlay2GUI.menuBar->window->toggleFullScreen, SLOT(trigger()));
#endif
        doubleClicked = true;
    }
    DockWidget::mouseDoubleClickEvent(e);
}
void VideoDock::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        canPopup = false;
    if (e->buttons() == Qt::MiddleButton)
        (!QMPlay2Core.getSettings().getBool("MiddleMouseToggleFullscreen") ? QMPlay2GUI.menuBar->player->togglePlay : QMPlay2GUI.menuBar->window->toggleFullScreen)->trigger();
    else if ((e->buttons() & (Qt::LeftButton | Qt::MiddleButton)) == (Qt::LeftButton | Qt::MiddleButton))
        QMPlay2GUI.menuBar->player->reset->trigger();
    else if ((e->buttons() & (Qt::LeftButton | Qt::RightButton)) == (Qt::LeftButton | Qt::RightButton))
        QMPlay2GUI.menuBar->player->switchARatio->trigger();
    m_pressedKeyModifiers = e->modifiers();
    DockWidget::mousePressEvent(e);
}
void VideoDock::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        canPopup = true;
        if (doubleClicked)
        {
            doubleClicked = false;
            leftButtonPlayTim.stop();
        }
        else if (m_pressedKeyModifiers == Qt::NoModifier)
        {
            const int val = QMPlay2Core.getSettings().getInt("LeftMouseTogglePlay");
            if (val > 0)
                leftButtonPlayTim.start((val == 1) ? 300 : 0);
        }
    }
    if (QWidget *internalW = internalWidget())
    {
        if (internalW->cursor().shape() != Qt::BlankCursor)
            hideCursorTim.start(g_hideCursorTimeout);
    }
    DockWidget::mouseReleaseEvent(e);
}
void VideoDock::moveEvent(QMoveEvent *e)
{
    if (isFloating())
        emit QMPlay2Core.videoDockMoved();
    DockWidget::moveEvent(e);
}
void VideoDock::wheelEvent(QWheelEvent *e)
{
    const auto deltaY  = e->angleDelta().y();
    auto player = QMPlay2GUI.menuBar->player;
    if (deltaY != 0 && (e->buttons() & Qt::LeftButton))
        deltaY > 0 ? player->zoom->zoomIn->trigger() : player->zoom->zoomOut->trigger();
    else
        QMPlay2Core.processWheelEvent(e);
    DockWidget::wheelEvent(e);
}
void VideoDock::leaveEvent(QEvent *e)
{
    hideCursorTim.stop();
    if (QWidget *internalW = internalWidget())
    {
        if (internalW->cursor().shape() == Qt::BlankCursor)
            unsetCursor(internalW);
    }
    pixels = 0;
    DockWidget::leaveEvent(e);
}
void VideoDock::enterEvent(Q_ENTER_EVENT *e)
{
    mouseMoveEvent(nullptr);
    DockWidget::enterEvent(e);
}
bool VideoDock::event(QEvent *e)
{
    switch (e->type())
    {
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
            isTouch = true;
            touchEnded = false;
            break;
        case QEvent::TouchEnd:
            touchEnded = true;
            break;
        case QEvent::Gesture:
            if (QPinchGesture *pinch = (QPinchGesture *)((QGestureEvent *)e)->gesture(Qt::PinchGesture))
            {
                if (pinch->state() == Qt::GestureStarted)
                    touchZoom = 0.0;
                else if (pinch->state() == Qt::GestureUpdated)
                {
                    touchZoom += pinch->scaleFactor() - 1.0;
                    if (fabs(touchZoom) >= 0.1)
                    {
                        if (touchZoom > 0.00)
                            QMPlay2GUI.menuBar->player->zoom->zoomIn->trigger();
                        else if (touchZoom < 0.00)
                            QMPlay2GUI.menuBar->player->zoom->zoomOut->trigger();
                        touchZoom = 0.0;
                    }
                }
            }
            break;
        default:
            break;
    }
    return DockWidget::event(e);
}

void VideoDock::popup(const QPoint &p)
{
    if (canPopup)
    {
        popupMenu->popup(mapToGlobal(p));
    }
}
void VideoDock::hideCursor()
{
    if (QWidget *internalW = internalWidget())
    {
        bool ok;
        const int cursorShape = internalW->property("customCursor").toInt(&ok);
        if (!ok || cursorShape < 0 || cursorShape > Qt::LastCursor || internalW->cursor().shape() == cursorShape)
            internalW->setCursor(Qt::BlankCursor);
    }
    pixels = 0;
}
void VideoDock::updateImage(const QImage &img)
{
    iDW.setCustomPixmap(QPixmap::fromImage(img));
}
void VideoDock::visibilityChanged(bool v)
{
    emit QMPlay2Core.videoDockVisible(v);
}
void VideoDock::hasCoverImage(bool b)
{
    if (canHideIDWCursor != b)
    {
        canHideIDWCursor = b;
        if (canHideIDWCursor)
        {
            if (!hideCursorTim.isActive() && iDW.underMouse())
                hideCursorTim.start(g_hideCursorTimeout);
        }
        else
        {
            if (!iDW.getWidget())
                hideCursorTim.stop();
            iDW.unsetCursor();
        }
    }
}
