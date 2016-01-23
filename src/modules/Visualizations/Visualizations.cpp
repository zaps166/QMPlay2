#include <Visualizations.hpp>
#include <SimpleVis.hpp>
#include <FFTSpectrum.hpp>

Visualizations::Visualizations() :
	Module("Visualizations")
{
	init("RefreshTime", 22);
	init("SimpleVis/SoundLength", 22);
	init("FFTSpectrum/Size", 7);
	init("FFTSpectrum/Scale", 3);
}

QList< Visualizations::Info > Visualizations::getModulesInfo(const bool) const
{
	QList< Info > modulesInfo;
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
