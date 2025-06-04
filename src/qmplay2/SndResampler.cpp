/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2025  Błażej Szczygieł

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

#include <SndResampler.hpp>

#include <QVarLengthArray>
#include <QByteArray>
#include <QDebug>

#include <cmath>

#ifdef QMPLAY2_RUBBERBAND
#   include <rubberband/RubberBandStretcher.h>
#else
namespace RubberBand {

class RubberBandStretcher
{};

}
#endif

extern "C"
{
    #include <libswresample/swresample.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/opt.h>
}

using namespace std;
using namespace RubberBand;

bool SndResampler::canKeepPitch()
{
#ifdef QMPLAY2_RUBBERBAND
    return true;
#else
    return false;
#endif
}

SndResampler::SndResampler()
{
}
SndResampler::~SndResampler()
{
    destroy();
}

const char *SndResampler::name() const
{
    return "SWResample";
}

bool SndResampler::create(int srcSamplerate, int srcChannels, int dstSamplerate, int dstChannels, double speed, bool keepPitch)
{
    m_keepPitch = keepPitch;

#ifdef QMPLAY2_RUBBERBAND
    if (m_keepPitch && qFuzzyCompare(speed, 1.0))
#endif
    {
        m_keepPitch = false;
    }

#ifdef QMPLAY2_RUBBERBAND
    if (!m_keepPitch || m_dstSamplerate != dstSamplerate || m_dstChannels != dstChannels)
        m_rubberBandStretcher.reset();
#endif

    m_srcSamplerate = srcSamplerate;
    m_srcChannels  = srcChannels;
    m_dstSamplerate = dstSamplerate;
    m_dstChannels = dstChannels;
    m_speed = speed;

    if (!m_keepPitch)
    {
        m_dstSamplerate /= speed;
    }

    if (m_srcSamplerate <= 0 || m_dstSamplerate <= 0 || m_srcChannels <= 0 || m_dstChannels <= 0)
        return false;

    if (m_sndConvertCtx)
        swr_close(m_sndConvertCtx);

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 28, 100)
    AVChannelLayout srcChnLayout, dstChnLayout;
    av_channel_layout_default(&srcChnLayout, srcChannels);
    av_channel_layout_default(&dstChnLayout, dstChannels);

    swr_alloc_set_opts2(
        &m_sndConvertCtx,
        &dstChnLayout,
        m_keepPitch ? AV_SAMPLE_FMT_FLTP : AV_SAMPLE_FMT_FLT,
        m_dstSamplerate,
        &srcChnLayout,
        AV_SAMPLE_FMT_FLT,
        m_srcSamplerate,
        0,
        nullptr
    );
#else
    const int64_t srcChnLayout = av_get_default_channel_layout(m_srcChannels);
    const int64_t dstChnLayout = av_get_default_channel_layout(m_dstChannels);

    if (srcChnLayout == 0 || dstChnLayout == 0)
    {
        destroy();
        return false;
    }

    m_sndConvertCtx = swr_alloc_set_opts(
        m_sndConvertCtx,
        dstChnLayout,
        m_keepPitch ? AV_SAMPLE_FMT_FLTP : AV_SAMPLE_FMT_FLT,
        m_dstSamplerate,
        srcChnLayout,
        AV_SAMPLE_FMT_FLT,
        m_srcSamplerate,
        0,
        nullptr
    );
