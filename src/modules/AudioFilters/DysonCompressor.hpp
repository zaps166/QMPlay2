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

#include <QMutex>

#define NFILT  12
#define NEFILT 17

class DysonCompressor final : public AudioFilter
{
public:
    DysonCompressor(Module &module);
    ~DysonCompressor();

    bool set() override;

private:
    bool setAudioParameters(uchar chn, uint srate) override;
    int bufferedSamples() const override;
    void clearBuffers() override;
    double filter(QByteArray &data, bool flush) override;

    using FloatVector = QVector<float>;

    QMutex mutex;
    bool enabled;

    int channels, sampleRate;
    int toRemove, delayedSamples;

    int ndelayptr; //ptr for the input
    double rlevelsq0, rlevelsq1;
    double rlevelsqn[NFILT];
    double rlevelsqe[NEFILT];
    QVector<FloatVector> samplesdelayed;
    /* Simple gain running average */
    double rgain;
    double lastrgain;
    /* Gainriding gain */
    double rmastergain0;
    /* Peak limit gain */
    double rpeakgain0, rpeakgain1;
    int peaklimitdelay, rpeaklimitdelay;

    int peakpercent;
    double releasetime;
    double fastgaincompressionratio, compressionratio;
};

#define DysonCompressorName "DysonCompressor"
