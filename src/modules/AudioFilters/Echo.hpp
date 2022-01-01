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

class Echo final : public AudioFilter
{
public:
    Echo(Module &);

    bool set() override;
private:
    bool setAudioParameters(uchar, uint) override;
    double filter(QByteArray &, bool) override;

    void alloc(bool);

    bool enabled, hasParameters, canFilter;

    uint echo_delay, echo_volume, echo_repeat;
    bool echo_surround;

    uchar chn;
    uint srate;

    int w_ofs;
    QVector<float> sampleBuffer;
};

#define EchoName "Echo"
