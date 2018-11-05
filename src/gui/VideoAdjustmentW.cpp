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

#include <VideoAdjustmentW.hpp>

#include <ModuleParams.hpp>
#include <Settings.hpp>
#include <Slider.hpp>

#include <QGridLayout>
#include <QPushButton>
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
constexpr const char *g_controlsNames[CONTROLS_COUNT] = {
	QT_TRANSLATE_NOOP("VideoAdjustmentW", "Brightness"),
	QT_TRANSLATE_NOOP("VideoAdjustmentW", "Contrast"),
	QT_TRANSLATE_NOOP("VideoAdjustmentW", "Saturation"),
	QT_TRANSLATE_NOOP("VideoAdjustmentW", "Hue"),
	QT_TRANSLATE_NOOP("VideoAdjustmentW", "Sharpness")
};

VideoAdjustmentW::VideoAdjustmentW()
{
	QGridLayout *layout = new QGridLayout;

	m_sliders.reserve(CONTROLS_COUNT);

	for (int i = 0; i < CONTROLS_COUNT; ++i)
	{
		QLabel *titleL = new QLabel(tr(g_controlsNames[i]) + ": ");
		titleL->setAlignment(Qt::AlignRight);

		QLabel *valueL = new QLabel("0");

		Slider *slider = new Slider;
		slider->setTickPosition(QSlider::TicksBelow);
		slider->setMinimumWidth(50);
		slider->setTickInterval(25);
		slider->setRange(-100, 100);
		slider->setWheelStep(1);
		slider->setValue(0);
		connect(slider, &Slider::valueChanged, this, [=](int v) {
			valueL->setText(QString::number(v));
			emit videoAdjustmentChanged();
		});
		m_sliders.push_back(slider);

		layout->addWidget(titleL, i, 0);
		layout->addWidget(slider, i, 1);
		layout->addWidget(valueL, i, 2);
	}

	QPushButton *resetB = new QPushButton(tr("Reset"));
	connect(resetB, &QPushButton::clicked, this, [this] {
		for (int i = 0; i < CONTROLS_COUNT; ++i)
			m_sliders[i]->setValue(0);
	});

	layout->addWidget(resetB, layout->rowCount(), 0, 1, 3);
	layout->addItem(new QSpacerItem(40, 0, QSizePolicy::Maximum, QSizePolicy::Minimum), layout->rowCount(), 2);

	setLayout(layout);
}
VideoAdjustmentW::~VideoAdjustmentW()
{}

void VideoAdjustmentW::restoreValues()
{
	for (int i = 0; i < CONTROLS_COUNT; ++i)
		m_sliders[i]->setValue(QMPlay2Core.getSettings().getInt(QString("VideoAdjustment/") + g_controlsNames[i]));
}
void VideoAdjustmentW::saveValues()
{
	for (int i = 0; i < CONTROLS_COUNT; ++i)
		QMPlay2Core.getSettings().set(QString("VideoAdjustment/") + g_controlsNames[i], m_sliders[i]->value());
}

void VideoAdjustmentW::setModuleParam(ModuleParams *writer)
{
	for (int i = 0; i < CONTROLS_COUNT; ++i)
	{
		m_sliders[i]->setEnabled(writer->hasParam(g_controlsNames[i]));
		writer->modParam(g_controlsNames[i], m_sliders[i]->value());
	}
}

void VideoAdjustmentW::enableControls()
{
	for (int i = 0; i < CONTROLS_COUNT; ++i)
		m_sliders[i]->setEnabled(true);
}
