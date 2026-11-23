// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <GUIHeaders.hpp>

#include <AVThread.hpp>

#include <SndResampler.hpp>

class QMPlay2Extensions;
class PlayClass;
class AudioFilter;

class AudioThr final : public AVThread
{
    Q_OBJECT
public:
    AudioThr(PlayClass &, const QStringList &pluginsName = {});
    ~AudioThr();

    void stop(bool terminate = false) override;
    void clearVisualizations();

    bool setParams(uchar realChn, uint realSRate, uchar chn, uint sRate, bool resamplerFirst);

    void silence(bool invert, bool fromPause);

    inline void setAllowAudioDrain()
    {
        allowAudioDrain = true;
    }
private:
    void run() override;

    bool createResampler(bool cleanBuffers);

    inline uchar currentChannels() const;
    inline uint currentSampleRate() const;

    SndResampler sndResampler;
    uchar realChannels, channels;
    uint  realSample_rate, sample_rate;
    bool m_resamplerFirst;
    bool m_lastKeepAudioPitch = false;
    double m_lastSpeed;

    int tmp_br;
    double tmp_time, silence_step;
    double doSilence;
    QMutex silenceChMutex;
    bool allowAudioDrain = false;

    QVector<QMPlay2Extensions *> visualizations;
    QVector<AudioFilter *> filters;
private slots:
    void pauseVis(bool);
signals:
    void pauseVisSig(bool);
};
