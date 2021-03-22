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

#ifdef Q_OS_WIN
    mWasapiStreamInfo.size = sizeof(PaWasapiStreamInfo);
    mWasapiStreamInfo.hostApiType = paWASAPI;
    mWasapiStreamInfo.version = 1;
    mWasapiStreamInfo.streamCategory = eAudioCategoryMedia;

    m_outputParameters.hostApiSpecificStreamInfo = &mWasapiStreamInfo;
#endif

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
#ifdef Q_OS_WIN
    const bool exclusive = sets().getBool("Exclusive");
    if (m_exclusive != exclusive || mWasapiStreamInfo.flags == 0 || mWasapiStreamInfo.threadPriority == 0)
    {
        m_exclusive = exclusive;
        if (m_exclusive)
        {
            mWasapiStreamInfo.flags = paWinWasapiExclusive | paWinWasapiThreadPriority;
            mWasapiStreamInfo.threadPriority = eThreadPriorityProAudio;
        }
        else
        {
            mWasapiStreamInfo.flags = paWinWasapiThreadPriority | paWinWasapiAutoConvert;
            mWasapiStreamInfo.threadPriority = eThreadPriorityAudio;
        }
        restartPlaying = true;
    }
#endif
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

    const int chn = getParam("chn").toInt();
    const int rate = getParam("rate").toInt();
    const int devIdx = PortAudioCommon::getDeviceIndexForOutput(m_outputDevice, chn);

    if (m_outputParameters.device != devIdx)
    {
        m_outputParameters.device = devIdx;
        resetAudio = true;
    }

    if (m_outputParameters.channelCount != chn)
    {
        resetAudio = true;
        m_outputParameters.channelCount = chn;
    }

    if (m_sampleRate != rate)
    {
        resetAudio = true;
        m_sampleRate = rate;
    }

    if (paramsCorrected && deviceNeedsChangeParams(&m_outputParameters.channelCount, &m_sampleRate))
    {
        modParam("chn", m_outputParameters.channelCount);
        modParam("rate", m_sampleRate);
        resetAudio = true;
        *paramsCorrected = true;
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
#ifdef Q_OS_WIN
        if (m_exclusive)
            name += QString(", %1Hz").arg(m_sampleRate);
#endif
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

bool PortAudioWriter::deviceNeedsChangeParams(int *newChn, int *newRate)
{
    auto outParams = m_outputParameters;
    int rate = m_sampleRate;

    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(outParams.device);
    if (!deviceInfo)
        return false;

    bool needsChange = false;

    for (int i = 0; i < 2; ++i)
    {
        const auto err = Pa_IsFormatSupported(nullptr, &outParams, rate);
        if (err == paInvalidChannelCount)
        {
            if (outParams.channelCount != deviceInfo->maxOutputChannels)
            {
                outParams.channelCount = deviceInfo->maxOutputChannels;
                needsChange = true;
            }
        }
        else if (err == paInvalidSampleRate)
        {
            if (rate != deviceInfo->defaultSampleRate)
            {
                rate = deviceInfo->defaultSampleRate;
                needsChange = true;
            }
        }
        else
        {
            break;
        }
    }

    if (needsChange)
    {
        if (newChn)
            *newChn = outParams.channelCount;
        if (newRate)
            *newRate = rate;
        return true;
    }

    return false;
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
    if (deviceNeedsChangeParams())
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
    if (Pa_IsStreamStopped(m_stream))
        return;

    writeStream(QByteArray(sizeof(float) * m_outputParameters.channelCount * m_outputLatency * m_sampleRate, 0));
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
