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

#include <AudioThr.hpp>

#include <Main.hpp>
#include <Writer.hpp>
#include <Decoder.hpp>
#include <Settings.hpp>
#include <Functions.hpp>
#include <PlayClass.hpp>
#include <AudioFilter.hpp>
#include <ScreenSaver.hpp>
#include <QMPlay2Extensions.hpp>

#include <QCoreApplication>

#include <cmath>

AudioThr::AudioThr(PlayClass &playC, const QStringList &pluginsName) :
    AVThread(playC)
{
    writer = Writer::create("audio:", pluginsName);

    for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
        if (QMPlay2Ext->isVisualization())
            visualizations += QMPlay2Ext;
    visualizations.squeeze();

    filters = AudioFilter::open();

    connect(this, SIGNAL(pauseVisSig(bool)), this, SLOT(pauseVis(bool)));

    if (QMPlay2GUI.mainW->property("fullScreen").toBool())
        QMPlay2GUI.screenSaver->inhibit(1);

    maybeStartThread();
}
AudioThr::~AudioThr()
{
    QMPlay2GUI.screenSaver->unInhibit(1);
}

void AudioThr::stop(bool terminate)
{
    for (QMPlay2Extensions *vis : qAsConst(visualizations))
        vis->visState(false);
    for (AudioFilter *filter : qAsConst(filters))
        delete filter;
    playC.audioSeekPos = -1;
    AVThread::stop(terminate);
}
void AudioThr::clearVisualizations()
{
    for (QMPlay2Extensions *vis : qAsConst(visualizations))
        vis->clearSoundData();
}

bool AudioThr::setParams(uchar realChn, uint realSRate, uchar chn, uint sRate, bool resamplerFirst)
{
    m_resamplerFirst = resamplerFirst;

    doSilence = -1.0;
    lastSpeed = playC.speed;

    realChannels    = realChn;
    realSample_rate = realSRate;

    writer->modParam("chn",  (channels = chn ? chn : realChannels));
    writer->modParam("rate", (sample_rate = sRate ? sRate : realSRate));

    bool paramsCorrected = false;
    if (writer->processParams((sRate && chn) ? nullptr : &paramsCorrected)) //nie pozwala na korektę jeżeli są wymuszone parametry
    {
        if (paramsCorrected)
        {
            const uchar lastChn = channels;
            const uint lastSRate = sample_rate;
            channels = writer->getParam("chn").toUInt();
            sample_rate = writer->getParam("rate").toUInt();
            if ((!chn || channels == lastChn) && (!sRate || sample_rate == lastSRate))
                QMPlay2Core.logInfo(tr("Module") + " \"" + writer->name() + "\" " + tr("sets the parameters to") + ": " + QString::number(channels) + " " + tr("channels") + ", " + QString::number(sample_rate) + " " + tr("Hz"));
            else
            {
                QMPlay2Core.logError(tr("Module") + " \"" + writer->name() + "\" " + tr("requires a change in one of the forced parameters, sound disabled ..."));
                return false;
            }
        }

        if (!resampler_create())
        {
            if (paramsCorrected)
                return false;
            channels    = realChannels;
            sample_rate = realSample_rate;
        }

        for (QMPlay2Extensions *vis : qAsConst(visualizations))
            vis->visState(true, currentChannels(), currentSampleRate());
        for (AudioFilter *filter : qAsConst(filters))
            filter->setAudioParameters(currentChannels(), currentSampleRate());

        return true;
    }

    return false;
}

void AudioThr::silence(bool invert, bool fromPause)
{
    if (QMPlay2Core.getSettings().getBool("Silence") && (!fromPause || playC.frame_last_pts <= 0.0) && doSilence == -1.0 && isRunning() && (invert || !playC.paused))
    {
        playC.doSilenceBreak = false;
        allowAudioDrain |= !invert;
        silence_step = (invert ? -4.0 : 4.0) / sample_rate;
        silenceChMutex.lock();
        doSilence = invert ? -silence_step : (1.0 - silence_step);
        silenceChMutex.unlock();
        while (!invert && isRunning())
        {
            silenceChMutex.lock();
            const double lastDoSilence = doSilence;
            silenceChMutex.unlock();
            if (lastDoSilence <= 0.0 || lastDoSilence >= 1.0)
                break;
            for (int i = 0; i < 100; ++i)
            {
                QCoreApplication::processEvents();
                if (playC.doSilenceBreak)
                {
                    playC.doSilenceBreak = false;
                    break;
                }
                Functions::s_wait(0.01);
                silenceChMutex.lock();
                if (doSilence <= 0.0)
                {
                    silenceChMutex.unlock();
                    break;
                }
                silenceChMutex.unlock();
            }
            silenceChMutex.lock();
            if (lastDoSilence == doSilence)
            {
                doSilence = -1.0;
                silenceChMutex.unlock();
                break;
            }
            silenceChMutex.unlock();
        }
    }
}

