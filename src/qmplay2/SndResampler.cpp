/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2021  Błażej Szczygieł

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

#include <QByteArray>

#include <cmath>

extern "C"
{
    #include <libswresample/swresample.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/opt.h>
}

const char *SndResampler::name() const
{
    return "SWResample";
}

bool SndResampler::create(int _src_samplerate, int _src_channels, int _dst_samplerate, int _dst_channels)
{
    destroy();

    src_samplerate = _src_samplerate;
    src_channels  = _src_channels;
    dst_samplerate = _dst_samplerate;
    dst_channels = _dst_channels;
    int src_chn_layout = av_get_default_channel_layout(src_channels);
    int dst_chn_layout = av_get_default_channel_layout(dst_channels);
    if (!src_samplerate || !dst_samplerate || !src_chn_layout || !dst_chn_layout)
        return false;

    if (!(snd_convert_ctx = swr_alloc_set_opts(nullptr, dst_chn_layout, AV_SAMPLE_FMT_FLT, dst_samplerate, src_chn_layout, AV_SAMPLE_FMT_FLT, src_samplerate, 0, nullptr)))
        return false;

    av_opt_set_int(snd_convert_ctx, "linear_interp", true, 0);
    if (dst_channels > src_channels)
    {
        double matrix[dst_channels][src_channels];
        memset(matrix, 0, sizeof matrix);
        for (int i = 0, c = 0; i < dst_channels; i++)
        {
            matrix[i][c] = 1.0;
            c = (c + 1) % src_channels;
        }
        swr_set_matrix(snd_convert_ctx, (const double *)matrix, src_channels);
    }

    if (swr_init(snd_convert_ctx))
    {
        destroy();
        return false;
    }
    return true;
}
void SndResampler::convert(const QByteArray &src, QByteArray &dst)
{
    const int in_size = src.size() / src_channels / sizeof(float);
    const int out_size = std::ceil(in_size * (double)dst_samplerate / (double)src_samplerate);

    dst.reserve(out_size * sizeof(float) * dst_channels);

    const quint8 *in[] = {(const quint8 *)src.constData()};
    quint8 *out[] = {(quint8 *)dst.data()};

    const int converted = swr_convert(snd_convert_ctx, out, out_size, in, in_size);
    if (converted > 0)
        dst.resize(converted * sizeof(float) * dst_channels);
    else
        dst.clear();
}
void SndResampler::destroy()
{
    swr_free(&snd_convert_ctx);
}
