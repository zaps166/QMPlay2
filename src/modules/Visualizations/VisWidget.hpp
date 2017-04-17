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

#include <QWidget>
#include <QTimer>

#ifdef USE_OPENGL
	#include <QOpenGLWidget>
#endif

class DockWidget;

class VisWidget : public QWidget
{
	Q_OBJECT
protected:
	static void setValue(qreal &out, qreal in, qreal tDiffScaled);
	static void setValue(QPair<qreal, double> &out, qreal in, qreal tDiffScaled);

	VisWidget();

	bool regionIsVisible() const;

	virtual void paint(QPainter &p) = 0;

	virtual void start(bool v, bool dontCheckRegion) = 0;
	virtual void stop();

#ifdef USE_OPENGL
	void setUseOpenGL(bool b);
#endif

	void resizeEvent(QResizeEvent *e) override;

	QTimer tim;
	bool stopped;
	DockWidget *dw;
	double time;
private:
	void mouseDoubleClickEvent(QMouseEvent *) override final;
	void paintEvent(QPaintEvent *) override final;
	void changeEvent(QEvent *) override final;

	bool eventFilter(QObject *watched, QEvent *event) override final;

#ifdef USE_OPENGL
	QOpenGLWidget *glW;
#endif
private slots:
	void wallpaperChanged(bool hasWallpaper, double alpha);
	void contextMenu(const QPoint &point);
	void visibilityChanged(bool v);
	void updateVisualization();
	void showSettings();
signals:
	void doubleClicked();
};
