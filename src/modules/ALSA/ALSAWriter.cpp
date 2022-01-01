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

#include <ALSAWriter.hpp>

#include <alsa/asoundlib.h>
#include <cmath>

#if SND_LIB_VERSION >= 0x1001B
    #define HAVE_CHMAP
#endif

template<typename T>
static void convert_samples(const float *src, const int samples, T *int_samples, unsigned channels)
{
    constexpr unsigned min = 1 << ((sizeof(T) << 3) - 1);
    constexpr unsigned max = min - 1;
    for (int i = 0; i < samples; ++i)
    {
        if (src[i] >= 1.0f)
            int_samples[i] = max;
        else if (src[i] <= -1.0f)
            int_samples[i] = min;
        else
            int_samples[i] = static_cast<double>(src[i]) * max;
    }
    if (channels == 6 || channels == 8)
    {
        T tmp;
        for (int i = 0; i < samples; i += channels)
        {
            tmp = int_samples[i+2];
            int_samples[i+2] = int_samples[i+4];
            int_samples[i+4] = tmp;
            tmp = int_samples[i+3];
            int_samples[i+3] = int_samples[i+5];
            int_samples[i+5] = tmp;
        }
    }
}

static bool set_snd_pcm_hw_params(snd_pcm_t *snd, snd_pcm_hw_params_t *params, snd_pcm_format_t fmt, unsigned &channels, unsigned &sample_rate, unsigned &delay_us)
{
    const bool ok = !snd_pcm_hw_params_set_access(snd, params, SND_PCM_ACCESS_RW_INTERLEAVED) && !snd_pcm_hw_params_set_format(snd, params, fmt) && !snd_pcm_hw_params_set_channels_near(snd, params, &channels) && !snd_pcm_hw_params_set_rate_near(snd, params, &sample_rate, nullptr);
    if (ok)
    {
        unsigned period_us = delay_us >> 2;
        return (!snd_pcm_hw_params_set_buffer_time_near(snd, params, &delay_us, nullptr) && !snd_pcm_hw_params_set_period_time_near(snd, params, &period_us, nullptr)) || (!snd_pcm_hw_params_set_period_time_near(snd, params, &period_us, nullptr) && !snd_pcm_hw_params_set_buffer_time_near(snd, params, &delay_us, nullptr));
    }
    return false;
}

/**/

ALSAWriter::ALSAWriter(Module &module) :
    snd(nullptr),
    delay(0.0),
    sample_rate(0), channels(0),
    autoFindMultichannelDevice(false), err(false)
{
    addParam("delay");
    addParam("rate");
    addParam("chn");
    addParam("drain");

    SetModule(module);
}
ALSAWriter::~ALSAWriter()
{
    close();
}

bool ALSAWriter::set()
{
    const double _delay = sets().getDouble("Delay");
    const QString _devName = ALSACommon::getDeviceName(ALSACommon::getDevices(), sets().getString("OutputDevice"));
    const bool _autoFindMultichannelDevice = sets().getBool("AutoFindMultichnDev");
    const bool restartPlaying = _delay != delay || _devName != devName || _autoFindMultichannelDevice != autoFindMultichannelDevice;
    delay = _delay;
    devName = _devName;
    autoFindMultichannelDevice = _autoFindMultichannelDevice;
    return !restartPlaying && sets().getBool("WriterEnabled");
}

bool ALSAWriter::readyWrite() const
{
    return snd && !err;
}

