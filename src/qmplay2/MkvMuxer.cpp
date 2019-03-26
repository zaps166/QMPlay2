/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2018  Błażej Szczygieł

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

#include <MkvMuxer.hpp>

#include <StreamInfo.hpp>
#include <Packet.hpp>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/samplefmt.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/pixdesc.h>
}

#include <cmath>

using namespace std;

MkvMuxer::MkvMuxer(const QString &fileName, const QList<StreamInfo *> &streamsInfo)
{
    if (avformat_alloc_output_context2(&m_ctx, nullptr, "matroska", nullptr) < 0)
        return;
    if (avio_open(&m_ctx->pb, fileName.toUtf8(), AVIO_FLAG_WRITE) < 0)
        return;
    for (StreamInfo *streamInfo : streamsInfo)
    {
        const AVCodec *codec = avcodec_find_decoder_by_name(streamInfo->codec_name);
        if (!codec)
            return;

        AVStream *stream = avformat_new_stream(m_ctx, nullptr);
        if (!stream)
            return;

        stream->time_base.num = streamInfo->time_base.num;
        stream->time_base.den = streamInfo->time_base.den;

        stream->codecpar->codec_type = (AVMediaType)streamInfo->type;
        stream->codecpar->codec_id = codec->id;

        if (streamInfo->data.size() > 0)
        {
            stream->codecpar->extradata = (uint8_t *)av_mallocz(streamInfo->data.capacity());
            stream->codecpar->extradata_size = streamInfo->data.size();
            memcpy(stream->codecpar->extradata, streamInfo->data.constData(), stream->codecpar->extradata_size);
        }

        switch (streamInfo->type)
        {
            case QMPLAY2_TYPE_VIDEO:
                stream->codecpar->width = streamInfo->W;
                stream->codecpar->height = streamInfo->H;
                stream->codecpar->format = av_get_pix_fmt(streamInfo->format);
                stream->codecpar->sample_aspect_ratio = av_d2q(streamInfo->sample_aspect_ratio, 10000);
                stream->avg_frame_rate = av_d2q(streamInfo->FPS, 10000);
                if (streamInfo->is_default)
                    stream->disposition |= AV_DISPOSITION_DEFAULT;
                break;
            case QMPLAY2_TYPE_AUDIO:
                stream->codecpar->channels = streamInfo->channels;
                stream->codecpar->sample_rate = streamInfo->sample_rate;
                stream->codecpar->block_align = streamInfo->block_align;
                stream->codecpar->format = av_get_sample_fmt(streamInfo->format);
                break;
            default:
                break;
        }

        // TODO: Metadata
    }
    if (avformat_write_header(m_ctx, nullptr) < 0)
        return;
    m_ok = true;
}
MkvMuxer::~MkvMuxer()
{
    if (m_ctx)
    {
        if (m_ctx->pb)
        {
            if (m_ok)
            {
                av_interleaved_write_frame(m_ctx, nullptr); // Flush interleaving queue
                av_write_trailer(m_ctx);
            }
            avio_close(m_ctx->pb);
            m_ctx->pb = nullptr;
        }
        avformat_free_context(m_ctx);
    }
}

bool MkvMuxer::write(Packet &packet, const int idx)
{
    const AVStream *stream = m_ctx->streams[idx];
    const double timeBase = (double)stream->time_base.num / (double)stream->time_base.den;

    AVPacket pkt;
    av_init_packet(&pkt);

    pkt.duration = round(packet.duration / timeBase);
    if (packet.ts.hasDts())
        pkt.dts = round(packet.ts.dts() / timeBase);
    if (packet.ts.hasPts())
        pkt.pts = round(packet.ts.pts() / timeBase);
    pkt.flags = packet.hasKeyFrame ? AV_PKT_FLAG_KEY : 0;
    pkt.buf = packet.toAvBufferRef();
    pkt.data = pkt.buf->data + packet.offset();
    pkt.size = packet.size();
    pkt.stream_index = idx;

    return (av_interleaved_write_frame(m_ctx, &pkt) >= 0);
}