#endif

    if (!m_sndConvertCtx)
    {
        destroy();
        return false;
    }

    av_opt_set_int(m_sndConvertCtx, "linear_interp", true, 0);
    if (m_dstChannels > m_srcChannels)
    {
        QVarLengthArray<double, 8 * 8> matrix(m_dstChannels * m_srcChannels);
        memset(matrix.data(), 0, matrix.size() * sizeof(double));
        for (int i = 0, c = 0; i < m_dstChannels; i++)
        {
            matrix[i * m_srcChannels + c] = 1.0;
            c = (c + 1) % m_srcChannels;
        }
        swr_set_matrix(m_sndConvertCtx, matrix.constData(), m_srcChannels);
    }

    if (swr_init(m_sndConvertCtx))
    {
        destroy();
        return false;
    }

    return true;
}
void SndResampler::convert(const QByteArray &src, QByteArray &dst, bool flush)
{
    const int inSize = src.size() / m_srcChannels / sizeof(float);
    const int possibleSwrSize = ceil(inSize * (double)m_dstSamplerate / (double)m_srcSamplerate);

#ifndef QMPLAY2_RUBBERBAND
    Q_UNUSED(flush)
#else
    if (m_keepPitch)
    {
        QVarLengthArray<QByteArray, 8> tmpQ(m_dstChannels);
        QVarLengthArray<quint8 *, 8> tmp(m_dstChannels);

        if (!m_rubberBandStretcher)
        {
            RubberBandStretcher::Options options = RubberBandStretcher::OptionProcessRealTime | RubberBandStretcher::OptionChannelsTogether;
            if (QStringLiteral(RUBBERBAND_VERSION) != QStringLiteral("3.0.0") || m_dstSamplerate >= 40000)
            {
                // Workaround for RubberBand 3.0.0 bug:
                // - low sample rates - bad audio quality
                // - very low sample rates - crash
                options |= RubberBandStretcher::OptionEngineFiner;
            }
            m_rubberBandStretcher = make_unique<RubberBandStretcher>(
                m_dstSamplerate,
                m_dstChannels,
                options
            );
        }

        const double newTimeRatio = 1.0 / m_speed;
        if (!qFuzzyCompare(m_rubberBandStretcher->getTimeRatio(), newTimeRatio))
            m_rubberBandStretcher->setTimeRatio(newTimeRatio);

        if (!flush)
        {
            const quint8 *in[] = {(const quint8 *)src.constData()};

            for (int i = 0; i < m_dstChannels; ++i)
            {
                tmpQ[i] = QByteArray(possibleSwrSize * sizeof(float), Qt::Uninitialized);
                tmp[i] = reinterpret_cast<quint8 *>(tmpQ[i].data());
            }

            const int converted = swr_convert(m_sndConvertCtx, tmp.constData(), possibleSwrSize, in, inSize);
            if (converted <= 0)
            {
                dst.clear();
                return;
            }

            m_rubberBandStretcher->process(reinterpret_cast<const float *const *>(tmp.constData()), converted, false);
        }
        else
        {
            for (int i = 0; i < m_dstChannels; ++i)
                tmp[i] = nullptr;
            m_rubberBandStretcher->process(reinterpret_cast<const float *const *>(tmp.constData()), 0, true);
        }

        const int available = m_rubberBandStretcher->available();
        if (available <= 0)
        {
            dst.clear();
            return;
        }

        for (int i = 0; i < m_dstChannels; ++i)
        {
            const int newSize = available * sizeof(float);
            if (tmpQ[i].size() < newSize)
                tmpQ[i] = QByteArray(available * sizeof(float), Qt::Uninitialized);
            tmp[i] = reinterpret_cast<quint8 *>(tmpQ[i].data());
        }

        m_rubberBandStretcher->retrieve(reinterpret_cast<float *const *>(tmp.constData()), available);

        dst = QByteArray(available * sizeof(float) * m_dstChannels, Qt::Uninitialized);
        auto dstF = reinterpret_cast<float *>(dst.data());
        for (int c = 0; c < m_dstChannels; ++c)
        {
            for (int i = 0; i < available; ++i)
            {
                dstF[i * m_dstChannels + c] = reinterpret_cast<const float *>(tmp[c])[i];
            }
        }

        if (flush)
            m_rubberBandStretcher.reset();
    }
    else
#endif
    {
        dst.reserve(possibleSwrSize * sizeof(float) * m_dstChannels);

        const quint8 *in[] = {(const quint8 *)src.constData()};
        quint8 *out[] = {(quint8 *)dst.data()};

        const int converted = swr_convert(m_sndConvertCtx, out, possibleSwrSize, in, inSize);
        if (converted > 0)
            dst.resize(converted * sizeof(float) * m_dstChannels);
        else
            dst.clear();
    }
}

void SndResampler::cleanBuffers()
{
#ifdef QMPLAY2_RUBBERBAND
    if (m_rubberBandStretcher)
        m_rubberBandStretcher->reset();
#endif
}
void SndResampler::destroy()
{
    swr_free(&m_sndConvertCtx);
#ifdef QMPLAY2_RUBBERBAND
    m_rubberBandStretcher.reset();
#endif
}

double SndResampler::getDelay() const
{
#ifdef QMPLAY2_RUBBERBAND
    return m_rubberBandStretcher
        ? static_cast<double>(m_rubberBandStretcher->getStartDelay()) / static_cast<double>(m_dstSamplerate)
        : 0.0
    ;
#else
    return 0.0;
#endif
}
bool SndResampler::hasBufferedSamples() const
{
#ifdef QMPLAY2_RUBBERBAND
    return m_rubberBandStretcher
        ? m_rubberBandStretcher->getSamplesRequired() > 0
        : false
    ;
#else
    return false;
#endif
}
