// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
