// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <PulseAudio.hpp>
#include <PulseAudioWriter.hpp>

PulseAudio::PulseAudio() :
    Module("PulseAudio")
{
    m_icon = QIcon(":/PulseAudio.svgz");

    init("WriterEnabled", true);
    init("Delay", 0.1);
}

QList<PulseAudio::Info> PulseAudio::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
    if (showDisabled || getBool("WriterEnabled"))
        modulesInfo += Info(PulseAudioWriterName, WRITER, QStringList{"audio"});
    return modulesInfo;
}
void *PulseAudio::createInstance(const QString &name)
{
    if (name == PulseAudioWriterName && getBool("WriterEnabled"))
        return new PulseAudioWriter(*this);
    return nullptr;
}

PulseAudio::SettingsWidget *PulseAudio::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(PulseAudio)

/**/

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
    Module::SettingsWidget(module)
{
    enabledB = new QCheckBox(tr("Enabled"));
    enabledB->setChecked(sets().getBool("WriterEnabled"));

    delayB = new QDoubleSpinBox;
    delayB->setRange(0.01, 1.0);
    delayB->setSingleStep(0.01);
    delayB->setSuffix(" " + tr("sec"));
    delayB->setValue(sets().getDouble("Delay"));

    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(enabledB);
    layout->addRow(tr("Maximum latency") + ": ", delayB);
}

void ModuleSettingsWidget::saveSettings()
{
    sets().set("WriterEnabled", enabledB->isChecked());
    sets().set("Delay", delayB->value());
}
