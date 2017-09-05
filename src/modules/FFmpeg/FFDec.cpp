/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

#include <FFDec.hpp>
#include <FFCommon.hpp>

#include <StreamInfo.hpp>

extern "C"
{
	#include <libavformat/avformat.h>
	#include <libavutil/pixdesc.h>
}

FFDec::FFDec(QMutex &avcodec_mutex) :
	codec_ctx(nullptr),
	packet(nullptr),
	frame(nullptr),
	codecIsOpen(false),
	avcodec_mutex(avcodec_mutex)
{}
FFDec::~FFDec()
{
	av_frame_free(&frame);
	FFCommon::freeAVPacket(packet);
	if (codecIsOpen)
	{
		avcodec_mutex.lock();
		avcodec_close(codec_ctx);
		avcodec_mutex.unlock();
	}
	av_free(codec_ctx);
}


AVCodec *FFDec::init(StreamInfo &streamInfo)
{
	AVCodec *codec = avcodec_find_decoder_by_name(streamInfo.codec_name);
	if (codec)
	{
		codec_ctx = avcodec_alloc_context3(codec);
		codec_ctx->codec_id = codec->id;
		codec_ctx->codec_tag = streamInfo.codec_tag;
		codec_ctx->bit_rate = streamInfo.bitrate;
		codec_ctx->channels = streamInfo.channels;
		codec_ctx->sample_rate = streamInfo.sample_rate;
		codec_ctx->block_align = streamInfo.block_align;
		codec_ctx->bits_per_coded_sample = streamInfo.bpcs;
		codec_ctx->pix_fmt = av_get_pix_fmt(streamInfo.format);
		codec_ctx->coded_width = codec_ctx->width = streamInfo.W;
		codec_ctx->coded_height = codec_ctx->height = streamInfo.H;
//		codec_ctx->debug_mv = FF_DEBUG_VIS_MV_P_FOR | FF_DEBUG_VIS_MV_B_FOR | FF_DEBUG_VIS_MV_B_BACK;
		if (!streamInfo.data.isEmpty())
		{
			codec_ctx->extradata = (uint8_t *)streamInfo.data.data();
			codec_ctx->extradata_size = streamInfo.data.size();
		}
	}
	return codec;
}
bool FFDec::openCodec(AVCodec *codec)
{
	avcodec_mutex.lock();
	if (avcodec_open2(codec_ctx, codec, nullptr))
	{
		avcodec_mutex.unlock();
		return false;
	}
	avcodec_mutex.unlock();
	packet = FFCommon::createAVPacket();
	switch (codec_ctx->codec_type)
	{
		case AVMEDIA_TYPE_VIDEO:
		case AVMEDIA_TYPE_AUDIO:
			frame = av_frame_alloc();
			break;
		default:
			break;
	}
	return (codecIsOpen = true);
}

void FFDec::decodeFirstStep(const Packet &encodedPacket, bool flush)
{
	packet->data = (quint8 *)encodedPacket.data();
	packet->size = encodedPacket.size();
	if (encodedPacket.ts.hasDts())
		packet->dts = qRound(encodedPacket.ts.dts() / time_base);
	if (encodedPacket.ts.hasPts())
		packet->pts = qRound(encodedPacket.ts.pts() / time_base);
	if (flush)
		avcodec_flush_buffers(codec_ctx);
	if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
		memcpy(&codec_ctx->reordered_opaque, &encodedPacket.sampleAspectRatio, 8);
}
void FFDec::decodeLastStep(Packet &encodedPacket, AVFrame *frame)
{
	const int64_t ts = av_frame_get_best_effort_timestamp(frame);
	if (ts != QMPLAY2_NOPTS_VALUE)
		encodedPacket.ts = ts * time_base;
	if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		double sampleAspectRatio;
		memcpy(&sampleAspectRatio, &frame->reordered_opaque, 8);
		if (qFuzzyIsNull(sampleAspectRatio) && frame->sample_aspect_ratio.num)
			encodedPacket.sampleAspectRatio = av_q2d(frame->sample_aspect_ratio);
	}
}
