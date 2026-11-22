// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <OpenSLES.hpp>
#include <OpenSLESWriter.hpp>

OpenSLES::OpenSLES() :
    Module("OpenSLES")
{
    m_icon = QIcon(":/OpenSLES.svgz");

    init("WriterEnabled", true);
}

QList<OpenSLES::Info> OpenSLES::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
    if (showDisabled || getBool("WriterEnabled"))
        modulesInfo += Info(OpenSLESWriterName, WRITER, QStringList{"audio"});
    return modulesInfo;
}
void *OpenSLES::createInstance(const QString &name)
{
    if (name == OpenSLESWriterName && getBool("WriterEnabled"))
        return new OpenSLESWriter(*this);
    return nullptr;
}

OpenSLES::SettingsWidget *OpenSLES::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(OpenSLES)

/**/

#include <QGridLayout>
#include <QCheckBox>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
    Module::SettingsWidget(module)
{
    openslesB = new QCheckBox(tr("Enabled"));
    openslesB->setChecked(sets().getBool("WriterEnabled"));

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(openslesB);
}

void ModuleSettingsWidget::saveSettings()
{
    sets().set("WriterEnabled", openslesB->isChecked());
}
