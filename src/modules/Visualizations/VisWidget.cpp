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

#include <VisWidget.hpp>
#include <QMPlay2Core.hpp>
#include <DockWidget.hpp>
#include <Functions.hpp>

#include <QMouseEvent>
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

VisWidget::VisWidget() :
	stopped(true),
	dw(new DockWidget)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	setFocusPolicy(Qt::StrongFocus);
	setAutoFillBackground(true);
	setMouseTracking(true);
	setPalette(Qt::black);

	connect(&tim, SIGNAL(timeout()), this, SLOT(update()));
	connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));
#if QT_VERSION >= 0x050000
	connect(&QMPlay2Core, SIGNAL(mainWidgetNotMinimized(bool)), this, SLOT(visibilityChanged(bool)));
#endif
	connect(&QMPlay2Core, SIGNAL(wallpaperChanged(bool, double)), this, SLOT(wallpaperChanged(bool, double)));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenu(const QPoint &)));
}

bool VisWidget::regionIsVisible() const
{
	return dw->visibleRegion() != QRegion() || visibleRegion() != QRegion();
}

void VisWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
	if (parent() == dw && (e->buttons() & Qt::LeftButton))
		emit doubleClicked();
	else
		QWidget::mouseDoubleClickEvent(e);
}
void VisWidget::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::ParentChange && !parent())
		dw->setWidget(this);
	QWidget::changeEvent(event);
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
#if QT_VERSION >= 0x050000
	const bool fromMainWindow = &QMPlay2Core == sender();
#else
	const bool fromMainWindow = false;
#endif
	if (!v && parent() == dw)
	{
		if (!fromMainWindow || !dw->isFloating()) //Don't stop on Qt5 when window is minimized and the dock is floating
			stop();
	}
	else if (!stopped)
	{
		//"fromMainWindow" ensures that visualization won't start on Qt5 when is not visible and main window is not minimized!
		start(v && (!fromMainWindow || regionIsVisible()), fromMainWindow);
	}
}
void VisWidget::showSettings()
{
	emit QMPlay2Core.showSettings("Visualizations");
}
