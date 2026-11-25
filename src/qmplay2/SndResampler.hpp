// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

#ifndef QMPLAY2_RUBBERBAND
namespace RubberBand {
class RubberBandStretcher;
}
#endif

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
