#include <PortAudio.hpp>
#include <PortAudioWriter.hpp>

PortAudio::PortAudio() :
	Module("PortAudio")
{
	moduleImg = QImage(":/PortAudio");

	Pa_Initialize();
	init("WriterEnabled", true);
#ifdef Q_OS_WIN
	init("Delay", 0.15);
#else
	init("Delay", 0.10);
#endif
	init("OutputDevice", QString());
}
PortAudio::~PortAudio()
{
	Pa_Terminate();
}

QList< PortAudio::Info > PortAudio::getModulesInfo(const bool showDisabled) const
{
	QList< Info > modulesInfo;
	if (showDisabled || getBool("WriterEnabled"))
		modulesInfo += Info(PortAudioWriterName, WRITER, QStringList("audio"));
	return modulesInfo;
}
void *PortAudio::createInstance(const QString &name)
{
	if (name == PortAudioWriterName && getBool("WriterEnabled"))
		return new PortAudioWriter(*this);
	return NULL;
}

PortAudio::SettingsWidget *PortAudio::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_PLUGIN(PortAudio)

/**/

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QPushButton>
#include <QComboBox>
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

	QLabel *devicesL = new QLabel(tr("Playback device") + ": ");

	devicesB = new QComboBox;
	devicesB->addItems(QStringList(tr("Default")) + PortAudioCommon::getOutputDeviceNames());
	int idx = devicesB->findText(sets().getString("OutputDevice"));
	devicesB->setCurrentIndex(idx < 0 ? 0 : idx);

	QPushButton *defaultDevs = new QPushButton;
	defaultDevs->setText(tr("Find default output device"));
	connect(defaultDevs, SIGNAL(clicked()), this, SLOT(defaultDevs()));

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(enabledB, 0, 0, 1, 2);
	layout->addWidget(devicesL, 1, 0, 1, 1);
	layout->addWidget(devicesB, 1, 1, 1, 1);
	layout->addWidget(defaultDevs, 2, 0, 1, 2);
	layout->addWidget(delayL, 3, 0, 1, 1);
	layout->addWidget(delayB, 3, 1, 1, 1);
}

void ModuleSettingsWidget::defaultDevs()
{
	devicesB->setCurrentIndex(PortAudioCommon::deviceIndexToOutputIndex(PortAudioCommon::getDefaultOutputDevice()) + 1);
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set("WriterEnabled", enabledB->isChecked());
	sets().set("OutputDevice", devicesB->currentIndex() == 0 ? QString() : devicesB->currentText());
	sets().set("Delay", delayB->value());
}
