// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <OpenMPTVisBase.hpp>

#include <QStaticText>

class OpenMPTVisPatterns : public OpenMPTVisBase
{
public:
    OpenMPTVisPatterns(openmpt::module *module, int channels, int rows);

    Frame render(openmpt::module *module) override;
    void reset() override;

private:
    int m_channels{0};
    int m_userRows{0};
    int m_rows{0};
    int m_rowHeight{0};
    int m_maxCellWidth{0};
    int m_charWidth{0};
    QFont m_font;
    QStaticText m_charCache[127];
    int m_lastPattern{-1};
    int m_lastRow{-1};
};
