// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <ScreenSaver.hpp>

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
