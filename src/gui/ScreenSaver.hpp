// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <GUIHeaders.hpp>

class ScreenSaverPriv;

class ScreenSaver
{
    Q_DISABLE_COPY(ScreenSaver)
public:
    ScreenSaver();
    ~ScreenSaver();

    void inhibit(int context);
    void unInhibit(int context);

private:
    inline bool inhibitHelper(int context);
    inline bool unInhibitHelper(int context);

    ScreenSaverPriv *m_priv;

    QMap<int, bool> m_refs;
};

/* Inline implementation */

inline bool ScreenSaver::inhibitHelper(int context)
{
    if (!m_refs.value(context))
    {
        m_refs[context] = true;
        for (auto it = m_refs.constBegin(), itEnd = m_refs.constEnd(); it != itEnd; ++it)
        {
            if (it.value() && it.key() != context)
                return false;
        }
        return true;
    }
    return false;
}
inline bool ScreenSaver::unInhibitHelper(int context)
{
    if (m_refs.value(context))
    {
        m_refs[context] = false;
        for (auto it = m_refs.constBegin(), itEnd = m_refs.constEnd(); it != itEnd; ++it)
        {
            if (it.value())
                return false;
        }
        return true;
    }
    return false;
}
