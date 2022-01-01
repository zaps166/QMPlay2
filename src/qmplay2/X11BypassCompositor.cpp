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

#include <X11BypassCompositor.hpp>
#include <QMPlay2Core.hpp>

#include <QLibrary>
#include <QVariant>
#include <QWidget>

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
