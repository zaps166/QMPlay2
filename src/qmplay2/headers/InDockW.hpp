/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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

#include <QMPlay2Lib.hpp>

#include <QPointer>
#include <QWidget>

class QMPLAY2SHAREDLIB_EXPORT InDockW final : public QWidget
{
	Q_OBJECT
public:
	InDockW(const QColor &, const QColor &, const QColor &);

	QWidget *getWidget();
	void setLoseHeight(int lh);
	void setCustomPixmap(const QPixmap &pix = QPixmap());
private:
	const QColor &grad1, &grad2, &qmpTxt;
	Qt::TransformationMode blurredTransformation;
	QPixmap customPixmap, customPixmapBlurred;
	bool hasWallpaper;
	int loseHeight;
	QPointer<QWidget> w;
private slots:
	void wallpaperChanged(bool hasWallpaper, double alpha);
	void setWidget(QWidget *newW);
protected:
	void resizeEvent(QResizeEvent *) override;
	void paintEvent(QPaintEvent *) override;
	bool event(QEvent *) override;
signals:
	void resized(int, int);
	void itemDropped(const QString &);
	void hasCoverImage(bool);
};
