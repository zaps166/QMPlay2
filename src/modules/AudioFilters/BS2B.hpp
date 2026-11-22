// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <AudioFilter.hpp>

#include <bs2b/bs2b.hpp>

class BS2B final : public AudioFilter
{
public:
    BS2B(Module &module);
    ~BS2B();

    bool set() override;
private:
    bool setAudioParameters(uchar, uint srate) override;
    void clearBuffers() override;
    double filter(QByteArray &data, bool flush) override;

    void alloc();

    bool m_enabled, m_hasParameters, m_canFilter;
    qint32 m_fcut, m_feed;
    quint32 m_srate;
    t_bs2bd *m_bs2b;
};

#define BS2BName "Bauer stereophonic-to-binaural DSP"
