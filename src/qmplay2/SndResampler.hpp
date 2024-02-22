/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

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

#include <QMPlay2Lib.hpp>

#include <memory>

class QByteArray;
struct SwrContext;

namespace RubberBand {

class RubberBandStretcher;

}

class QMPLAY2SHAREDLIB_EXPORT SndResampler
{
public:
    static bool canKeepPitch();

public:
    SndResampler();
    ~SndResampler();

    const char *name() const;

    inline bool isOpen() const
    {
        return m_sndConvertCtx != nullptr;
    }

    bool create(int srcSamplerate, int srcChannels, int dstSamplerate, int dstChannels, double speed, bool keepPitch);
    void convert(const QByteArray &src, QByteArray &dst, bool flush);
    void cleanBuffers();
    void destroy();

    double getDelay() const;
    bool hasBufferedSamples() const;

private:
    SwrContext *m_sndConvertCtx = nullptr;
    std::unique_ptr<RubberBand::RubberBandStretcher> m_rubberBandStretcher;
    bool m_keepPitch = false;
    int m_srcSamplerate = 0;
    int m_srcChannels = 0;
    int m_dstSamplerate = 0;
    int m_dstChannels = 0;
    double m_speed = 0.0;
};
