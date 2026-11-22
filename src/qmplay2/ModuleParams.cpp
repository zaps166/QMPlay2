// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <ModuleParams.hpp>

bool ModuleParams::modParam(const QString &key, const QVariant &val)
{
    auto it = paramList.find(key);
    if (it != paramList.end())
    {
        *it = val;
        return true;
    }
    return false;
}

bool ModuleParams::processParams(bool *paramsCorrected)
{
    Q_UNUSED(paramsCorrected)
    return true;
}
