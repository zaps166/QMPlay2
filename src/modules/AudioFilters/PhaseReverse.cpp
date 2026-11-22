// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <PhaseReverse.hpp>

PhaseReverse::PhaseReverse(Module &module) :
    hasParameters(false), canFilter(false)
{
    SetModule(module);
}

bool PhaseReverse::set()
{
    enabled = sets().getBool("PhaseReverse");
    reverseRight = sets().getBool("PhaseReverse/ReverseRight");
    canFilter = enabled && hasParameters;
    return true;
}

bool PhaseReverse::setAudioParameters(uchar chn, uint /*srate*/)
{
    hasParameters = chn >= 2;
    if (hasParameters)
        this->chn = chn;
    canFilter = enabled && hasParameters;
    return hasParameters;
}
double PhaseReverse::filter(QByteArray &data, bool)
{
    if (canFilter)
    {
        const int size = data.size() / sizeof(float);
        float *samples = (float *)data.data();

        for (int i = reverseRight; i < size; i += chn)
            samples[i] = -samples[i];
    }
    return 0.0;
}
