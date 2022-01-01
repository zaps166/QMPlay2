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

#include <SubsDec.hpp>
#include <Module.hpp>

SubsDec *SubsDec::create(const QString &type)
{
    if (type.isEmpty())
        return nullptr;
    for (Module *module : QMPlay2Core.getPluginsInstance())
        for (const Module::Info &mod : module->getModulesInfo())
            if (mod.type == Module::SUBSDEC && mod.extensions.contains(type))
            {
                SubsDec *subsdec = (SubsDec *)module->createInstance(mod.name);
                if (!subsdec)
                    continue;
                return subsdec;
            }
    return nullptr;
}
QStringList SubsDec::extensions()
{
    QStringList extensions;
    for (const Module *module : QMPlay2Core.getPluginsInstance())
        for (const Module::Info &mod : module->getModulesInfo())
            if (mod.type == Module::SUBSDEC)
                extensions << mod.extensions;
    return extensions;
}
