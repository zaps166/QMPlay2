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

#include <PortAudio.hpp>
#include <PortAudioWriter.hpp>

PortAudio::PortAudio() :
    Module("PortAudio")
{
    m_icon = QIcon(":/PortAudio.svgz");

    init("WriterEnabled", true);
#if defined(Q_OS_MACOS)
    init("Delay", 0.03);
    init("BitPerfect", false);
#elif defined(Q_OS_WIN)
    init("Delay", 0.10);
    init("Exclusive", false);
#else
    init("Delay", 0.10);
#endif
    init("OutputDevice", QString());
}
PortAudio::~PortAudio()
{
}

QList<PortAudio::Info> PortAudio::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
    if (showDisabled || getBool("WriterEnabled"))
        modulesInfo += Info(PortAudioWriterName, WRITER, QStringList("audio"));
    return modulesInfo;
}
void *PortAudio::createInstance(const QString &name)
{
    if (name == PortAudioWriterName && getBool("WriterEnabled"))
        return new PortAudioWriter(*this);
    return nullptr;
}

PortAudio::SettingsWidget *PortAudio::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(PortAudio)

/**/

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QPushButton>
#include <QComboBox>
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

    devicesB = new QComboBox;
    devicesB->addItems(QStringList(tr("Default")) + PortAudioCommon::getOutputDeviceNames());
    int idx = devicesB->findText(sets().getString("OutputDevice"));
    devicesB->setCurrentIndex(idx < 0 ? 0 : idx);

#ifdef Q_OS_WIN
    m_exclusive = new QCheckBox(tr("Exclusive mode"));
    m_exclusive->setChecked(sets().getBool("Exclusive"));
#endif

#ifdef Q_OS_MACOS
    bitPerfect = new QCheckBox(tr("Bit-perfect audio"));
    bitPerfect->setChecked(sets().getBool("BitPerfect"));
    bitPerfect->setToolTip(tr("This sets the selected output device to the sample rate of the content being played"));
#endif

    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(enabledB);
    layout->addRow(tr("Playback device") + ": ", devicesB);
    layout->addRow(tr("Maximum latency") + ": ", delayB);
#ifdef Q_OS_WIN
    layout->addRow(m_exclusive);
#endif
#ifdef Q_OS_MACOS
    layout->addRow(bitPerfect);
#endif
}

void ModuleSettingsWidget::saveSettings()
{
    sets().set("WriterEnabled", enabledB->isChecked());
    sets().set("OutputDevice", devicesB->currentIndex() == 0 ? QString() : devicesB->currentText());
    sets().set("Delay", delayB->value());
#ifdef Q_OS_WIN
    sets().set("Exclusive", m_exclusive->isChecked());
#endif
#ifdef Q_OS_MACOS
    sets().set("BitPerfect", bitPerfect->isChecked());
#endif
}
