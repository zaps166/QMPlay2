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

#include <PipeWire.hpp>
#include <PipeWireWriter.hpp>

PipeWire::PipeWire() :
    Module("PipeWire")
{
    m_icon = QIcon(":/PipeWire.svgz");

    init("WriterEnabled", true);

    pw_init(nullptr, nullptr);
}
PipeWire::~PipeWire()
{
    pw_deinit();
}

QList<PipeWire::Info> PipeWire::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
    if (showDisabled || getBool("WriterEnabled"))
        modulesInfo += Info(PipeWireWriterName, WRITER, QStringList{"audio"});
    return modulesInfo;
}
void *PipeWire::createInstance(const QString &name)
{
    if (name == PipeWireWriterName && getBool("WriterEnabled"))
        return new PipeWireWriter(*this);
    return nullptr;
}

PipeWire::SettingsWidget *PipeWire::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(PipeWire)

/**/

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
    Module::SettingsWidget(module)
{
    m_enabledCheckBox = new QCheckBox(tr("Enabled"));
    m_enabledCheckBox->setChecked(sets().getBool("WriterEnabled"));

    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(m_enabledCheckBox);
}
ModuleSettingsWidget::~ModuleSettingsWidget()
{}

void ModuleSettingsWidget::saveSettings()
{
    sets().set("WriterEnabled", m_enabledCheckBox->isChecked());
}
