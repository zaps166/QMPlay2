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

#ifndef VISWIDGET_HPP
#define VISWIDGET_HPP

#ifdef USE_OPENGL
	#include <QOpenGLWidget>
	#define BaseWidget QOpenGLWidget
#else
	#include <QWidget>
	#define BaseWidget QWidget
#endif
#include <QTimer>

class DockWidget;

class VisWidget : public BaseWidget
{
	Q_OBJECT
protected:
	static void setValue(qreal &out, qreal in, qreal tDiffScaled);
	static void setValue(QPair<qreal, double> &out, qreal in, qreal tDiffScaled);

	VisWidget();

	bool regionIsVisible() const;

	virtual void start(bool v, bool dontCheckRegion) = 0;
	virtual void stop() = 0;

	QTimer tim;
	bool stopped;
	DockWidget *dw;
	double time;
private:
	void mouseDoubleClickEvent(QMouseEvent *) override;
	void changeEvent(QEvent *) override;
private slots:
	void wallpaperChanged(bool hasWallpaper, double alpha);
	void contextMenu(const QPoint &point);
	void visibilityChanged(bool v);
	void showSettings();
signals:
	void doubleClicked();
};

#endif
