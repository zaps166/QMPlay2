// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <OpenMPTVisBase.hpp>

int OpenMPTVisBase::width() const
{
    return m_width;
}
int OpenMPTVisBase::height() const
{
    return m_height;
}

QFont OpenMPTVisBase::visFont(int pt)
{
    QFont font(QStringLiteral("Monospace"), pt);
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    return font;
}
