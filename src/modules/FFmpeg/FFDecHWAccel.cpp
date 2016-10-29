/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

#include <FFDecHWAccel.hpp>
#include <FFCommon.hpp>

#include <VideoFrame.hpp>
#include <StreamInfo.hpp>

extern "C"
{
	#include <libavformat/avformat.h>
}

FFDecHWAccel::FFDecHWAccel(QMutex &mutex) :
	FFDec(mutex),
	hwAccelWriter(NULL)
{}
FFDecHWAccel::~FFDecHWAccel()
{
	if (hwAccelWriter)
		av_free(codec_ctx->hwaccel_context);
}

VideoWriter *FFDecHWAccel::HWAccel() const
{
	return hwAccelWriter;
}

bool FFDecHWAccel::hasHWAccel(const char *hwaccelName)
{
	AVHWAccel *avHWAccel = NULL;
	while ((avHWAccel = av_hwaccel_next(avHWAccel)))
		if (avHWAccel->id == codec_ctx->codec_id && strstr(avHWAccel->name, hwaccelName))
			break;
	return avHWAccel;
}

int FFDecHWAccel::decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &, bool flush, unsigned)
{
	int frameFinished = 0;
	decodeFirstStep(encodedPacket, flush);
	const int bytes_consumed = avcodec_decode_video2(codec_ctx, frame, &frameFinished, packet);
	if (frameFinished)
	{
		decoded = VideoFrame(VideoFrameSize(frame->width, frame->height), (quintptr)frame->data[3], (bool)frame->interlaced_frame, (bool)frame->top_field_first);
		decodeLastStep(encodedPacket, frame);
	}
	else
		encodedPacket.ts.setInvalid();
	return bytes_consumed > 0 ? bytes_consumed : 0;
}
