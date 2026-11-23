// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <PortAudioCommon.hpp>
#include <QMPlay2Core.hpp>

QString PortAudioCommon::getOutputDeviceName(const PaDeviceInfo *deviceInfo)
{
    return QString("%1: %2").arg(Pa_GetHostApiInfo(deviceInfo->hostApi)->name, deviceInfo->name);
}
QStringList PortAudioCommon::getOutputDeviceNames()
{
    if (Pa_Initialize() != paNoError)
        return {};

    QStringList outputDeviceNames;
    const int numDevices = Pa_GetDeviceCount();
    for (int i = 0; i < numDevices; i++)
    {
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo && deviceInfo->maxOutputChannels > 0)
            outputDeviceNames += getOutputDeviceName(deviceInfo);
    }

    Pa_Terminate();

    return outputDeviceNames;
}
int PortAudioCommon::getDeviceIndexForOutput(const QString &name, const int chn)
{
    if (!name.isEmpty())
    {
        const int numDevices = Pa_GetDeviceCount();
        for (int i = 0; i < numDevices; i++)
        {
            const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
            if (deviceInfo && deviceInfo->maxOutputChannels > 0 && getOutputDeviceName(deviceInfo) == name)
                return i;
        }
    }

    //Find the default device
#if defined Q_OS_LINUX
    if (chn > 0)
    {
        const char alsaDefault[] = "ALSA: default";
        if (getOutputDeviceNames().contains(alsaDefault))
            return getDeviceIndexForOutput(alsaDefault, 0);
    }
#else
    Q_UNUSED(chn)
#endif
    return Pa_GetDefaultOutputDevice();
}
