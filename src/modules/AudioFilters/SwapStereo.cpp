// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <SwapStereo.hpp>

SwapStereo::SwapStereo(Module &module)
{
    SetModule(module);
}

bool SwapStereo::set()
{
    m_enabled = sets().getBool("SwapStereo");
    m_canFilter = m_enabled && m_hasParameters;
    return true;
}

bool SwapStereo::setAudioParameters(uchar chn, uint srate)
{
    Q_UNUSED(srate)
    m_hasParameters = (chn >= 2);
    if (m_hasParameters)
        m_chn = chn;
    m_canFilter = (m_enabled && m_hasParameters);
    return m_hasParameters;
}
double SwapStereo::filter(QByteArray &data, bool)
{
    if (m_canFilter)
    {
        const int size = data.size() / sizeof(float);
        float *samples = (float *)data.data();

        for (int i = 0; i < size; i += m_chn)
            qSwap(samples[i + 0], samples[i + 1]);
    }
    return 0.0;
}
