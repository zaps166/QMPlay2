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
/*
    Copyright (c) 1996, John S. Dyson
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    This code (easily) runs realtime on a P5-166 w/EDO, Triton-II on FreeBSD.

    More info/comments: dyson@freebsd.org

    This program provides compression of a stereo 16bit audio stream,
    such as that contained by a 16Bit wav file.  Extreme measures have
    been taken to make the compression as subtile as possible.  One
    possible purpose for this code would be to master cassette tapes from
    CD's for playback in automobiles where dynamic range needs to be
    restricted.

    Suitably recoded for an embedded DSP, this would make a killer audio
    compressor for broadcast or recording.  When writing this code, I
    ignored the issues of roundoff error or trucation -- Pentiums have
    really nice FP processors :-).
*/

#include <DysonCompressor.hpp>

#include <cmath>

#define MAXLEVEL 1.0 //default: 0.9

/* These filters should filter at least the lowest audio freq */
#define RLEVELSQ0FILTER 0.001
#define RLEVELSQ1FILTER 0.010
/* These are the attack time for the rms measurement */
#define RLEVELSQ0FFILTER 0.001
#define RLEVELSQEFILTER 0.001
/* delay for rlevelsq0ffilter delay */
#define NDELAY ((int)(1.0 / RLEVELSQ0FFILTER))
/* Slow compressor time constants */
#define RMASTERGAIN0FILTER 0.000003
#define RPEAKGAINFILTER 0.001
/* Max fast agc gain, slow agc gain */
/* maximum gain for fast compressor */
#define MAXFASTGAIN 3
/* maximum gain for slow compressor */
#define MAXSLOWGAIN 9
/* Level below which gain tracking shuts off */
#define FLOORLEVEL 0.001 //default: 0.06

DysonCompressor::DysonCompressor(Module &module) :
    enabled(false),
    channels(0),
    sampleRate(0)
{
    SetModule(module);
}
DysonCompressor::~DysonCompressor()
{}

bool DysonCompressor::set()
{
    QMutexLocker locker(&mutex);

    const bool newEnabled = sets().getBool("Compressor");

    peakpercent = sets().getInt("Compressor/PeakPercent");
    releasetime = sets().getDouble("Compressor/ReleaseTime");

    // Compression ratio for fast gain. This will determine how
    // much the audio is made more dense. 0.5 is equiv to 2:1
    // compression. 1.0 is equiv to inf:1 compression.
    fastgaincompressionratio = sets().getDouble("Compressor/FastGainCompressionRatio");

    // Overall ompression ratio.
    compressionratio = sets().getDouble("Compressor/OverallCompressionRatio");

    if (newEnabled != enabled)
    {
        enabled = newEnabled;
        clearBuffers();
    }

    return true;
}

