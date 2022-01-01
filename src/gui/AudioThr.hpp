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

#include <AVThread.hpp>

#include <SndResampler.hpp>

#include <QVector>

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

    bool resampler_create();

    inline uchar currentChannels() const;
    inline uint currentSampleRate() const;

    SndResampler sndResampler;
    uchar realChannels, channels;
    uint  realSample_rate, sample_rate;
    bool m_resamplerFirst;
    double lastSpeed;

    int tmp_br;
    double tmp_time, silence_step;
    volatile double doSilence;
    QMutex silenceChMutex;
    bool allowAudioDrain = false;

    QVector<QMPlay2Extensions *> visualizations;
    QVector<AudioFilter *> filters;
private slots:
    void pauseVis(bool);
signals:
    void pauseVisSig(bool);
};
