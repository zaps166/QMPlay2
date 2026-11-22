// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Lib.hpp>

#include <string.h>

class QMPLAY2SHAREDLIB_EXPORT VideoAdjustment
{
public:
    inline void unset()
    {
        brightness = contrast = saturation = hue = sharpness = gamma = (qint16)0x8000;
        negative = false;
    }

    inline void zero()
    {
        memset(this, 0, sizeof *this);
    }

    inline bool operator !=(const VideoAdjustment &other) const
    {
        return memcmp(this, &other, sizeof *this);
    }

    qint16 brightness, contrast, gamma, saturation, hue, sharpness;
    bool negative;
};
