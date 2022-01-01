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
