// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Lib.hpp>

#include <QByteArray>

namespace Version
{
    QMPLAY2SHAREDLIB_EXPORT QByteArray get();
    QMPLAY2SHAREDLIB_EXPORT QByteArray userAgent();
    QMPLAY2SHAREDLIB_EXPORT QByteArray userAgentWithMozilla();
    QMPLAY2SHAREDLIB_EXPORT bool isPortable();
}
