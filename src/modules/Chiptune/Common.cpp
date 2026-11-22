// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <Common.hpp>

namespace ChiptuneCommon {

void doFadeOut(float *data, int len, int chn, int srate, double fadePos, double fadeTime)
{
    const double silenceStep = (1.0 / fadeTime) / srate;
    double silence = 1.0 - (fadePos / fadeTime); //More or less OK
    for (int i = 0; i < len; i += chn)
    {
        for (int c = 0; c < chn; ++c)
            data[i + c] *= silence;
        silence -= silenceStep;
        if (silence < 0.0)
            silence = 0.0;
    }
}

}
