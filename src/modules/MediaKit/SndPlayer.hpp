// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QByteArray>

#include <SoundPlayer.h>

#include <Window.h>
#include <View.h>
#include <TextControl.h>

#include "RingBuffer.hpp"

class SndPlayer
{
public:
    SndPlayer();
    inline ~SndPlayer()
    {
        stop();
    }

    inline bool isOK() const
    {
        return _isOK;
    }
    inline bool isOpen() const
    {
        return player;
    }

    bool start();
    void stop();

    double getLatency();

    bool write( const QByteArray & );

    double delay;
    uchar channels;
    float sample_rate;

private:
    bool _isOK;
    BSoundPlayer *player;
    RingBuffer *ring;
};
