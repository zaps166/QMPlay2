// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <AudioFiltersHeaders.hpp>

#include <AudioFilter.hpp>
#include <FFT.hpp>

class Equalizer final : public AudioFilter
{
public:
    static QVector<float> interpolate(const QVector<float> &, const int);
    static QVector<float> freqs(Settings &);
    static float getAmpl(int val);

    Equalizer(Module &);
    ~Equalizer();

    bool set() override;
private:
    bool setAudioParameters(uchar, uint) override;
    int bufferedSamples() const override;
    void clearBuffers() override;
    double filter(QByteArray &data, bool flush) override;

    /**/

    void alloc(bool);
    void interpolateFilterCurve();

private:
    int m_fftNBits = 0;
    int m_fftSize = 0;

    uchar m_chn = 0;
    uint m_srate = 0;

    bool m_canFilter = false;
    bool m_hasParameters = false;
    bool m_enabled = false;

    mutable QRecursiveMutex m_mutex;
    FFT m_fftIn;
    FFT m_fftOut;
    FFT::Complex *m_complex = nullptr;
    std::vector<std::vector<float>> m_input, m_lastSamples;
    std::vector<float> m_windF, m_f;
    float m_preamp = 0.0f;
};

#define EqualizerName "Audio Equalizer"
