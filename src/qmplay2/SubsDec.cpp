// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
