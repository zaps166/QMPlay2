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

#include <PortAudioCommon.hpp>
#include <QMPlay2Core.hpp>

#include <portaudio.h>

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
