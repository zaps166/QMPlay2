/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

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

class VoiceRemoval final : public AudioFilter
{
public:
    VoiceRemoval(Module &);

    bool set() override;
private:
    bool setAudioParameters(uchar, uint) override;
    double filter(Buffer &, bool) override;

    bool enabled, hasParameters, canFilter;
    uchar chn;
};

#define VoiceRemovalName "Voice Removal"
