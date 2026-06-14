/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2026  Błażej Szczygieł

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
