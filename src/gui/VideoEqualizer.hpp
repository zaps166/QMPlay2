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

#ifndef VIDEOEQUALIZER_HPP
#define VIDEOEQUALIZER_HPP

#include <QWidget>

class QGridLayout;
class QPushButton;
class QLabel;
class Slider;

class VideoEqualizer : public QWidget
{
	Q_OBJECT
public:
	VideoEqualizer();

	void restoreValues();
	void saveValues();
signals:
	void valuesChanged(int b, int s, int c, int h);
private slots:
	void setValue(int);
	void reset();
private:
	QGridLayout *layout;
	enum CONTROLS {BRIGHTNESS, SATURATION, CONTRAST, HUE, CONTROLS_COUNT};
	struct
	{
		QLabel *titleL, *valueL;
		Slider *slider;
	} controls[CONTROLS_COUNT];
	QPushButton *resetB;
};

#endif //VIDEOEQUALIZER_HPP
