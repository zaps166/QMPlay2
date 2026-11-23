// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
