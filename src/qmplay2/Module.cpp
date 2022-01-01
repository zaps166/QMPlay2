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

#include <Module.hpp>
#include <ModuleCommon.hpp>

QList<QAction *> Module::getAddActions()
{
    return QList<QAction *>();
}

Module::SettingsWidget *Module::getSettingsWidget()
{
    return nullptr;
}

void Module::videoDeintSave()
{}

void Module::setInstances(bool &restartPlaying)
{
    QMutexLocker locker(&mutex);
    for (ModuleCommon *mc : qAsConst(instances))
        if (!mc->set())
            restartPlaying = true;
}
