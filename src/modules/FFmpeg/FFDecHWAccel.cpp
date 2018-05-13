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

#include <FFDecHWAccel.hpp>
#include <HWAccelHelper.hpp>

#include <VideoFrame.hpp>

extern "C"
{
	#include <libavformat/avformat.h>
}

FFDecHWAccel::FFDecHWAccel(QMutex &mutex) :
	FFDec(mutex),
	m_hwAccelWriter(nullptr),
	m_hasCriticalError(false)
{}
FFDecHWAccel::~FFDecHWAccel()
{
	if (codec_ctx)
	{
		if (codec_ctx->hwaccel_context)
			av_free(codec_ctx->hwaccel_context);
		if (codec_ctx->opaque)
			delete (HWAccelHelper *)codec_ctx->opaque;
	}
}

VideoWriter *FFDecHWAccel::HWAccel() const
{
	return m_hwAccelWriter;
}

bool FFDecHWAccel::hasHWAccel(const char *hwaccelName) const
{
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(58, 9, 100)
	return (av_hwdevice_find_type_by_name(hwaccelName) != AV_HWDEVICE_TYPE_NONE);
#else
	AVHWAccel *hwAccel = nullptr;
	while ((hwAccel = av_hwaccel_next(hwAccel)))
		if (hwAccel->id == codec_ctx->codec_id && strstr(hwAccel->name, hwaccelName))
			break;
	return hwAccel;
#endif
}

int FFDecHWAccel::decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurryUp)
{
	Q_UNUSED(newPixFmt)
	int frameFinished = 0;

	decodeFirstStep(encodedPacket, flush);

	if (hurryUp > 1)
		codec_ctx->skip_frame = AVDISCARD_NONREF;
	else if (hurryUp == 0)
		codec_ctx->skip_frame = AVDISCARD_DEFAULT;

	const int bytes_consumed = avcodec_decode_video2(codec_ctx, frame, &frameFinished, packet);

	if (frameFinished && ~hurryUp)
	{
		if (m_hwAccelWriter)
			decoded = VideoFrame(VideoFrameSize(frame->width, frame->height), (quintptr)frame->data[3], (bool)frame->interlaced_frame, (bool)frame->top_field_first);
		else
			downloadVideoFrame(decoded);
	}

	if (frameFinished)
		decodeLastStep(encodedPacket, frame);
	else
		encodedPacket.ts.setInvalid();

	m_hasCriticalError = (bytes_consumed < 0);

	return bytes_consumed < 0 ? -1 : bytes_consumed;
}
void FFDecHWAccel::downloadVideoFrame(VideoFrame &decoded)
{
	Q_UNUSED(decoded)
}

bool FFDecHWAccel::hasCriticalError() const
{
	return m_hasCriticalError;
}
