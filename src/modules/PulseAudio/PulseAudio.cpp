#include <PulseAudio.hpp>
#include <PulseAudioWriter.hpp>

PulseAudio::PulseAudio() :
	Module("PulseAudio")
{
	moduleImg = QImage(":/PulseAudio");

	init("WriterEnabled", true);
	init("Delay", 0.1);
}

QList<PulseAudio::Info> PulseAudio::getModulesInfo(const bool showDisabled) const
{
	QList<Info> modulesInfo;
	if (showDisabled || getBool("WriterEnabled"))
		modulesInfo += Info(PulseAudioWriterName, WRITER, QStringList("audio"));
	return modulesInfo;
}
void *PulseAudio::createInstance(const QString &name)
{
	if (name == PulseAudioWriterName && getBool("WriterEnabled"))
		return new PulseAudioWriter(*this);
	return NULL;
}

PulseAudio::SettingsWidget *PulseAudio::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_PLUGIN(PulseAudio)

/**/

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	enabledB = new QCheckBox(tr("Enabled"));
	enabledB->setChecked(sets().getBool("WriterEnabled"));

	QLabel *delayL = new QLabel(tr("Delay") + ": ");

	delayB = new QDoubleSpinBox;
	delayB->setRange(0.01, 1.0);
	delayB->setSingleStep(0.01);
	delayB->setSuffix(" " + tr("sec"));
	delayB->setValue(sets().getDouble("Delay"));

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(enabledB, 0, 0, 1, 2);
	layout->addWidget(delayL, 1, 0, 1, 1);
	layout->addWidget(delayB, 1, 1, 1, 1);
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set("WriterEnabled", enabledB->isChecked());
	sets().set("Delay", delayB->value());
}
