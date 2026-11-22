// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <AudioFilter.hpp>

class SwapStereo final : public AudioFilter
{
public:
    SwapStereo(Module &);

    bool set() override;
private:
    bool setAudioParameters(uchar, uint) override;
    double filter(QByteArray &, bool) override;

    bool m_enabled = false, m_hasParameters = false, m_canFilter = false;
    uchar m_chn = 0;
};

#define SwapStereoName "Swap Stereo"
