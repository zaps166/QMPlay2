/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2021  Błażej Szczygieł

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

#include <PortAudioWriter.hpp>
#include <QMPlay2Core.hpp>

#include <QElapsedTimer>
#include <QThread>
#include <QDebug>

#ifdef Q_OS_WIN
#   include <audioclient.h>
#endif

#ifdef Q_OS_MACOS
    #include "3rdparty/CoreAudio/AudioDeviceList.h"
    #include "3rdparty/CoreAudio/AudioDevice.h"
#endif

PortAudioWriter::PortAudioWriter(Module &module)
{
    addParam("delay");
    addParam("chn");
    addParam("rate");
    addParam("drain");

    m_outputParameters.sampleFormat = paFloat32;
    m_outputParameters.hostApiSpecificStreamInfo = nullptr;

    SetModule(module);
}
PortAudioWriter::~PortAudioWriter()
{
    close();
#ifdef Q_OS_MACOS
    delete m_coreAudioDevice;
#endif
    if (m_initialized)
        Pa_Terminate();
}

bool PortAudioWriter::set()
{
    bool restartPlaying = false;
    const double delay = sets().getDouble("Delay");
    const QString newOutputDevice = sets().getString("OutputDevice");
#ifdef Q_OS_MACOS
    const bool bitPerfect = sets().getBool("BitPerfect");
#endif
    if (m_outputDevice != newOutputDevice)
    {
        m_outputDevice = newOutputDevice;
        restartPlaying = true;
    }
    if (!qFuzzyCompare(m_outputParameters.suggestedLatency, delay))
    {
        m_outputParameters.suggestedLatency = delay;
        restartPlaying = true;
    }
#ifdef Q_OS_MACOS
    if (m_bitPerfect != bitPerfect)
    {
        m_bitPerfect = bitPerfect;
        restartPlaying = true;
    }
#endif
    return !restartPlaying && sets().getBool("WriterEnabled");
}

bool PortAudioWriter::readyWrite() const
{
    return m_stream && !m_err;
}

bool PortAudioWriter::processParams(bool *paramsCorrected)
{
    bool resetAudio = false;

    int chn = getParam("chn").toInt();

    const int devIdx = PortAudioCommon::getDeviceIndexForOutput(m_outputDevice, chn);
    if (m_outputParameters.device != devIdx)
    {
        m_outputParameters.device = devIdx;
        resetAudio = true;
    }

    if (paramsCorrected)
    {
        const int newChn = getRequiredDeviceChn(chn);
        if (newChn > 0)
        {
            chn = newChn;
            modParam("chn", chn);
            *paramsCorrected = true;
        }
    }

    if (m_outputParameters.channelCount != chn)
    {
        resetAudio = true;
        m_outputParameters.channelCount = chn;
    }

    int rate = getParam("rate").toInt();
    if (m_sampleRate != rate)
    {
        resetAudio = true;
        m_sampleRate = rate;
    }

    if (resetAudio || m_err)
    {
        close();
        if (!openStream())
        {
            QMPlay2Core.logError("PortAudio :: " + tr("Cannot open audio output stream"));
            m_err = true;
        }
    }

    return readyWrite();
}
qint64 PortAudioWriter::write(const QByteArray &arr)
{
    if (!readyWrite())
        return 0;

    if (Pa_IsStreamStopped(m_stream))
    {
        if (!startStream())
        {
            playbackError();
            return 0;
        }
    }

    if (!writeStream(arr))
    {
        bool isError = true;

#ifdef Q_OS_WIN
        if (isDeviceInvalidated() && reopenStream()) //"writeStream()" must fail only on "paUnanticipatedHostError"
        {
            isError = !writeStream(arr);
        }
#endif

        if (isError)
        {
            playbackError();
            return 0;
        }
    }

    return arr.size();
}
void PortAudioWriter::pause()
{
    if (readyWrite())
    {
        drain();
        Pa_AbortStream(m_stream);
    }
}

QString PortAudioWriter::name() const
{
    QString name = PortAudioWriterName;
    if (m_stream)
    {
        if (const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(m_outputParameters.device))
            name += " (" + PortAudioCommon::getOutputDeviceName(deviceInfo) + ")";
#ifdef Q_OS_MACOS
        if (m_coreAudioDevice)
            name += QStringLiteral(", %1Hz").arg(m_coreAudioDevice->CurrentNominalSampleRate());
#endif
    }
    return name;
}

