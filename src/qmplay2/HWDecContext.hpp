// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Core.hpp>

class Frame;

class HWDecContext
{
public:
    virtual ~HWDecContext() = default;

    virtual QString name() const = 0;

    inline bool hasError() const
    {
        return m_error;
    }

protected:
    std::atomic_bool m_error {false};
};