void AudioThr::run()
{
    setPriority(QThread::HighestPriority);

    QMutex emptyBufferMutex;
    bool paused = false;
    bool oneFrame = false;
    tmp_br = tmp_time = 0;
    while (!br)
    {
        double delay = 0.0, audio_pts = 0.0; //"audio_pts" odporny na zerowanie przy przewijaniu
        Decoder *last_dec = dec;
        while (!br && dec == last_dec)
        {
            playC.aPackets.lock();
            const bool hasAPackets = playC.aPackets.canFetch();
            bool hasBufferedSamples = false;
            if (playC.endOfStream && !hasAPackets)
                for (AudioFilter *filter : qAsConst(filters))
                    if (filter->bufferedSamples())
                    {
                        hasBufferedSamples = true;
                        break;
                    }

            if ((playC.paused && !oneFrame) || (!hasAPackets && !hasBufferedSamples) || playC.waitForData || (playC.audioSeekPos <= 0.0 && playC.videoSeekPos > 0.0))
            {
                tmp_br = tmp_time = 0;
                if (playC.paused && !paused)
                {
                    doSilence = -1.0;
                    writer->pause();
                    paused = true;
                    emit pauseVisSig(paused);
                }
                playC.aPackets.unlock();

                if (!playC.paused)
                    waiting = playC.fillBufferB = true;

                emptyBufferMutex.lock();
                playC.emptyBufferCond.wait(&emptyBufferMutex, MUTEXWAIT_TIMEOUT);
                emptyBufferMutex.unlock();
                continue;
            }
            if (paused)
            {
                paused = false;
                emit pauseVisSig(paused);
            }
            waiting = false;

            const bool flushAudio = playC.flushAudio;
            double ts = qQNaN();

            Packet packet;
            if (!hasBufferedSamples && (dec->pendingFrames() == 0 || flushAudio))
                packet = playC.aPackets.fetch();
            else if (hasBufferedSamples)
                ts = audio_pts + playC.audio_last_delay + delay; //szacowanie czasu
            playC.aPackets.unlock();

            if (playC.nextFrameB && playC.seekTo < 0.0 && playC.audioSeekPos <= 0.0 && playC.frame_last_pts <= 0.0)
            {
                playC.nextFrameB = false;
                oneFrame = playC.paused = true;
            }

            playC.fillBufferB = true;

            mutex.lock();
            if (br)
            {
                mutex.unlock();
                break;
            }

            QByteArray decoded;
            if (!hasBufferedSamples)
            {
                quint8 newChannels = 0;
                quint32 newSampleRate = 0;
                const int bytesConsumed = dec->decodeAudio(packet, decoded, ts, newChannels, newSampleRate, flushAudio);
                tmp_br += bytesConsumed;
                if (newChannels && newSampleRate && (newChannels != realChannels || newSampleRate != realSample_rate))
                {
                    //Audio parameters has been changed
                    updateMutex.lock();
                    mutex.unlock();
                    emit playC.audioParamsUpdate(newChannels, newSampleRate);
                    updateMutex.lock(); //Wait for "audioParamsUpdate()" to be finished
                    mutex.lock();
                    updateMutex.unlock();
                }
            }

            if (tmp_time >= 1000.0)
            {
                emit playC.updateBitrateAndFPS(round((tmp_br << 3) / tmp_time), -1);
                tmp_br = tmp_time = 0;
            }

            if (m_resamplerFirst && sndResampler.isOpen())
            {
                QByteArray converted;
                sndResampler.convert(decoded, converted);
                decoded = std::move(converted);
            }

            delay = writer->getParam("delay").toDouble();
            for (AudioFilter *filter : qAsConst(filters))
            {
                if (flushAudio)
                    filter->clearBuffers();
                delay += filter->filter(decoded, hasBufferedSamples);
            }

            if (flushAudio)
                playC.flushAudio = false;
            int decodedSize = decoded.size();
            int decodedPos = 0;
            while (decodedSize > 0 && (!playC.paused || oneFrame) && !br && !br2)
            {
                const double max_len = 0.02; //TODO: zrobić opcje?
                const int chunk = qMin(decodedSize, (int)(ceil(currentSampleRate() * max_len) * currentChannels() * sizeof(float)));
                float vol[2] = {0.0f, 0.0f};
                if (!playC.muted)
                {
                    const auto getVolume = [this](double vol) {
                        return playC.replayGain * (qFuzzyCompare(vol, 1.0) ? 1.0 : vol * vol);
                    };
                    if (currentChannels() == 1)
                    {
                        const double monoVol = (playC.vol[0] + playC.vol[1]) / 2.0;
                        if (monoVol > 0.0)
                            vol[0] = vol[1] = getVolume(monoVol);
                    }
                    else for (int c = 0; c < 2; ++c)
                    {
                        if (playC.vol[c] > 0.0)
                            vol[c] = getVolume(playC.vol[c]);
                    }
                }

                const bool isMuted = qFuzzyIsNull(vol[0]) && qFuzzyIsNull(vol[1]);

                QByteArray decodedChunk;
                if (isMuted)
                    decodedChunk.fill(0, chunk);
                else
                    decodedChunk = QByteArray::fromRawData((const char *)decoded.constData() + decodedPos, chunk);

                decodedPos += chunk;
                decodedSize -= chunk;

                playC.audio_last_delay = (double)decodedChunk.size() / (double)(sizeof(float) * currentChannels() * currentSampleRate());
                if (!qIsNaN(ts))
                {
                    audio_pts = playC.audio_current_pts = ts - delay;
                    if (!playC.vThr && playC.audioSeekPos <= 0)
                    {
                        playC.chPos(playC.audio_current_pts);
                    }
                }

                if (playC.audioSeekPos > 0.0)
                {
                    bool cont = true;
                    if (audio_pts >= playC.audioSeekPos)
                    {
                        tmp_br = 0;
                        playC.audioSeekPos = -1.0;
                        playC.emptyBufferCond.wakeAll();
                        if (playC.videoSeekPos <= 0.0)
                            cont = false; // Don't play if video is not ready
                    }
                    if (cont)
                        break;
                }

                tmp_time += playC.audio_last_delay * 1000.0;
                if (!qIsNaN(ts))
                    ts += playC.audio_last_delay;

                if (playC.skipAudioFrame <= 0.0 || oneFrame)
                {
                    const double speed = playC.speed;
                    if (speed != lastSpeed)
                    {
                        resampler_create();
                        lastSpeed = speed;
                    }

                    if (!isMuted && (!qFuzzyCompare(vol[0], 1.0f) || !qFuzzyCompare(vol[1], 1.0f)))
                    {
                        const int size = decodedChunk.size() / sizeof(float);
                        float *data = (float *)decodedChunk.data();
                        for (int i = 0; i < size; ++i)
                            data[i] *= vol[i & 1];
                    }

                    for (QMPlay2Extensions *vis : qAsConst(visualizations))
                        vis->sendSoundData(decodedChunk);

                    QByteArray dataToWrite;
                    if (!m_resamplerFirst && sndResampler.isOpen())
                        sndResampler.convert(decodedChunk, dataToWrite);
                    else
                        dataToWrite = decodedChunk;

                    if (doSilence >= 0.0)
                    {
                        silenceChMutex.lock();
                        if (isMuted)
                            doSilence = -1.0;
                        if (doSilence >= 0.0)
                        {
                            float *data = (float *)dataToWrite.data();
                            const int s = dataToWrite.size() / sizeof(float);
                            for (int i = 0; i < s; i += channels)
                            {
                                for (int j = 0; j < channels; ++j)
                                    data[i+j] *= doSilence;
                                doSilence -= silence_step;
                                if (doSilence < 0.0)
                                    doSilence = 0.0;
                                else if (doSilence > 1.0)
                                {
                                    doSilence = -1.0;
                                    break;
                                }
                            }
                        }
                        silenceChMutex.unlock();
                    }

                    oneFrame = false;

                    do
                    {
                        const int ret = writer->write(dataToWrite);
                        if (ret >= 0 || !writer->readyWrite())
                            break;
                    } while (!br && !br2);
                }
                else
                {
                    playC.skipAudioFrame -= playC.audio_last_delay;
                }
            }

            mutex.unlock();
        }
    }
    writer->modParam("drain", allowAudioDrain);
}

bool AudioThr::resampler_create()
{
    const double speed = playC.speed > 0.0 ? playC.speed : 1.0;
    if (realSample_rate != sample_rate || realChannels != channels || speed != 1.0)
    {
        const bool OK = sndResampler.create(realSample_rate, realChannels, sample_rate / speed, channels);
        if (!OK)
            QMPlay2Core.logError(tr("Error during initialization") + ": " + sndResampler.name());
        return OK;
    }
    sndResampler.destroy();
    return true;
}

inline uchar AudioThr::currentChannels() const
{
    return m_resamplerFirst ? channels : realChannels;
}
inline uint AudioThr::currentSampleRate() const
{
    return m_resamplerFirst ? sample_rate : realSample_rate;
}

void AudioThr::pauseVis(bool b)
{
    for (QMPlay2Extensions *vis : qAsConst(visualizations))
        vis->visState(!b, currentChannels(), currentSampleRate());
}
