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

QList<ALSA::Info> ALSA::getModulesInfo(const bool showDisabled) const
{
	QList<Info> modulesInfo;
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

	enabledB = new QCheckBox(tr("Enabled"));
	enabledB->setChecked(sets().getBool("WriterEnabled"));

	autoMultichnB = new QCheckBox(tr("Automatic looking for multichannel device"));
	autoMultichnB->setChecked(sets().getBool("AutoFindMultichnDev"));

	QLabel *delayL = new QLabel(tr("Delay") + ": ");

	delayB = new QDoubleSpinBox;
	delayB->setRange(0.01, 1.0);
	delayB->setSingleStep(0.01);
	delayB->setSuffix(" " + tr("sec"));
	delayB->setValue(sets().getDouble("Delay"));

	QLabel *devicesL = new QLabel(tr("Playback device") + ": ");

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
