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

#include <ALSACommon.hpp>

#include <alsa/asoundlib.h>

ALSACommon::DevicesList ALSACommon::getDevices()
{
    DevicesList devices;

    snd_ctl_card_info_t *cardInfo;
    snd_ctl_card_info_alloca(&cardInfo);
    snd_pcm_info_t *pcmInfo;
    snd_pcm_info_alloca(&pcmInfo);
    int cardIdx = -1;
    while (!snd_card_next(&cardIdx) && cardIdx >= 0)
    {
        const QString dev = "hw:" + QString::number(cardIdx);
        snd_ctl_t *ctl;
        if (!snd_ctl_open(&ctl, dev.toLocal8Bit(), 0))
        {
            if (!snd_ctl_card_info(ctl, cardInfo))
            {
                const QString cardName = snd_ctl_card_info_get_name(cardInfo);
                int devIdx = -1;
                while (!snd_ctl_pcm_next_device(ctl, &devIdx) && devIdx >= 0)
                {
                    snd_pcm_info_set_device(pcmInfo, devIdx);
                    snd_pcm_info_set_stream(pcmInfo, SND_PCM_STREAM_PLAYBACK);
                    if (snd_ctl_pcm_info(ctl, pcmInfo) >= 0)
                    {
                        const QString pcmName = snd_pcm_info_get_name(pcmInfo);
                        devices.first += dev + "," + QString::number(devIdx);
                        devices.second += cardName + (!pcmName.isEmpty() ? QString(": ") + snd_pcm_info_get_name(pcmInfo) : QString());
                    }
                }
            }
            snd_ctl_close(ctl);
        }
    }

    char **hints;
    if (!snd_device_name_hint(-1, "pcm", (void ***)&hints)) //add "defaults.namehint.!showall on" to .asoundrc
    {
        char **n = hints;
        while (*n != nullptr)
        {
            char *name = snd_device_name_get_hint(*n, "NAME");
            if (name)
            {
                if (strcmp(name, "null"))
                {
                    char *colon = strchr(name, ':');
                    if (colon)
                        *colon = '\0';
                    if (!devices.first.contains(name))
                    {
                        devices.first += name;
                        char *desc = snd_device_name_get_hint(*n, "DESC");
                        if (!desc)
                            devices.second += QString();
                        else
                        {
                            const QStringList descL = QString(desc).split(',');
                            devices.second += descL.count() > 1 ? descL[1].simplified() : descL[0].simplified();
                            free(desc);
                        }
                    }
                }
                free(name);
            }
            ++n;
        }
        snd_device_name_free_hint((void **)hints);
    }

    return devices;
}

QString ALSACommon::getDeviceName(const ALSACommon::DevicesList &devicesList, const QString &deviceName)
{
    int devIdx = devicesList.first.indexOf(deviceName);
    if (devIdx < 0)
    {
        devIdx = devicesList.first.indexOf("default");
        if (devIdx < 0)
            devIdx = devicesList.first.indexOf("sysdefault");
        if (devIdx < 0 && !devicesList.first.isEmpty())
            devIdx = 0;
    }
    return devIdx > -1 ? devicesList.first[devIdx] : QString();
}
