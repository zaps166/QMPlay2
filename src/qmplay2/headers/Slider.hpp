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

#ifndef SLIDER_HPP
#define SLIDER_HPP

#include <QSlider>

class Slider : public QSlider
{
	Q_OBJECT
public:
	Slider();

	inline bool ignoringValueChanged() const
	{
		return ignoreValueChanged;
	}
public slots:
	void setValue(int val);
	inline void setMaximum(int max)
	{
		QAbstractSlider::setMaximum(max);
	}
	inline void setWheelStep(int ws)
	{
		wheelStep = ws;
	}
	void drawRange(int first, int second);
protected:
	void paintEvent(QPaintEvent *);
	void mousePressEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void wheelEvent(QWheelEvent *);
	void enterEvent(QEvent *);
private:
	int getMousePos(int X);

	bool canSetValue, ignoreValueChanged;
	int lastMousePos, wheelStep, firstLine, secondLine;
	int cachedSliderValue;
signals:
	void mousePosition(int xPos);
};

#endif
