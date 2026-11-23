// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <DirectX.hpp>
#include <DirectDraw.hpp>

DirectX::DirectX() :
    Module("DirectX")
{
    m_icon = QIcon(":/DirectX.svgz");

    init("DirectDrawEnabled", true);
}

QList<DirectX::Info> DirectX::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
    if (showDisabled || getBool("DirectDrawEnabled"))
        modulesInfo += Info(DirectDrawWriterName, WRITER, QStringList("video"));
    return modulesInfo;
}
void *DirectX::createInstance(const QString &name)
{
    if (name == DirectDrawWriterName && getBool("DirectDrawEnabled"))
        return new DirectDrawWriter(*this);
    return NULL;
}

DirectX::SettingsWidget *DirectX::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(DirectX)

/**/

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
    Module::SettingsWidget(module)
{
    ddrawB = new QCheckBox(tr("Enabled"));
    ddrawB->setChecked(sets().getBool("DirectDrawEnabled"));

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(ddrawB);
}

void ModuleSettingsWidget::saveSettings()
{
    sets().set("DirectDrawEnabled", ddrawB->isChecked());
}
