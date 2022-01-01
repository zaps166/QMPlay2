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

#include <BS2B.hpp>

BS2B::BS2B(Module &module) :
    m_hasParameters(false), m_canFilter(false),
    m_srate(0),
    m_bs2b(nullptr)
{
    SetModule(module);
}
BS2B::~BS2B()
{
    bs2b_close(m_bs2b);
}

bool BS2B::set()
{
    m_enabled = sets().getBool("BS2B");
    m_fcut = sets().getInt("BS2B/Fcut");
    m_feed = sets().getDouble("BS2B/Feed") * 10;
    m_canFilter = m_enabled && m_hasParameters;
    alloc();
    return true;
}

bool BS2B::setAudioParameters(uchar chn, uint srate)
{
    m_hasParameters = (chn == 2);
    m_canFilter = m_enabled && m_hasParameters;
    m_srate = srate;
    alloc();
    return m_hasParameters;
}
void BS2B::clearBuffers()
{
    if (m_bs2b)
        bs2b_clear(m_bs2b);
}
double BS2B::filter(QByteArray &data, bool flush)
{
    Q_UNUSED(flush)
    if (m_canFilter)
        bs2b_cross_feed_f(m_bs2b, (float *)data.data(), data.size() / sizeof(float) / 2);
    return 0.0;
}

void BS2B::alloc()
{
    if (m_canFilter)
    {
        if (!m_bs2b)
            m_bs2b = bs2b_open();
        bs2b_set_srate(m_bs2b, m_srate);
        bs2b_set_level_fcut(m_bs2b, m_fcut);
        bs2b_set_level_feed(m_bs2b, m_feed);
    }
    else if (m_bs2b)
    {
        bs2b_close(m_bs2b);
        m_bs2b = nullptr;
    }
}
