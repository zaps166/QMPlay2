// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
