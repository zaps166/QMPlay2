// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <PulseAudioHeaders.hpp>

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
