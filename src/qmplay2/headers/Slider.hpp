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

#include <QSlider>

class QMPLAY2SHAREDLIB_EXPORT Slider final : public QSlider
{
	Q_OBJECT
public:
	Slider();
	~Slider() final = default;

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
	void paintEvent(QPaintEvent *) override;
	void mousePressEvent(QMouseEvent *) override;
	void mouseReleaseEvent(QMouseEvent *) override;
	void mouseMoveEvent(QMouseEvent *) override;
	void wheelEvent(QWheelEvent *) override;
	void enterEvent(QEvent *) override;
private:
	int getMousePos(const QPoint &pos);

	bool canSetValue, ignoreValueChanged;
	int lastMousePos, wheelStep, firstLine, secondLine;
	int cachedSliderValue;
signals:
	void mousePosition(int xPos);
};
