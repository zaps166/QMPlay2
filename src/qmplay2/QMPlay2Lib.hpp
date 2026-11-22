// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QtGlobal>

#if defined(QMPLAY2SHAREDLIB_LIBRARY)
    #define QMPLAY2SHAREDLIB_EXPORT Q_DECL_EXPORT
#else
    #define QMPLAY2SHAREDLIB_EXPORT Q_DECL_IMPORT
#endif
