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

#include <Common.hpp>

namespace ChiptuneCommon {

void doFadeOut(float *data, int len, int chn, int srate, double fadePos, double fadeTime)
{
    const double silenceStep = (1.0 / fadeTime) / srate;
    double silence = 1.0 - (fadePos / fadeTime); //More or less OK
    for (int i = 0; i < len; i += chn)
    {
        for (int c = 0; c < chn; ++c)
            data[i + c] *= silence;
        silence -= silenceStep;
        if (silence < 0.0)
            silence = 0.0;
    }
}

}
