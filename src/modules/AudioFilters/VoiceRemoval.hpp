// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <AudioFiltersHeaders.hpp>

#include <AudioFilter.hpp>

class VoiceRemoval final : public AudioFilter
{
public:
    VoiceRemoval(Module &);

    bool set() override;
private:
    bool setAudioParameters(uchar, uint) override;
    double filter(QByteArray &, bool) override;

    bool enabled, hasParameters, canFilter;
    uchar chn;
};

#define VoiceRemovalName "Voice Removal"
