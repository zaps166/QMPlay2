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

/* Algorithm from XMMS 1.x */

#include <Echo.hpp>

Echo::Echo(Module &module) :
    hasParameters(false), canFilter(false)
{
    SetModule(module);
}

bool Echo::set()
{
    enabled = sets().getBool("Echo");
    echo_delay = sets().getUInt("Echo/Delay");
    echo_volume = sets().getUInt("Echo/Volume");
    echo_repeat = sets().getUInt("Echo/Feedback");
    echo_surround = sets().getBool("Echo/Surround");
    if (echo_delay > 1000)
        echo_delay = 1000;
    if (echo_volume > 100)
        echo_volume = 100;
    if (echo_repeat > 100)
        echo_repeat = 100;
    alloc(enabled && hasParameters);
    return true;
}

bool Echo::setAudioParameters(uchar chn, uint srate)
{
    hasParameters = chn && srate;
    if (hasParameters)
    {
        this->chn = chn;
        this->srate = srate;
    }
    alloc(enabled && hasParameters);
    return hasParameters;
}
double Echo::filter(QByteArray &data, bool)
{
    if (canFilter)
    {
        const int size = data.size() / sizeof(float);
        const int sampleBufferSize = sampleBuffer.size();
        float *sampleBufferData = sampleBuffer.data();
        const int repeat_div = echo_surround ? 200 : 100;
        float *samples = (float *)data.data();
        int r_ofs = w_ofs - (srate * echo_delay / 1000) * chn;
        if (r_ofs < 0)
            r_ofs += sampleBufferSize;
        for (int i = 0; i < size; i++)
        {
            float sample = sampleBufferData[r_ofs];
            if (echo_surround && chn >= 2)
            {
                if (i & 1)
                    sample -= sampleBufferData[r_ofs - 1];
                else
                    sample -= sampleBufferData[r_ofs + 1];
            }
            sampleBufferData[w_ofs] = samples[i] + sample * echo_repeat / repeat_div;
            samples[i] += sample * echo_volume / 100;
            if (++r_ofs >= sampleBufferSize)
                r_ofs -= sampleBufferSize;
            if (++w_ofs >= sampleBufferSize)
                w_ofs -= sampleBufferSize;
        }
    }
    return 0.0;
}

void Echo::alloc(bool b)
{
    if (!b || srate * chn != (uint)sampleBuffer.size())
        sampleBuffer.clear();
    if (b && sampleBuffer.isEmpty())
    {
        w_ofs = 0;
        sampleBuffer.fill(0.0, srate * chn);
    }
    canFilter = b;
}
