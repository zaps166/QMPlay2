// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <ALSA.hpp>
#include <ALSAWriter.hpp>

ALSA::ALSA() :
    Module("ALSA")
{
    m_icon = QIcon(":/ALSA.svgz");

    init("WriterEnabled", true);
    init("AutoFindMultichnDev", true);
    init("Delay", 0.1);
    init("OutputDevice", "default");
}

QList<ALSA::Info> ALSA::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
    if (showDisabled || getBool("WriterEnabled"))
        modulesInfo += Info(ALSAWriterName, WRITER, QStringList{"audio"});
    return modulesInfo;
}
void *ALSA::createInstance(const QString &name)
{
    if (name == ALSAWriterName && getBool("WriterEnabled"))
        return new ALSAWriter(*this);
    return nullptr;
}

ALSA::SettingsWidget *ALSA::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(ALSA)

/**/

#include <QDoubleSpinBox>
#include <QFormLayout>
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

    delayB = new QDoubleSpinBox;
    delayB->setRange(0.01, 1.0);
    delayB->setSingleStep(0.01);
    delayB->setSuffix(" " + tr("sec"));
    delayB->setValue(sets().getDouble("Delay"));

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

    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(enabledB);
    layout->addRow(tr("Maximum latency") + ": ", delayB);
    layout->addRow(tr("Playback device") + ": ", devicesB);
    layout->addRow(autoMultichnB);
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
