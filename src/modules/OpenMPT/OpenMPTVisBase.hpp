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

#include <Frame.hpp>

#include <QFont>

namespace openmpt {
    class module;
}

class OpenMPTVisBase
{
public:
    enum class VisMode
    {
        Off = 0,
        Patterns = 1,
        Samples = 2
    };

    static constexpr int kMaxVisSize = 2048;
    static constexpr int kDefaultFontPt = 12;

public:
    virtual ~OpenMPTVisBase() = default;

    int width() const;
    int height() const;

    virtual Frame render(openmpt::module *module) = 0;
    virtual void reset() = 0;

    static QFont visFont(int pt);

protected:
    int m_width{0};
    int m_height{0};
};
