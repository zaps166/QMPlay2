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

#include <QCoreApplication>

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

private:
    bool nativeEventFilter(const QByteArray &, void *m, long *) override
    {
        return inhibitScreenSaver((MSG *)m, inhibited);
    }
};

/**/

ScreenSaver::ScreenSaver() :
    m_priv(new ScreenSaverPriv)
{
    qApp->installNativeEventFilter(m_priv);
}
ScreenSaver::~ScreenSaver()
{
    delete m_priv;
}

void ScreenSaver::inhibit(int context)
{
    if (inhibitHelper(context))
        m_priv->inhibited = true;
}
void ScreenSaver::unInhibit(int context)
{
    if (unInhibitHelper(context))
        m_priv->inhibited = false;
}
