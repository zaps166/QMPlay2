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

#include <ScreenSaver.hpp>

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS) && !defined(Q_OS_ANDROID)

#include <QGuiApplication>
#include <QLibrary>

class ScreenSaverPriv : public QObject
{
public:
    inline ScreenSaverPriv() :
        m_disp(nullptr)
    {}
    inline ~ScreenSaverPriv()
    {
        if (m_disp)
            XCloseDisplayFunc(m_disp);
    }

    inline void load()
    {
        QLibrary libX11("libX11.so.6");
        if (libX11.load())
        {
            XOpenDisplayFunc = (XOpenDisplayType)libX11.resolve("XOpenDisplay");
            XForceScreenSaverFunc = (XForceScreenSaverType)libX11.resolve("XForceScreenSaver");
            XFlushFunc = (XFlushType)libX11.resolve("XFlush");
            XCloseDisplayFunc = (XCloseDisplayType)libX11.resolve("XCloseDisplay");
            if (XOpenDisplayFunc && XForceScreenSaverFunc && XFlushFunc && XCloseDisplayFunc)
                m_disp = XOpenDisplayFunc(nullptr);
        }
    }
    inline bool isLoaded() const
    {
        return (bool)m_disp;
    }

    inline void inhibit()
    {
        timerEvent(nullptr);
        m_timerID = startTimer(30000);
    }
    inline void unInhibit()
    {
        killTimer(m_timerID);
    }

private:
    void timerEvent(QTimerEvent *) override final
    {
        XForceScreenSaverFunc(m_disp, 0);
        XFlushFunc(m_disp);
    }

    using XOpenDisplayType = void *(*)(const char *name);
    XOpenDisplayType XOpenDisplayFunc;

    using XForceScreenSaverType = int (*)(void *display, int mode);
    XForceScreenSaverType XForceScreenSaverFunc;

    using XFlushType = int (*)(void *display);
    XFlushType XFlushFunc;

    using XCloseDisplayType = int (*)(void *display);
    XCloseDisplayType XCloseDisplayFunc;

    void *m_disp;
    int m_timerID;
};

/**/

ScreenSaver::ScreenSaver() :
    m_priv(new ScreenSaverPriv)
{
    if (QGuiApplication::platformName() == "xcb")
        m_priv->load();
}
ScreenSaver::~ScreenSaver()
{
    delete m_priv;
}

void ScreenSaver::inhibit(int context)
{
    if (inhibitHelper(context) && m_priv->isLoaded())
        m_priv->inhibit();
}
void ScreenSaver::unInhibit(int context)
{
    if (unInhibitHelper(context) && m_priv->isLoaded())
        m_priv->unInhibit();
}

#else

ScreenSaver::ScreenSaver() :
    m_priv(nullptr)
{
    Q_UNUSED(m_priv)
}
ScreenSaver::~ScreenSaver()
{}

void ScreenSaver::inhibit(int)
{}
void ScreenSaver::unInhibit(int)
{}

#endif
