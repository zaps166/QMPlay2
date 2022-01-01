/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QLabel>

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
