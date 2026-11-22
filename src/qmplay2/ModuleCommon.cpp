// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