bool ALSAWriter::processParams(bool *paramsCorrected)
{
    const unsigned chn = getParam("chn").toUInt();
    const unsigned rate = getParam("rate").toUInt();
    const bool resetAudio = channels != chn || sample_rate != rate;
    channels = chn;
    sample_rate = rate;
    if (resetAudio || err)
    {
        snd_pcm_hw_params_t *params;
        snd_pcm_hw_params_alloca(&params);

        close();

        QString chosenDevName = devName;
        if (autoFindMultichannelDevice && channels > 2)
        {
            bool mustAutoFind = true, forceStereo = false;
            if (!snd_pcm_open(&snd, chosenDevName.toLocal8Bit(), SND_PCM_STREAM_PLAYBACK, 0))
            {
                if (snd_pcm_type(snd) == SND_PCM_TYPE_HW)
                {
                    unsigned max_chn = 0;
                    snd_pcm_hw_params_any(snd, params);
                    mustAutoFind = snd_pcm_hw_params_get_channels_max(params, &max_chn) || max_chn < channels;
                }
#ifdef HAVE_CHMAP
                else if (paramsCorrected)
                {
                    snd_pcm_chmap_query_t **chmaps = snd_pcm_query_chmaps(snd);
                    if (chmaps)
                        snd_pcm_free_chmaps(chmaps);
                    else
                        forceStereo = true;
                }
#endif
                snd_pcm_close(snd);
                snd = nullptr;
            }
            if (mustAutoFind)
            {
                QString newDevName;
                if (channels <= 4)
                    newDevName = "surround40";
                else if (channels <= 6)
                    newDevName = "surround51";
                else
                    newDevName = "surround71";
                if (!newDevName.isEmpty() && newDevName != chosenDevName)
                {
                    if (ALSACommon::getDevices().first.contains(newDevName))
                        chosenDevName = newDevName;
                    else if (forceStereo)
                    {
                        channels = 2;
                        *paramsCorrected = true;
                    }
                }
            }
        }
        if (!chosenDevName.isEmpty())
        {
            bool sndOpen = !snd_pcm_open(&snd, chosenDevName.toLocal8Bit(), SND_PCM_STREAM_PLAYBACK, 0);
            if (devName != chosenDevName)
            {
                if (sndOpen)
                    QMPlay2Core.logInfo("ALSA :: " + devName + "\" -> \"" + chosenDevName + "\"");
                else
                {
                    sndOpen = !snd_pcm_open(&snd, devName.toLocal8Bit(), SND_PCM_STREAM_PLAYBACK, 0);
                    QMPlay2Core.logInfo("ALSA :: " + tr("Cannot open") + " \"" + chosenDevName + "\", " + tr("back to") + " \"" + devName + "\"");
                }
            }
            if (sndOpen)
            {
                snd_pcm_hw_params_any(snd, params);

                snd_pcm_format_t fmt = SND_PCM_FORMAT_UNKNOWN;
                if (!snd_pcm_hw_params_test_format(snd, params, SND_PCM_FORMAT_S32))
                {
                    fmt = SND_PCM_FORMAT_S32;
                    sample_size = 4;
                }
                else if (!snd_pcm_hw_params_test_format(snd, params, SND_PCM_FORMAT_S16))
                {
                    fmt = SND_PCM_FORMAT_S16;
                    sample_size = 2;
                }
                else if (!snd_pcm_hw_params_test_format(snd, params, SND_PCM_FORMAT_S8))
                {
                    fmt = SND_PCM_FORMAT_S8;
                    sample_size = 1;
                }

                unsigned delay_us = round(delay * 1000000.0);
                if (fmt != SND_PCM_FORMAT_UNKNOWN && set_snd_pcm_hw_params(snd, params, fmt, channels, sample_rate, delay_us))
                {
                    bool err2 = false;
                    if (channels != chn || sample_rate != rate)
                    {
                        if (paramsCorrected)
                            *paramsCorrected = true;
                        else
                            err2 = true;
                    }
                    if (!err2)
                    {
                        err2 = snd_pcm_hw_params(snd, params);
                        if (err2 && paramsCorrected) //jakiś błąd, próba zmiany sample_rate
                        {
                            snd_pcm_hw_params_any(snd, params);
                            err2 = snd_pcm_hw_params_set_rate_resample(snd, params, false) || !set_snd_pcm_hw_params(snd, params, fmt, channels, sample_rate, delay_us) || snd_pcm_hw_params(snd, params);
                            if (!err2)
                                *paramsCorrected = true;
                        }
                        if (!err2)
                        {
                            modParam("delay", delay_us / 1000000.0);
                            if (paramsCorrected && *paramsCorrected)
                            {
                                modParam("chn", channels);
                                modParam("rate", sample_rate);
                            }

                            canPause = snd_pcm_hw_params_can_pause(params) && snd_pcm_hw_params_can_resume(params);

                            mustSwapChn = channels == 6 || channels == 8;
#ifdef HAVE_CHMAP
                            if (mustSwapChn)
                            {
                                snd_pcm_chmap_query_t **chmaps = snd_pcm_query_chmaps(snd);
                                if (chmaps)
                                {
                                    for (int i = 0; chmaps[i]; ++i)
                                    {
                                        if (chmaps[i]->map.channels >= channels)
                                        {
                                            for (uint j = 0; j < channels; ++j)
                                            {
                                                mustSwapChn &= chmaps[i]->map.pos[j] == j + SND_CHMAP_FL;
                                                if (!mustSwapChn)
                                                    break;
                                            }
                                            break;
                                        }
                                    }
                                    snd_pcm_free_chmaps(chmaps);
                                }
                            }
#endif
                            return true;
                        }
                    }
                }
            }
        }
        err = true;
        QMPlay2Core.logError("ALSA :: " + tr("Cannot open audio output stream"));
    }

    return readyWrite();
}
qint64 ALSAWriter::write(const QByteArray &arr)
{
    if (!readyWrite())
        return 0;

    const int samples = arr.size() / sizeof(float);
    const int to_write = samples / channels;

    const int bytes = samples * sample_size;
    if (int_samples.size() < bytes)
        int_samples.resize(bytes);
    switch (sample_size)
    {
        case 4:
            convert_samples((const float *)arr.constData(), samples, (qint32 *)int_samples.constData(), mustSwapChn ? channels : 0);
            break;
        case 2:
            convert_samples((const float *)arr.constData(), samples, (qint16 *)int_samples.constData(), mustSwapChn ? channels : 0);
            break;
        case 1:
            convert_samples((const float *)arr.constData(), samples, (qint8 *)int_samples.constData(), mustSwapChn ? channels : 0);
            break;
    }
    switch (snd_pcm_state(snd))
    {
        case SND_PCM_STATE_XRUN:
            if (!snd_pcm_prepare(snd))
            {
                const int silence = snd_pcm_avail(snd) - to_write;
                if (silence > 0)
                {
                    QByteArray silenceArr(silence * channels * sample_size, 0);
                    snd_pcm_writei(snd, silenceArr.constData(), silence);
                }
            }
            break;
        case SND_PCM_STATE_PAUSED:
            snd_pcm_pause(snd, false);
            break;
        default:
            break;
    }
    int ret = snd_pcm_writei(snd, int_samples.constData(), to_write);
    if (ret < 0 && ret != -EPIPE && snd_pcm_recover(snd, ret, false))
    {
        QMPlay2Core.logError("ALSA :: " + tr("Playback error"));
        err = true;
        return 0;
    }

    return arr.size();
}
void ALSAWriter::pause()
{
    if (canPause)
        snd_pcm_pause(snd, true);
}

QString ALSAWriter::name() const
{
    return ALSAWriterName;
}

bool ALSAWriter::open()
{
    return true;
}

/**/

void ALSAWriter::close()
{
    if (snd)
    {
        if (!err && getParam("drain").toBool())
            snd_pcm_drain(snd);
        else
            snd_pcm_drop(snd);
        snd_pcm_close(snd);
        snd = nullptr;
    }
    err = false;
}
