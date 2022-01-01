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

#include <SwapStereo.hpp>

SwapStereo::SwapStereo(Module &module)
{
    SetModule(module);
}

bool SwapStereo::set()
{
    m_enabled = sets().getBool("SwapStereo");
    m_canFilter = m_enabled && m_hasParameters;
    return true;
}

bool SwapStereo::setAudioParameters(uchar chn, uint srate)
{
    Q_UNUSED(srate)
    m_hasParameters = (chn >= 2);
    if (m_hasParameters)
        m_chn = chn;
    m_canFilter = (m_enabled && m_hasParameters);
    return m_hasParameters;
}
double SwapStereo::filter(QByteArray &data, bool)
{
    if (m_canFilter)
    {
        const int size = data.size() / sizeof(float);
        float *samples = (float *)data.data();

        for (int i = 0; i < size; i += m_chn)
            qSwap(samples[i + 0], samples[i + 1]);
    }
    return 0.0;
}
