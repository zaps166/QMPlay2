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

#include <VideoDock.hpp>

#include <Settings.hpp>
#include <MenuBar.hpp>
#include <Main.hpp>

#include <Functions.hpp>
#include <SubsDec.hpp>

#include <QApplication>
#include <QMouseEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QPainter>
#include <QGesture>
#include <QMenu>

#include <math.h>

static const int hideCursorTimeout = 750;

VideoDock::VideoDock() :
	isTouch(false), touchEnded(false),
	iDW(QMPlay2Core.getQMPlay2Pixmap(), QMPlay2GUI.grad1, QMPlay2GUI.grad2, QMPlay2GUI.qmpTxt),
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
	foreach (QAction *act, QMPlay2GUI.menuBar->window->actions())
		addAction(act);
	foreach (QAction *act, QMPlay2GUI.menuBar->playlist->actions())
		addAction(act);
	foreach (QAction *act, QMPlay2GUI.menuBar->player->actions())
		addAction(act);
	foreach (QAction *act, QMPlay2GUI.menuBar->playback->actions())
		addAction(act);
	foreach (QAction *act, QMPlay2GUI.menuBar->options->actions())
		addAction(act);
	foreach (QAction *act, QMPlay2GUI.menuBar->help->actions())
		addAction(act);
	/**/

	setMouseTracking(true);
	setAcceptDrops(true);
	grabGesture(Qt::PinchGesture);
	setContextMenuPolicy(Qt::CustomContextMenu);

	setWidget(&iDW);

	hideCursorTim.setSingleShot(true);

	connect(&hideCursorTim, SIGNAL(timeout()), this, SLOT(hideCursor()));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popup(const QPoint &)));
	connect(&iDW, SIGNAL(resized(int, int)), this, SLOT(resizedIDW(int, int)));
	connect(&iDW, SIGNAL(hasCoverImage(bool)), this, SLOT(hasCoverImage(bool)));
	connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));
	connect(&QMPlay2Core, SIGNAL(dockVideo(QWidget *)), &iDW, SLOT(setWidget(QWidget *)));

	if ((isBreeze = QApplication::style()->objectName() == "breeze"))
		setStyle(&commonStyle);

	canHideIDWCursor = false;
}

void VideoDock::fullScreen(bool b)
{
	if (b)
	{
		is_floating = isFloating();

		setTitleBarVisible(false);
		setFeatures(DockWidget::NoDockWidgetFeatures);
		setFloating(false);

		if (!isBreeze)
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
				dockedW->setParent(NULL);
			}
			setWidget(&iDW);
		}

		setTitleBarVisible();
		setFeatures(DockWidget::AllDockWidgetFeatures);
		setFloating(is_floating);

		if (!isBreeze)
			setStyle(NULL);
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
		w->unsetCursor();
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
			const QStringList urls = Functions::getUrlsFromMimeData(mimeData);
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
		hideCursorTim.start(hideCursorTimeout);
	}
	if (e)
		DockWidget::mouseMoveEvent(e);
}
#ifndef Q_OS_MAC
void VideoDock::mouseDoubleClickEvent(QMouseEvent *e)
{
	if (e->buttons() == Qt::LeftButton)
		QMPlay2GUI.menuBar->window->toggleFullScreen->trigger();
	DockWidget::mouseDoubleClickEvent(e);
}
#endif
void VideoDock::mousePressEvent(QMouseEvent *e)
{
	if (e->button() == Qt::LeftButton)
		canPopup = false;
	if (e->buttons() == Qt::MiddleButton)
		QMPlay2GUI.menuBar->player->togglePlay->trigger();
	else if ((e->buttons() & (Qt::LeftButton | Qt::MiddleButton)) == (Qt::LeftButton | Qt::MiddleButton))
		QMPlay2GUI.menuBar->player->reset->trigger();
	else if ((e->buttons() & (Qt::LeftButton | Qt::RightButton)) == (Qt::LeftButton | Qt::RightButton))
		QMPlay2GUI.menuBar->player->switchARatio->trigger();
	DockWidget::mousePressEvent(e);
}
void VideoDock::mouseReleaseEvent(QMouseEvent *e)
{
	if (e->button() == Qt::LeftButton)
		canPopup = true;
	if (QWidget *internalW = internalWidget())
	{
		if (internalW->cursor().shape() != Qt::BlankCursor)
			hideCursorTim.start(hideCursorTimeout);
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
	if (e->orientation() == Qt::Vertical)
	{
		Settings &settings = QMPlay2Core.getSettings();
		MenuBar::Player *player = QMPlay2GUI.menuBar->player;
		if (e->buttons() & Qt::LeftButton)
			e->delta() > 0 ? player->zoomIn->trigger() : player->zoomOut->trigger();
		else if (e->buttons() == Qt::NoButton && settings.getBool("WheelAction"))
		{
			if (settings.getBool("WheelSeek"))
				e->delta() > 0 ? player->seekF->trigger() : player->seekB->trigger();
			else if (settings.getBool("WheelVolume"))
				e->delta() > 0 ? player->volUp->trigger() : player->volDown->trigger();
		}
	}
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
void VideoDock::enterEvent(QEvent *e)
{
	mouseMoveEvent(NULL);
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
							QMPlay2GUI.menuBar->player->zoomIn->trigger();
						else if (touchZoom < 0.00)
							QMPlay2GUI.menuBar->player->zoomOut->trigger();
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
		popupMenu->popup(mapToGlobal(p));
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
void VideoDock::resizedIDW(int w, int h)
{
	emit resized(w, h);
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
				hideCursorTim.start(hideCursorTimeout);
		}
		else
		{
			if (!iDW.getWidget())
				hideCursorTim.stop();
			iDW.unsetCursor();
		}
	}
}
