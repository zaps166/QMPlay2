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

#include <QByteArray>

#include <pulse/simple.h>
#include <pulse/pulseaudio.h>

class Pulse
{
public:
    Pulse();

    inline bool isOK() const
    {
        return _isOK;
    }
    inline bool isOpen() const
    {
        return pulse;
    }

    bool start();
    void stop(bool drain = false);

    bool write(const QByteArray &, bool &showError);

    double delay;
    uchar channels;
    uint sample_rate;
private:
    bool _isOK, writing;

    pa_simple *pulse;
    pa_sample_spec ss;
};
