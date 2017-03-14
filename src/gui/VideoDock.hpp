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

#include <QCommonStyle>
#include <QTimer>

#include <DockWidget.hpp>
#include <InDockW.hpp>

class QMenu;

class VideoDock : public DockWidget
{
	Q_OBJECT
public:
	VideoDock();

	void fullScreen(bool);
	inline void setLoseHeight(int lh)
	{
		iDW.setLoseHeight(lh);
	}

	inline void updateIDW()
	{
		iDW.update();
	}

	bool isTouch, touchEnded;
private:
	QWidget *internalWidget();

	void unsetCursor(QWidget *w);

	void dragEnterEvent(QDragEnterEvent *) override final;
	void dropEvent(QDropEvent *) override final;
	void mouseMoveEvent(QMouseEvent *) override final;
	void mouseDoubleClickEvent(QMouseEvent *) override final;
	void mousePressEvent(QMouseEvent *) override final;
	void mouseReleaseEvent(QMouseEvent *) override final;
	void moveEvent(QMoveEvent *) override final;
	void wheelEvent(QWheelEvent *) override final;
	void leaveEvent(QEvent *) override final;
	void enterEvent(QEvent *) override final;
	bool event(QEvent *) override final;

	QTimer hideCursorTim, leftButtonPlayTim;
	InDockW iDW;
	QMenu *popupMenu;
	QCommonStyle commonStyle;
	int pixels;
	bool canPopup, is_floating, isBreeze, canHideIDWCursor, doubleClicked;
	double touchZoom;
private slots:
	void popup(const QPoint &);
	void hideCursor();
	void resizedIDW(int, int);
	void updateImage(const QImage &);
	void visibilityChanged(bool);
	void hasCoverImage(bool);
signals:
	void resized(int, int);
	void itemDropped(const QString &, bool);
};
