// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <X11BypassCompositor.hpp>
#include <QMPlay2Core.hpp>

X11BypassCompositor::X11BypassCompositor()
{
}
X11BypassCompositor::~X11BypassCompositor()
{
    if (m_fullScreenChangedConn)
    {
        setX11BypassCompositor(false);
        QObject::disconnect(m_fullScreenChangedConn);
    }
}

void X11BypassCompositor::setX11BypassCompositor(bool bypassCompositor)
{
    if (!m_fullScreenChangedConn)
    {
        m_fullScreenChangedConn = QObject::connect(&QMPlay2Core, &QMPlay2CoreClass::fullScreenChanged, [this](bool fullScreen) {
            m_isFullScreen = fullScreen;
            setX11BypassCompositor(m_bypassCompositor);
        });
        m_isFullScreen = QMPlay2Core.getMainWindow()->property("fullScreen").toBool();
    }

    m_bypassCompositor = bypassCompositor;

    const bool compositorBypassed = (m_isFullScreen && m_bypassCompositor);
    if (m_compositorBypassed == compositorBypassed)
        return;

    using XOpenDisplayType = void *(*)(const char *name);
    using XInternAtomType = unsigned long (*)(void *display, const char *atomName, int onlyIfExists);
    using XChangePropertyType = int *(*)(void *display, unsigned long window, unsigned long atom, unsigned long type, int format, int mode, const uint8_t *data, int nElements);
    using XCloseDisplayType = int (*)(void *display);

    QLibrary libX11("libX11.so.6");
    if (!libX11.load())
        return;

    auto XOpenDisplayFunc = (XOpenDisplayType)libX11.resolve("XOpenDisplay");
    auto XInternAtomFunc = (XInternAtomType)libX11.resolve("XInternAtom");
    auto XChangePropertyFunc = (XChangePropertyType)libX11.resolve("XChangeProperty");
    auto XCloseDisplayFunc = (XCloseDisplayType)libX11.resolve("XCloseDisplay");
    if (!XOpenDisplayFunc || !XInternAtomFunc || !XChangePropertyFunc || !XCloseDisplayFunc)
        return;

    auto disp = XOpenDisplayFunc(nullptr);
    if (!disp)
        return;

    if (auto atom = XInternAtomFunc(disp, "_NET_WM_BYPASS_COMPOSITOR", true))
    {
        m_compositorBypassed = compositorBypassed;

        const int value = m_compositorBypassed ? 1 : 0;
        XChangePropertyFunc(
            disp,
            QMPlay2Core.getMainWindow()->internalWinId(),
            atom,
            6 /* XA_CARDINAL */,
            32,
            0 /* PropModeReplace */,
            (const uint8_t *)&value,
            1
        );
    }

    XCloseDisplayFunc(disp);
}
