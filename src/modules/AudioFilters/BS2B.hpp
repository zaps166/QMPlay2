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

#pragma once

#include <AudioFilter.hpp>

#include <bs2b/bs2b.hpp>

class BS2B final : public AudioFilter
{
public:
    BS2B(Module &module);
    ~BS2B();

    bool set() override;
private:
    bool setAudioParameters(uchar, uint srate) override;
    void clearBuffers() override;
    double filter(QByteArray &data, bool flush) override;

    void alloc();

    bool m_enabled, m_hasParameters, m_canFilter;
    qint32 m_fcut, m_feed;
    quint32 m_srate;
    t_bs2bd *m_bs2b;
};

#define BS2BName "Bauer stereophonic-to-binaural DSP"
