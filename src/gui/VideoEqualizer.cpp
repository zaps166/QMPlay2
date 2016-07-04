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

#include <VideoEqualizer.hpp>

#include <QMPlay2Core.hpp>
#include <Settings.hpp>
#include <Slider.hpp>

#include <QGridLayout>
#include <QPushButton>
#include <QVariant>
#include <QLabel>

VideoEqualizer::VideoEqualizer()
{
	layout = new QGridLayout;

	int i;
	for (i = BRIGHTNESS; i < CONTROLS_COUNT; ++i)
	{
		QLabel *&titleL = controls[i].titleL;
		QLabel *&valueL = controls[i].valueL;
		Slider *&slider = controls[i].slider;

		switch (i)
		{
			case BRIGHTNESS:
				titleL = new QLabel(tr("Brightness") + ": ");
				break;
			case SATURATION:
				titleL = new QLabel(tr("Saturation") + ": ");
				break;
			case CONTRAST:
				titleL = new QLabel(tr("Contrast") + ": ");
				break;
			case HUE:
				titleL = new QLabel(tr("Hue") + ": ");
				break;
		}

		valueL = new QLabel("0");

		slider = new Slider;
		slider->setProperty("valueL", qVariantFromValue((void *)valueL));
		slider->setTickPosition(QSlider::TicksBelow);
		slider->setMinimumWidth(50);
		slider->setTickInterval(25);
		slider->setMinimum(-100);
		slider->setMaximum(100);
		slider->setValue(0);

		layout->addWidget(titleL, i, 0);
		layout->addWidget(slider, i, 1);
		layout->addWidget(valueL, i, 2);

		connect(slider, SIGNAL(valueChanged(int)), this, SLOT(setValue(int)));
	}

	resetB = new QPushButton(tr("Reset"));
	connect(resetB, SIGNAL(clicked()), this, SLOT(reset()));

	layout->addWidget(resetB, i++, 0, 1, 3);
	layout->addItem(new QSpacerItem(40, 0, QSizePolicy::Maximum, QSizePolicy::Minimum), i, 2);

	setLayout(layout);
}

void VideoEqualizer::restoreValues()
{
	controls[BRIGHTNESS].slider->setValue(QMPlay2Core.getSettings().getInt("VideoEqualizer/Brightness"));
	controls[SATURATION].slider->setValue(QMPlay2Core.getSettings().getInt("VideoEqualizer/Saturation"));
	controls[CONTRAST].slider->setValue(QMPlay2Core.getSettings().getInt("VideoEqualizer/Contrast"));
	controls[HUE].slider->setValue(QMPlay2Core.getSettings().getInt("VideoEqualizer/Hue"));
}
void VideoEqualizer::saveValues()
{
	QMPlay2Core.getSettings().set("VideoEqualizer/Brightness", controls[BRIGHTNESS].slider->value());
	QMPlay2Core.getSettings().set("VideoEqualizer/Saturation", controls[SATURATION].slider->value());
	QMPlay2Core.getSettings().set("VideoEqualizer/Contrast", controls[CONTRAST].slider->value());
	QMPlay2Core.getSettings().set("VideoEqualizer/Hue", controls[HUE].slider->value());
}

void VideoEqualizer::setValue(int v)
{
	((QLabel *)sender()->property("valueL").value<void *>())->setText(QString::number(v));
	emit valuesChanged
	(
		controls[BRIGHTNESS].slider->value(),
		controls[SATURATION].slider->value(),
		controls[CONTRAST].slider->value(),
		controls[HUE].slider->value()
	);
}
void VideoEqualizer::reset()
{
	for (int i = BRIGHTNESS; i < CONTROLS_COUNT; ++i)
		controls[i].slider->setValue(0);
}
