/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2023  Błażej Szczygieł

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <AudioFilter.hpp>

#include <vector>

struct FFTContext;
struct FFTComplex;

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

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    mutable QRecursiveMutex m_mutex;
#else
    mutable QMutex m_mutex;
#endif
    FFTContext *m_fftIn = nullptr;
    FFTContext *m_fftOut = nullptr;
    FFTComplex *m_complex = nullptr;
    std::vector<std::vector<float>> m_input, m_lastSamples;
    std::vector<float> m_windF, m_f;
    float m_preamp = 0.0f;
};

#define EqualizerName "Audio Equalizer"
