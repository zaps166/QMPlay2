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

#include <windows.h>

class ScreenSaverPriv
{
public:
    HANDLE handle;
};

/**/

ScreenSaver::ScreenSaver() :
    m_priv(new ScreenSaverPriv)
{
    REASON_CONTEXT reason = {};
    reason.Version = POWER_REQUEST_CONTEXT_VERSION;
    reason.Flags = POWER_REQUEST_CONTEXT_SIMPLE_STRING;
    reason.Reason.SimpleReasonString = const_cast<LPWSTR>(L"Playback");
    m_priv->handle = PowerCreateRequest(&reason);
}
ScreenSaver::~ScreenSaver()
{
    if (m_priv->handle != INVALID_HANDLE_VALUE)
        CloseHandle(m_priv->handle);
    delete m_priv;
}

void ScreenSaver::inhibit(int context)
{
    if (m_priv->handle != INVALID_HANDLE_VALUE && inhibitHelper(context))
    {
        PowerSetRequest(m_priv->handle, PowerRequestDisplayRequired);
        PowerSetRequest(m_priv->handle, PowerRequestSystemRequired);
    }
}
void ScreenSaver::unInhibit(int context)
{
    if (m_priv->handle != INVALID_HANDLE_VALUE && unInhibitHelper(context))
    {
        PowerClearRequest(m_priv->handle, PowerRequestDisplayRequired);
        PowerClearRequest(m_priv->handle, PowerRequestSystemRequired);
    }
}