bool DysonCompressor::setAudioParameters(uchar chn, uint srate)
{
    QMutexLocker locker(&mutex);
    channels = chn;
    sampleRate = srate;
    clearBuffers();
    return true;
}
int DysonCompressor::bufferedSamples() const
{
    return delayedSamples;
}
void DysonCompressor::clearBuffers()
{
    toRemove = NDELAY;
    delayedSamples = 0;
    ndelayptr = 0;

    rgain = rmastergain0 = 1.0;
    rlevelsq0 = rlevelsq1 = 0.0;

    rpeakgain0 = rpeakgain1 = 1.0;
    peaklimitdelay = rpeaklimitdelay = 0;
    lastrgain = 1.0;

    memset(rlevelsqn, 0, sizeof rlevelsqn);
    memset(rlevelsqe, 0, sizeof rlevelsqe);

    samplesdelayed.clear();
    if (enabled)
    {
        for (int i = 0; i < channels; ++i)
            samplesdelayed.append(FloatVector(NDELAY));
    }
}
double DysonCompressor::filter(QByteArray &data, bool flush)
{
    if (!enabled)
        return 0.0;

    QMutexLocker locker(&mutex);

    if (!flush)
    {
        const int size = data.size() / sizeof(float);
        float *samples = (float *)data.data();

        const double targetlevel = MAXLEVEL * peakpercent / 100.0;
        const double rgainfilter = 1.0 / (releasetime * sampleRate);

        for (int pos = 0; pos < size; pos += channels)
        {
            float *currentsamples = samples + pos;

            double levelsq0 = 0.0;
            for (int c = 0; c < channels; ++c)
            {
                samplesdelayed[c][ndelayptr] = currentsamples[c];
                levelsq0 += (double)currentsamples[c] * (double)currentsamples[c];
            }

            if (++ndelayptr >= NDELAY)
                ndelayptr = 0;

            if (levelsq0 > rlevelsq0)
                rlevelsq0 = (levelsq0 * RLEVELSQ0FFILTER) + rlevelsq0 * (1.0 - RLEVELSQ0FFILTER);
            else
                rlevelsq0 = (levelsq0 * RLEVELSQ0FILTER) + rlevelsq0 * (1.0 - RLEVELSQ0FILTER);

            //AGC
            if (rlevelsq0 > FLOORLEVEL * FLOORLEVEL)
            {
                if (rlevelsq0 > rlevelsq1)
                    rlevelsq1 = rlevelsq0;
                else
                    rlevelsq1 = rlevelsq0 * RLEVELSQ1FILTER + rlevelsq1 * (1.0 - RLEVELSQ1FILTER);

                rlevelsqn[0] = rlevelsq1;
                for (int i = 0; i < NFILT-1; i++)
                {
                    if (rlevelsqn[i] > rlevelsqn[i+1])
                        rlevelsqn[i+1] = rlevelsqn[i];
                    else
                        rlevelsqn[i+1] = rlevelsqn[i] * RLEVELSQ1FILTER + rlevelsqn[i+1] * (1.0 - RLEVELSQ1FILTER);
                }

                double efilt = RLEVELSQEFILTER;
                double levelsqe = rlevelsqe[0] = rlevelsqn[NFILT-1];
                for (int i = 0; i < NEFILT-1; ++i)
                {
                    rlevelsqe[i+1] = rlevelsqe[i] * efilt +    rlevelsqe[i+1] * (1.0 - efilt);
                    if (rlevelsqe[i+1] > levelsqe)
                        levelsqe = rlevelsqe[i+1];
                    efilt *= 1.0 / 1.5;
                }

                double gain = targetlevel / sqrt(levelsqe);
                if (compressionratio < 0.99)
                {
                    if (compressionratio == 0.50)
                        gain = sqrt(gain);
                    else
                        gain = exp(log(gain) * compressionratio);
                }

                if (gain < rgain)
                    rgain = gain * RLEVELSQEFILTER / 2.0 + rgain * (1.0 - RLEVELSQEFILTER / 2.0);
                else
                    rgain = gain * rgainfilter + rgain * (1.0 - rgainfilter);

                lastrgain = rgain;
                if (gain < lastrgain)
                    lastrgain = gain;
            }

            float sampled[channels];
            for (int c = 0; c < channels; ++c)
                sampled[c] = samplesdelayed.at(c).at(ndelayptr);

            double fastgain = lastrgain;
            if (fastgain > MAXFASTGAIN)
                fastgain = MAXFASTGAIN;
            if (fastgain < 0.0001)
                fastgain = 0.0001;

            double qgain;
            if (fastgaincompressionratio == 0.25)
                qgain = sqrt(sqrt(fastgain));
            else if (fastgaincompressionratio == 0.5)
                qgain = sqrt(fastgain);
            else if (fastgaincompressionratio == 1.0)
                qgain = fastgain;
            else
                qgain = exp(log(fastgain) * fastgaincompressionratio);

            double tslowgain = lastrgain / qgain;
            if (tslowgain > MAXSLOWGAIN)
                tslowgain = MAXSLOWGAIN;
            if (tslowgain < rmastergain0)
                rmastergain0 = tslowgain;
            else
                rmastergain0 = tslowgain * RMASTERGAIN0FILTER + (1.0 - RMASTERGAIN0FILTER) * rmastergain0;

            const double npeakgain = rmastergain0 * qgain;

            float ngain = MAXLEVEL;

            double newsample[channels];
            for (int c = 0; c < channels; ++c)
            {
                newsample[c] = sampled[c] * npeakgain;

                double tmpgain = 1.0;
                if (fabs(newsample[c]) >= MAXLEVEL)
                    tmpgain = MAXLEVEL / fabs(newsample[c]);

                if (tmpgain < ngain)
                    ngain = tmpgain;
            }

            const double ngsq = ngain * ngain;
            if (ngsq <= rpeakgain0)
            {
                rpeakgain0      = ngsq /* * 0.50 + rpeakgain0 * 0.50 */;
                rpeaklimitdelay = peaklimitdelay;
            }
            else if (rpeaklimitdelay == 0)
            {
                const double tnrgain = (ngain < 1.0) ? ngain : 1.0;
                rpeakgain0 = tnrgain * RPEAKGAINFILTER + (1.0 - RPEAKGAINFILTER) * rpeakgain0;
            }

            if (rpeakgain0 <= rpeakgain1)
            {
                rpeakgain1 = rpeakgain0;
                rpeaklimitdelay = peaklimitdelay;
            }
            else if (rpeaklimitdelay == 0)
                rpeakgain1 = RPEAKGAINFILTER * rpeakgain0 + (1.0 - RPEAKGAINFILTER) * rpeakgain1;
            else
                --rpeaklimitdelay;

            const double sqrtrpeakgain = sqrt(rpeakgain1);
            for (int c = 0; c < channels; ++c)
                currentsamples[c] = newsample[c] * sqrtrpeakgain;
        }

        if (toRemove > 0)
        {
            const int realToRemove = qMin(size / channels, toRemove);
            data.remove(0, realToRemove * channels * sizeof(float));
            delayedSamples += realToRemove;
            toRemove -= realToRemove;
        }
    }
    else
    {
        data.resize(channels * sizeof(float) * delayedSamples);
        float *samples = (float *)data.data();
        for (int pos = 0; pos < delayedSamples; ++pos)
        {
            for (int c = 0; c < channels; ++c)
                samples[pos * channels + c] = samplesdelayed.at(c).at(ndelayptr);
            if (++ndelayptr >= NDELAY)
                ndelayptr = 0;
        }
        delayedSamples = 0;
    }

    return NDELAY / (double)sampleRate;
}
