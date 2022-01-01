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

#pragma once

#include <QMap>

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
