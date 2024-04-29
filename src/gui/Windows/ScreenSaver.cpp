/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

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

#include <QCoreApplication>
#include <QTimer>

#include <windows.h>

static inline bool inhibitScreenSaver(MSG *msg, const bool inhibited)
{
    return (inhibited && msg->message == WM_SYSCOMMAND && ((msg->wParam & 0xFFF0) == SC_SCREENSAVE || (msg->wParam & 0xFFF0) == SC_MONITORPOWER));
}

#include <QAbstractNativeEventFilter>

class ScreenSaverPriv : public QAbstractNativeEventFilter
{
public:
    bool inhibited = false;
    QTimer timer;

private:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEventFilter(const QByteArray &, void *m, qintptr *) override
#else
    bool nativeEventFilter(const QByteArray &, void *m, long *) override
#endif
    {
        return inhibitScreenSaver((MSG *)m, inhibited);
    }
};

/**/

ScreenSaver::ScreenSaver() :
    m_priv(new ScreenSaverPriv)
{
    qApp->installNativeEventFilter(m_priv);

    m_priv->timer.setInterval(30 * 1000);
    m_priv->timer.setSingleShot(true);
    QObject::connect(&m_priv->timer, &QTimer::timeout, [this] {
        m_priv->inhibited = false;
    });
}
ScreenSaver::~ScreenSaver()
{
    delete m_priv;
}

void ScreenSaver::inhibit(int context)
{
    if (inhibitHelper(context))
    {
        m_priv->timer.stop();
        m_priv->inhibited = true;
    }
}
void ScreenSaver::unInhibit(int context)
{
    if (unInhibitHelper(context))
    {
        if (Q_LIKELY(!m_priv->timer.isActive()))
            m_priv->timer.start();
    }
}
