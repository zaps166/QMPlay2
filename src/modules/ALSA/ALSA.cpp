#include <ALSA.hpp>
#include <ALSAWriter.hpp>

ALSA::ALSA() :
	Module("ALSA")
{
	moduleImg = QImage(":/ALSA");

	init("WriterEnabled", true);
	init("AutoFindMultichnDev", true);
	init("Delay", 0.1);
	init("OutputDevice", "default");
}

QList< ALSA::Info > ALSA::getModulesInfo(const bool showDisabled) const
{
	QList< Info > modulesInfo;
	if (showDisabled || getBool("WriterEnabled"))
		modulesInfo += Info(ALSAWriterName, WRITER, QStringList("audio"));
	return modulesInfo;
}
void *ALSA::createInstance(const QString &name)
{
	if (name == ALSAWriterName && getBool("WriterEnabled"))
		return new ALSAWriter(*this);
	return NULL;
}

ALSA::SettingsWidget *ALSA::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_PLUGIN(ALSA)

/**/

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	const ALSACommon::DevicesList devicesList = ALSACommon::getDevices();
	const QString devName = ALSACommon::getDeviceName(devicesList, sets().getString("OutputDevice"));

	enabledB = new QCheckBox(tr("Włączony"));
	enabledB->setChecked(sets().getBool("WriterEnabled"));

	autoMultichnB = new QCheckBox(tr("Automatycznie znajdź urządzenie dla odtwarzania wielokanałowego"));
	autoMultichnB->setChecked(sets().getBool("AutoFindMultichnDev"));

	QLabel *delayL = new QLabel(tr("Opóźnienie") + ": ");

	delayB = new QDoubleSpinBox;
	delayB->setRange(0.01, 1.0);
	delayB->setSingleStep(0.01);
	delayB->setSuffix(" " + tr("sek"));
	delayB->setValue(sets().getDouble("Delay"));

	QLabel *devicesL = new QLabel(tr("Urządzenia odtwarzające") + ": ");

	devicesB = new QComboBox;
	for (int i = 0; i < devicesList.first.count(); ++i)
	{
		QString itemText = devicesList.second[i];
		if (itemText.isEmpty())
			itemText += devicesList.first[i];
		else
			itemText += " (" + devicesList.first[i] + ")";
		devicesB->addItem(itemText, devicesList.first[i]);
		if (devName == devicesList.first[i])
			devicesB->setCurrentIndex(i);
	}

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(enabledB, 0, 0, 1, 2);
	layout->addWidget(devicesL, 1, 0, 1, 1);
	layout->addWidget(devicesB, 1, 1, 1, 1);
	layout->addWidget(delayL, 2, 0, 1, 1);
	layout->addWidget(delayB, 2, 1, 1, 1);
	layout->addWidget(autoMultichnB, 3, 0, 1, 2);
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set("WriterEnabled", enabledB->isChecked());
	sets().set("AutoFindMultichnDev", autoMultichnB->isChecked());
	if (devicesB->currentIndex() > -1)
		sets().set("OutputDevice", devicesB->itemData(devicesB->currentIndex()).toString());
	else
		sets().set("OutputDevice", "default");
	sets().set("Delay", delayB->value());
}
