// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QPainter.hpp>
#include <QPainterWriter.hpp>

QPainterSW::QPainterSW() :
    Module("QPainterSW")
{
    m_icon = QIcon(":/QPainter.svgz");

    init("Enabled", true);
}

QList<QPainterSW::Info> QPainterSW::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
    if (showDisabled || getBool("Enabled"))
        modulesInfo += Info(QPainterWriterName, WRITER, QStringList{"video"});
    return modulesInfo;
}
void *QPainterSW::createInstance(const QString &name)
{
    if (name == QPainterWriterName && getBool("Enabled"))
        return new QPainterWriter(*this);
    return nullptr;
}

QPainterSW::SettingsWidget *QPainterSW::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(QPainterSW)

/**/

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
    Module::SettingsWidget(module)
{
    enabledB = new QCheckBox(tr("Enabled"));
    enabledB->setChecked(sets().getBool("Enabled"));

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(enabledB);
}

void ModuleSettingsWidget::saveSettings()
{
    sets().set("Enabled", enabledB->isChecked());
}
