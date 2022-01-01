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

#include <VoiceRemoval.hpp>

VoiceRemoval::VoiceRemoval(Module &module) :
    hasParameters(false), canFilter(false)
{
    SetModule(module);
}

bool VoiceRemoval::set()
{
    enabled = sets().getBool("VoiceRemoval");
    canFilter = enabled && hasParameters;
    return true;
}

bool VoiceRemoval::setAudioParameters(uchar chn, uint /*srate*/)
{
    hasParameters = chn >= 2;
    if (hasParameters)
        this->chn = chn;
    canFilter = enabled && hasParameters;
    return hasParameters;
}
double VoiceRemoval::filter(QByteArray &data, bool)
{
    if (canFilter)
    {
        const int size = data.size() / sizeof(float);
        float *samples = (float *)data.data();

        for (int i = 0; i < size; i += chn)
            samples[i] = samples[i+1] = samples[i] - samples[i+1];
    }
    return 0.0;
}
