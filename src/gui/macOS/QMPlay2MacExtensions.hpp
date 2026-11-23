// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <GUIHeaders.hpp>

namespace QMPlay2MacExtensions
{
    using MediaKeysCallback = std::function<void(const QString &param)>;

    void setApplicationVisible(bool visible);

    void registerMacOSMediaKeys(const MediaKeysCallback &fn);
    void unregisterMacOSMediaKeys();

    void showSystemUi(QWindow *mainWindow, bool visible);
}
