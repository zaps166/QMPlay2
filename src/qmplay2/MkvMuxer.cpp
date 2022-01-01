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

#include <MkvMuxer.hpp>

#include <StreamInfo.hpp>
#include <Packet.hpp>

#include <QLoggingCategory>
#include <QMap>

Q_LOGGING_CATEGORY(mux, "MkvMuxer")

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/samplefmt.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/pixdesc.h>
}

#include <cmath>

using namespace std;

struct MkvMuxer::Priv
{
    AVFormatContext *ctx = nullptr;
    AVPacket *pkt = nullptr;
    QMap<int, int64_t> lastDts;
};

MkvMuxer::MkvMuxer(const QString &fileName, const QList<StreamInfo *> &streamsInfo)
    : p(*new Priv)
{
    if (avformat_alloc_output_context2(&p.ctx, nullptr, "matroska", nullptr) < 0)
        return;
    if (avio_open(&p.ctx->pb, fileName.toUtf8(), AVIO_FLAG_WRITE) < 0)
        return;
    for (StreamInfo *streamInfo : streamsInfo)
    {
        const AVCodec *codec = avcodec_find_decoder_by_name(streamInfo->codec_name);
        if (!codec)
            return;

        AVStream *stream = avformat_new_stream(p.ctx, nullptr);
        if (!stream)
            return;

        stream->time_base = streamInfo->time_base;

        stream->codecpar->codec_type = streamInfo->codec_type;
        stream->codecpar->codec_id = codec->id;

        if (streamInfo->extradata_size > 0)
        {
            stream->codecpar->extradata = (uint8_t *)av_mallocz(streamInfo->getExtraDataCapacity());
            stream->codecpar->extradata_size = streamInfo->extradata_size;
            memcpy(stream->codecpar->extradata, streamInfo->extradata, stream->codecpar->extradata_size);
        }

        switch (streamInfo->codec_type)
        {
            case AVMEDIA_TYPE_VIDEO:
                stream->codecpar->width = streamInfo->width;
                stream->codecpar->height = streamInfo->height;
                stream->codecpar->format = streamInfo->format;
                stream->codecpar->sample_aspect_ratio = streamInfo->sample_aspect_ratio;
                stream->avg_frame_rate = streamInfo->fps;
                if (streamInfo->is_default)
                    stream->disposition |= AV_DISPOSITION_DEFAULT;
                break;
            case AVMEDIA_TYPE_AUDIO:
                stream->codecpar->channels = streamInfo->channels;
                stream->codecpar->sample_rate = streamInfo->sample_rate;
                stream->codecpar->block_align = streamInfo->block_align;
                stream->codecpar->format = streamInfo->format;
                break;
            default:
                break;
        }

        // TODO: Metadata
    }
    if (avformat_write_header(p.ctx, nullptr) < 0)
        return;
    p.pkt = av_packet_alloc();
}
MkvMuxer::~MkvMuxer()
{
    if (p.ctx)
    {
        if (p.ctx->pb)
        {
            if (p.pkt)
            {
                av_interleaved_write_frame(p.ctx, nullptr); // Flush interleaving queue
                av_write_trailer(p.ctx);
                av_packet_free(&p.pkt);
            }
            avio_close(p.ctx->pb);
            p.ctx->pb = nullptr;
        }
        avformat_free_context(p.ctx);
    }
}

bool MkvMuxer::isOk() const
{
    return static_cast<bool>(p.pkt);
}

bool MkvMuxer::write(Packet &packet, const int idx)
{
    const double timeBase = av_q2d(p.ctx->streams[idx]->time_base);

    p.pkt->duration = round(packet.duration() / timeBase);
    if (packet.hasDts())
        p.pkt->dts = round(packet.dts() / timeBase);
    if (packet.hasPts())
        p.pkt->pts = round(packet.pts() / timeBase);
    p.pkt->flags = packet.hasKeyFrame() ? AV_PKT_FLAG_KEY : 0;
    p.pkt->buf = packet.getBufferRef();
    p.pkt->data = packet.data();
    p.pkt->size = packet.size();
    p.pkt->stream_index = idx;

    bool dtsOk = false;
    auto it = p.lastDts.find(idx);
    if (it == p.lastDts.end())
    {
        if (p.pkt->dts != AV_NOPTS_VALUE)
        {
            p.lastDts[idx] = p.pkt->dts;
            dtsOk = true;
        }
    }
    else
    {
        const auto lastDts = it.value();
        if (p.pkt->dts != AV_NOPTS_VALUE && p.pkt->dts >= lastDts)
            dtsOk = true;
    }
    if (!dtsOk)
    {
        qCWarning(mux) << "Skipping packet with invalid dts in stream" << idx;
        return true;
    }

    const int err = av_interleaved_write_frame(p.ctx, p.pkt);
    return (err == 0);
}
