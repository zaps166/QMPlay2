// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
    for (ModuleCommon *mc : std::as_const(instances))
        if (!mc->set())
            restartPlaying = true;
}
