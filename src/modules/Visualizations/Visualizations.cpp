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

#include <Visualizations.hpp>
#include <SimpleVis.hpp>
#include <FFTSpectrum.hpp>

Visualizations::Visualizations() :
	Module("Visualizations")
{
#ifdef USE_OPENGL
	const int ms = 10; //Should rely on VSync
#else
	const int ms = 22;
#endif
	init("RefreshTime", ms);
	init("SimpleVis/SoundLength", ms);
	init("FFTSpectrum/Size", 7);
	init("FFTSpectrum/Scale", 3);
}

QList<Visualizations::Info> Visualizations::getModulesInfo(const bool) const
{
	QList<Info> modulesInfo;
	modulesInfo += Info(SimpleVisName, QMPLAY2EXTENSION);
	modulesInfo += Info(FFTSpectrumName, QMPLAY2EXTENSION);
	return modulesInfo;
}
void *Visualizations::createInstance(const QString &name)
{
	if (name == SimpleVisName)
		return new SimpleVis(*this);
	else if (name == FFTSpectrumName)
		return new FFTSpectrum(*this);
	return NULL;
}

Visualizations::SettingsWidget *Visualizations::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_PLUGIN(Visualizations)

/**/

#include <QGridLayout>
#include <QSpinBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	QLabel *refTimeL = new QLabel(tr("Refresh time") + ": ");

	refTimeB = new QSpinBox;
	refTimeB->setRange(1, 500);
	refTimeB->setSuffix(" " + tr("ms"));
	refTimeB->setValue(sets().getInt("RefreshTime"));

	QLabel *sndLenL = new QLabel(tr("Displayed sound length") + ": ");

	sndLenB = new QSpinBox;
	sndLenB->setRange(1, 500);
	sndLenB->setSuffix(" " + tr("ms"));
	sndLenB->setValue(sets().getInt("SimpleVis/SoundLength"));

	QLabel *fftSizeL = new QLabel(tr("FFT spectrum size") + ": ");

	fftSizeB = new QSpinBox;
	fftSizeB->setRange(5, 10);
	fftSizeB->setPrefix("2^");
	fftSizeB->setValue(sets().getInt("FFTSpectrum/Size"));

	QLabel *fftScaleL = new QLabel(tr("FFT spectrum scale") + ": ");

	fftScaleB = new QSpinBox;
	fftScaleB->setRange(1, 20);
	fftScaleB->setValue(sets().getInt("FFTSpectrum/Scale"));

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(refTimeL, 0, 0);
	layout->addWidget(refTimeB, 0, 1);
	layout->addWidget(sndLenL, 1, 0);
	layout->addWidget(sndLenB, 1, 1);
	layout->addWidget(fftSizeL, 2, 0);
	layout->addWidget(fftSizeB, 2, 1);
	layout->addWidget(fftScaleL, 3, 0);
	layout->addWidget(fftScaleB, 3, 1);

	connect(refTimeB, SIGNAL(valueChanged(int)), sndLenB, SLOT(setValue(int)));
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set("RefreshTime", refTimeB->value());
	sets().set("SimpleVis/SoundLength", sndLenB->value());
	sets().set("FFTSpectrum/Size", fftSizeB->value());
	sets().set("FFTSpectrum/Scale", fftScaleB->value());
}
