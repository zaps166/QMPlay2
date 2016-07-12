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

#include <VideoAdjustment.hpp>

#include <ModuleParams.hpp>
#include <QMPlay2Core.hpp>
#include <Settings.hpp>
#include <Slider.hpp>

#include <QGridLayout>
#include <QPushButton>
#include <QVariant>
#include <QLabel>

enum CONTROLS
{
	BRIGHTNESS,
	CONTRAST,
	SATURATION,
	HUE,
	SHARPNESS,

	CONTROLS_COUNT
};
static const char *ControlsNames[CONTROLS_COUNT] = {
	QT_TRANSLATE_NOOP("VideoAdjustment", "Brightness"),
	QT_TRANSLATE_NOOP("VideoAdjustment", "Contrast"),
	QT_TRANSLATE_NOOP("VideoAdjustment", "Saturation"),
	QT_TRANSLATE_NOOP("VideoAdjustment", "Hue"),
	QT_TRANSLATE_NOOP("VideoAdjustment", "Sharpness")
};

VideoAdjustment::VideoAdjustment() :
	layout(new QGridLayout),
	controls(new Controls[CONTROLS_COUNT])
{
	int i;
	for (i = 0; i < CONTROLS_COUNT; ++i)
	{
		QLabel *&titleL = controls[i].titleL;
		QLabel *&valueL = controls[i].valueL;
		Slider *&slider = controls[i].slider;

		titleL = new QLabel(tr(ControlsNames[i]) + ": ");
		titleL->setAlignment(Qt::AlignRight);

		valueL = new QLabel("0");

		slider = new Slider;
		slider->setProperty("valueL", qVariantFromValue((void *)valueL));
		slider->setTickPosition(QSlider::TicksBelow);
		slider->setMinimumWidth(50);
		slider->setTickInterval(25);
		slider->setRange(-100, 100);
		slider->setWheelStep(1);
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
VideoAdjustment::~VideoAdjustment()
{
	delete[] controls;
}

void VideoAdjustment::restoreValues()
{
	for (int i = 0; i < CONTROLS_COUNT; ++i)
		controls[i].slider->setValue(QMPlay2Core.getSettings().getInt(QString("VideoAdjustment/") + ControlsNames[i]));
}
void VideoAdjustment::saveValues()
{
	for (int i = 0; i < CONTROLS_COUNT; ++i)
		QMPlay2Core.getSettings().set(QString("VideoAdjustment/") + ControlsNames[i], controls[i].slider->value());
}

void VideoAdjustment::setModuleParam(ModuleParams *writer)
{
	for (int i = 0; i < CONTROLS_COUNT; ++i)
	{
		controls[i].slider->setEnabled(writer->hasParam(ControlsNames[i]));
		writer->modParam(ControlsNames[i], controls[i].slider->value());
	}
}

void VideoAdjustment::enableControls()
{
	for (int i = 0; i < CONTROLS_COUNT; ++i)
		controls[i].slider->setEnabled(true);
}

void VideoAdjustment::setValue(int v)
{
	((QLabel *)sender()->property("valueL").value<void *>())->setText(QString::number(v));
	emit videoAdjustmentChanged();
}
void VideoAdjustment::reset()
{
	for (int i = 0; i < CONTROLS_COUNT; ++i)
		controls[i].slider->setValue(0);
}
