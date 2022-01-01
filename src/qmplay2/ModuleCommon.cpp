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

#include <ModuleCommon.hpp>

bool ModuleCommon::set()
{
    return true;
}

ModuleCommon::~ModuleCommon()
{
    if (module)
    {
        module->mutex.lock();
        module->instances.removeOne(this);
        module->mutex.unlock();
    }
}

void ModuleCommon::SetModule(Module &m)
{
    if (!module)
    {
        module = &m;
        module->mutex.lock();
        module->instances.append(this);
        module->mutex.unlock();
        set();
    }
}
