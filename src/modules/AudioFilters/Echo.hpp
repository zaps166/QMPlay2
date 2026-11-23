// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <AudioFiltersHeaders.hpp>

#include <AudioFilter.hpp>

class Echo final : public AudioFilter
{
public:
    Echo(Module &);

    bool set() override;
private:
    bool setAudioParameters(uchar, uint) override;
    double filter(QByteArray &, bool) override;

    void alloc(bool);

    bool enabled, hasParameters, canFilter;

    uint echo_delay, echo_volume, echo_repeat;
    bool echo_surround;

    uchar chn;
    uint srate;

    int w_ofs;
    QVector<float> sampleBuffer;
};

#define EchoName "Echo"
