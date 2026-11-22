// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Lib.hpp>

#include <QMetaObject>

class QMPLAY2SHAREDLIB_EXPORT X11BypassCompositor
{
    Q_DISABLE_COPY(X11BypassCompositor)

public:
    X11BypassCompositor();
    ~X11BypassCompositor();

    void setX11BypassCompositor(bool bypassCompositor);

private:
    bool m_bypassCompositor = false;
    bool m_compositorBypassed = false;
    bool m_isFullScreen = false;
    QMetaObject::Connection m_fullScreenChangedConn;
};
