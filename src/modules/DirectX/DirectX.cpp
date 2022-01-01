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

#include <QGridLayout>
#include <QCheckBox>

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
