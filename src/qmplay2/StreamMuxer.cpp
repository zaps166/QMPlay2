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

#include <StreamMuxer.hpp>

#include <StreamInfo.hpp>
#include <Packet.hpp>

#include <QLoggingCategory>

#include <unordered_map>

Q_LOGGING_CATEGORY(mux, "StreamMuxer")

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/samplefmt.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/pixdesc.h>
}

#include <cmath>

using namespace std;

struct StreamData
{
    int64_t lastDts = AV_NOPTS_VALUE;
    bool firstFrame = true;
};

struct StreamMuxer::Priv
{
    AVFormatContext *ctx = nullptr;
    AVPacket *pkt = nullptr;
    bool streamRecording = false;
    unordered_map<int, StreamData> streamData;
    double firstDts = 0.0;
};

StreamMuxer::StreamMuxer(const QString &fileName, const QList<StreamInfo *> &streamsInfo, const QString &format, bool streamRecording)
    : p(*new Priv)
{
    p.streamRecording = streamRecording;
    if (avformat_alloc_output_context2(&p.ctx, nullptr, format.toLatin1().constData(), nullptr) < 0)
        return;
    if (avio_open(&p.ctx->pb, fileName.toUtf8(), AVIO_FLAG_WRITE) < 0)
        return;
    bool isRawVideo = false;
    for (StreamInfo *streamInfo : streamsInfo)
    {
        AVCodecID codecId = streamInfo->params->codec_id;
        if (codecId == AV_CODEC_ID_NONE)
        {
            if (streamInfo->params->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                Q_ASSERT(format == QStringLiteral("wav"));
                codecId = AV_CODEC_ID_PCM_F32LE;
            }
            else
            {
                return;
            }
        }

        AVStream *stream = avformat_new_stream(p.ctx, nullptr);
        if (!stream)
            return;

        stream->time_base = streamInfo->time_base;

        stream->codecpar->codec_type = streamInfo->params->codec_type;
        stream->codecpar->codec_id = codecId;
        if (stream->codecpar->codec_id == AV_CODEC_ID_RAWVIDEO)
        {
            stream->codecpar->codec_tag = streamInfo->params->codec_tag;
            isRawVideo = true;
        }

        if (streamInfo->params->extradata_size > 0)
        {
            stream->codecpar->extradata = (uint8_t *)av_mallocz(streamInfo->getExtraDataCapacity());
            stream->codecpar->extradata_size = streamInfo->params->extradata_size;
            memcpy(stream->codecpar->extradata, streamInfo->params->extradata, stream->codecpar->extradata_size);
        }

        switch (streamInfo->params->codec_type)
        {
            case AVMEDIA_TYPE_VIDEO:
                stream->codecpar->width = streamInfo->params->width;
                stream->codecpar->height = streamInfo->params->height;
                stream->codecpar->format = streamInfo->params->format;
                stream->codecpar->sample_aspect_ratio = streamInfo->params->sample_aspect_ratio;
                stream->avg_frame_rate = streamInfo->fps;
                if (streamInfo->is_default)
                    stream->disposition |= AV_DISPOSITION_DEFAULT;
                break;
            case AVMEDIA_TYPE_AUDIO:
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 28, 100)
                stream->codecpar->ch_layout = streamInfo->params->ch_layout;
#else
                stream->codecpar->channels = streamInfo->params->channels;
#endif
                stream->codecpar->sample_rate = streamInfo->params->sample_rate;
                stream->codecpar->block_align = streamInfo->params->block_align;
                stream->codecpar->format = streamInfo->params->format;
                break;
            default:
                break;
        }

        // TODO: Metadata
    }
    AVDictionary *options = nullptr;
    if (isRawVideo)
        av_dict_set(&options, "allow_raw_vfw", "1", 0);
    if (avformat_write_header(p.ctx, &options) < 0)
        return;
    p.pkt = av_packet_alloc();
}
StreamMuxer::~StreamMuxer()
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

bool StreamMuxer::isOk() const
{
    return static_cast<bool>(p.pkt);
}

bool StreamMuxer::setFirstDts(const Packet &packet, const int idx)
{
    if (!p.streamRecording)
        return false;

    if (packet.hasKeyFrame() && packet.hasDts())
        p.firstDts = max(p.firstDts, packet.dts());

    return true;
}

bool StreamMuxer::write(const Packet &packet, const int idx)
{
    const double timeBase = av_q2d(p.ctx->streams[idx]->time_base);

    auto &streamData = p.streamData[idx];

    if (streamData.firstFrame)
    {
        if (!packet.hasKeyFrame())
        {
            qCDebug(mux) << "Skipping first packet, because it is not key frame, stream:" << idx;
            return true; // Skip packet if we can't start with key frame
        }

        streamData.firstFrame = false;
    }

    p.pkt->duration = round(packet.duration() / timeBase);
    if (packet.hasDts())
        p.pkt->dts = round((packet.dts() - p.firstDts) / timeBase);
    if (packet.hasPts())
        p.pkt->pts = round((packet.pts() - p.firstDts) / timeBase);
    p.pkt->flags = packet.hasKeyFrame() ? AV_PKT_FLAG_KEY : 0;
    p.pkt->buf = packet.getBufferRef();
    p.pkt->data = packet.data();
    p.pkt->size = packet.size();
    p.pkt->stream_index = idx;

    bool dtsOk = false;
    if (streamData.lastDts == AV_NOPTS_VALUE)
    {
        if (p.pkt->dts != AV_NOPTS_VALUE)
        {
            streamData.lastDts = p.pkt->dts;
            dtsOk = true;
        }
    }
    else
    {
        if (p.pkt->dts != AV_NOPTS_VALUE && p.pkt->dts >= streamData.lastDts)
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
