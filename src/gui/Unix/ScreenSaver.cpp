/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2025  Błażej Szczygieł

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

#include <QGuiApplication>
#include <QLibrary>

#ifdef DBUS_PM
# include <QDBusConnection>
# include <QDBusInterface>
#endif

class ScreenSaverPriv : public QObject
{
public:
    virtual bool load() = 0;

    virtual void inhibit() = 0;
    virtual void unInhibit() = 0;
};

class ScreenSaverPrivX11 final : public ScreenSaverPriv
{
public:
    ScreenSaverPrivX11() :
        m_disp(nullptr)
    {}
    ~ScreenSaverPrivX11() override
    {
        if (m_disp)
            XCloseDisplayFunc(m_disp);
    }

    bool load() override
    {
        QLibrary libX11("libX11.so.6");
        if (libX11.load())
        {
            XOpenDisplayFunc = (XOpenDisplayType)libX11.resolve("XOpenDisplay");
            XForceScreenSaverFunc = (XForceScreenSaverType)libX11.resolve("XForceScreenSaver");
            XFlushFunc = (XFlushType)libX11.resolve("XFlush");
            XCloseDisplayFunc = (XCloseDisplayType)libX11.resolve("XCloseDisplay");
            if (XOpenDisplayFunc && XForceScreenSaverFunc && XFlushFunc && XCloseDisplayFunc)
            {
                m_disp = XOpenDisplayFunc(nullptr);
                if (m_disp)
                    return true;
            }
        }
        return false;
    }

    void inhibit() override
    {
        timerEvent(nullptr);
        m_timerID = startTimer(30000);
    }
    void unInhibit() override
    {
        killTimer(m_timerID);
    }

private:
    void timerEvent(QTimerEvent *) override
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

    void *m_disp = nullptr;
    int m_timerID = 0;
};

#ifdef DBUS_PM
class ScreenSaverPrivDBus final : public ScreenSaverPriv
{
public:
    ScreenSaverPrivDBus()
        : m_iface("org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver", "org.freedesktop.ScreenSaver")
    {}
    ~ScreenSaverPrivDBus() override
    {
    }

    bool load() override
    {
        return !m_iface.lastError().isValid();
    }

    void inhibit() override
    {
        bool ok = false;
        quint32 cookie = m_iface.call("Inhibit", QCoreApplication::applicationName(), "Playback").arguments().value(0).toUInt(&ok);
        if (ok)
            m_cookie = cookie;
    }
    void unInhibit() override
    {
        if (Q_UNLIKELY(m_cookie == 0))
            return;

        m_iface.call("UnInhibit", m_cookie);
        m_cookie = 0;
    }

private:
    QDBusInterface m_iface;
    quint32 m_cookie = 0;
};
#endif

/**/

ScreenSaver::ScreenSaver()
    : m_priv(nullptr)
{
#ifdef DBUS_PM
    m_priv = new ScreenSaverPrivDBus;
    if (m_priv->load())
        return;

    delete m_priv;
    m_priv = nullptr;
#endif

    if (QGuiApplication::platformName() != "xcb")
        return;

    m_priv = new ScreenSaverPrivX11;
    if (m_priv->load())
        return;

    delete m_priv;
    m_priv = nullptr;
}
ScreenSaver::~ScreenSaver()
{
    delete m_priv;
}

void ScreenSaver::inhibit(int context)
{
    if (m_priv && inhibitHelper(context))
        m_priv->inhibit();
}
void ScreenSaver::unInhibit(int context)
{
    if (m_priv && unInhibitHelper(context))
        m_priv->unInhibit();
}
