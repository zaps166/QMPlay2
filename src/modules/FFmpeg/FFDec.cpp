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

#include <FFDec.hpp>
#include <FFCommon.hpp>

#include <StreamInfo.hpp>

extern "C"
{
	#include <libavformat/avformat.h>
	#include <libavutil/pixdesc.h>
}

#include <cmath>

using namespace std;

FFDec::FFDec() :
	codec_ctx(nullptr),
	packet(nullptr),
	frame(nullptr),
	codecIsOpen(false)
{}
FFDec::~FFDec()
{
	destroyDecoder();
}

int FFDec::pendingFrames() const
{
	return m_frames.count();
}

void FFDec::destroyDecoder()
{
	clearFrames();
	av_frame_free(&frame);
	av_packet_free(&packet);
	if (codecIsOpen)
	{
		avcodec_close(codec_ctx);
		codecIsOpen = false;
	}
	av_freep(&codec_ctx);
}

void FFDec::clearFrames()
{
	for (AVFrame *&frame : m_frames)
		av_frame_free(&frame);
	m_frames.clear();
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
	if (avcodec_open2(codec_ctx, codec, nullptr))
		return false;
	packet = av_packet_alloc();
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
		packet->dts = round(encodedPacket.ts.dts() / time_base);
	if (encodedPacket.ts.hasPts())
		packet->pts = round(encodedPacket.ts.pts() / time_base);
	if (flush)
	{
		avcodec_flush_buffers(codec_ctx);
		clearFrames();
	}
	if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
		memcpy(&codec_ctx->reordered_opaque, &encodedPacket.sampleAspectRatio, 8);
}
int FFDec::decodeStep(bool &frameFinished)
{
	int bytesConsumed = 0;
	bool sendOk = false;

	int sendErr = avcodec_send_packet(codec_ctx, packet);
	if (sendErr == 0 || sendErr == AVERROR(EAGAIN))
	{
		bytesConsumed = packet->size;
		sendOk = true;
	}

	for (;;)
	{
		int recvErr = avcodec_receive_frame(codec_ctx, frame);
		if (recvErr == 0)
		{
			m_frames.push_back(frame);
			frame = av_frame_alloc();
		}
		else
		{
			if ((recvErr != AVERROR_EOF && recvErr != AVERROR(EAGAIN)) || (!sendOk && sendErr != AVERROR_EOF))
			{
				bytesConsumed = -1;
				clearFrames();
			}
			break;
		}
	}

	frameFinished = maybeTakeFrame();

	return bytesConsumed;
}
void FFDec::decodeLastStep(Packet &encodedPacket, AVFrame *frame)
{
	const int64_t ts = frame->best_effort_timestamp;
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

bool FFDec::maybeTakeFrame()
{
	if (!m_frames.isEmpty())
	{
		av_frame_free(&frame);
		frame = m_frames.takeFirst();
		return true;
	}
	return false;
}
