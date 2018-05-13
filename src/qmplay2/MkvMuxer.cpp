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

#if LIBAVFORMAT_VERSION_INT >= 0x392900
	static inline AVCodecParameters *codecParams(AVStream *stream)
	{
		return stream->codecpar;
	}
	#define HAS_CODECPAR
#else
	static inline AVCodecContext *codecParams(AVStream *stream)
	{
		return stream->codec;
	}
#endif

/**/

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
#ifndef HAS_CODECPAR
		stream->codec->time_base = stream->time_base;
		if (m_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
#endif

		codecParams(stream)->codec_type = (AVMediaType)streamInfo->type;
		codecParams(stream)->codec_id = codec->id;

		if (streamInfo->data.size() > 0)
		{
			codecParams(stream)->extradata = (uint8_t *)av_mallocz(streamInfo->data.capacity());
			codecParams(stream)->extradata_size = streamInfo->data.size();
			memcpy(codecParams(stream)->extradata, streamInfo->data.constData(), codecParams(stream)->extradata_size);
		}

		switch (streamInfo->type)
		{
			case QMPLAY2_TYPE_VIDEO:
				codecParams(stream)->width = streamInfo->W;
				codecParams(stream)->height = streamInfo->H;
#ifdef HAS_CODECPAR
				codecParams(stream)->format = av_get_pix_fmt(streamInfo->format);
#else
				codecParams(stream)->pix_fmt = av_get_pix_fmt(streamInfo->format);
#endif
				codecParams(stream)->sample_aspect_ratio = av_d2q(streamInfo->sample_aspect_ratio, 10000);
				stream->avg_frame_rate = av_d2q(streamInfo->FPS, 10000);
				if (streamInfo->is_default)
					stream->disposition |= AV_DISPOSITION_DEFAULT;
				break;
			case QMPLAY2_TYPE_AUDIO:
				codecParams(stream)->channels = streamInfo->channels;
				codecParams(stream)->sample_rate = streamInfo->sample_rate;
				codecParams(stream)->block_align = streamInfo->block_align;
#ifdef HAS_CODECPAR
				codecParams(stream)->format = av_get_sample_fmt(streamInfo->format);
#else
				codecParams(stream)->sample_fmt = av_get_sample_fmt(streamInfo->format);
#endif
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