bool PortAudioWriter::open()
{
    if (Pa_Initialize() == paNoError)
    {
        m_initialized = true;
        return true;
    }
    return false;
}

int PortAudioWriter::getRequiredDeviceChn(int chn)
{
    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(m_outputParameters.device);
    if (!deviceInfo)
        return 0;

    auto isWASAPI = [&] {
#ifdef Q_OS_WIN
        if (auto hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi))
            return (hostApiInfo->type == paWASAPI);
#endif
        return false;
    };

    const int maxOutChn = deviceInfo->maxOutputChannels;
    if (maxOutChn < chn || (isWASAPI() && maxOutChn > 2 && maxOutChn != chn))
        return maxOutChn;

    return 0;
}

/**/

bool PortAudioWriter::openStream()
{
    PaStream *newStream = nullptr;
    if (Pa_OpenStream(&newStream, nullptr, &m_outputParameters, m_sampleRate, 0, paDitherOff, nullptr, nullptr) == paNoError)
    {
        m_stream = newStream;
        m_outputLatency = Pa_GetStreamInfo(m_stream)->outputLatency;
        modParam("delay", m_outputLatency);
#ifdef Q_OS_MACOS
        if (m_bitPerfect)
        {
            const QString devName(Pa_GetDeviceInfo(m_outputParameters.device)->name);
            const AudioDeviceList::DeviceDict devDict = AudioDeviceList().GetDict();
            if (devDict.contains(devName))
            {
                m_coreAudioDevice = AudioDevice::GetDevice(devDict[devName], false, m_coreAudioDevice);
                if (m_coreAudioDevice)
                    m_coreAudioDevice->SetNominalSampleRate(m_sampleRate);
            }
            else
            {
                delete m_coreAudioDevice;
                m_coreAudioDevice = nullptr;
            }
        }
        else
        {
            delete m_coreAudioDevice;
            m_coreAudioDevice = nullptr;
        }
#endif
        return true;
    }
    return false;
}
bool PortAudioWriter::startStream()
{
    const PaError e = Pa_StartStream(m_stream);
    if (e != paNoError)
    {
#ifdef Q_OS_WIN
        if (e == paUnanticipatedHostError && isDeviceInvalidated())
            return reopenStream();
#endif
        return false;
    }
    return true;
}
bool PortAudioWriter::writeStream(const QByteArray &arr)
{
    const PaError e = Pa_WriteStream(m_stream, arr.constData(), arr.size() / m_outputParameters.channelCount / sizeof(float));
    return (e != paUnanticipatedHostError);
}
void PortAudioWriter::playbackError()
{
    if (!m_dontShowError)
        QMPlay2Core.logError("PortAudio :: " + tr("Playback error"));
    m_err = true;
}

#ifdef Q_OS_WIN
bool PortAudioWriter::isDeviceInvalidated() const
{
    const PaHostErrorInfo *errorInfo = Pa_GetLastHostErrorInfo();
    return errorInfo && errorInfo->hostApiType == paWASAPI && errorInfo->errorCode == AUDCLNT_E_DEVICE_INVALIDATED;
}
bool PortAudioWriter::reopenStream()
{
    Pa_CloseStream(m_stream);
    m_stream = nullptr;

    Pa_Terminate();
    if (Pa_Initialize() != paNoError)
    {
        m_initialized = false;
        return false;
    }

    m_outputParameters.device = PortAudioCommon::getDeviceIndexForOutput(m_outputDevice);
    if (getRequiredDeviceChn(m_outputParameters.channelCount) > 0)
    {
        QMPlay2Core.processParam("RestartPlaying");
        m_dontShowError = true;
        return false;
    }

    if (openStream())
        return (Pa_StartStream(m_stream) == paNoError);

    return false;
}
#endif

void PortAudioWriter::drain()
{
    const qint64 maxTime = m_outputLatency * 1000.0;

    QElapsedTimer et;
    et.start();

    do
    {
        const double avail = Pa_GetStreamWriteAvailable(m_stream) - m_outputLatency * m_sampleRate;
        if (avail < 0.0)
            QThread::msleep(10);
        else
            break;
    } while (et.elapsed() <= maxTime);
}

void PortAudioWriter::close()
{
    if (m_stream)
    {
        if (!m_err && getParam("drain").toBool())
            drain();
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
    }
#ifdef Q_OS_MACOS
    if (m_coreAudioDevice)
        m_coreAudioDevice->ResetNominalSampleRate();
#endif
    m_err = false;
}
