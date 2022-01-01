/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

    int FFT_NBITS, FFT_SIZE, FFT_SIZE_2;

    uchar chn;
    uint srate;
    bool canFilter, hasParameters, enabled;

    mutable QMutex mutex;
    FFTContext *fftIn, *fftOut;
    FFTComplex *complex;
    QVector<QVector<float>> input, last_samples;
    QVector<float> wind_f, f;
    float preamp;
};

#define EqualizerName "Audio Equalizer"
