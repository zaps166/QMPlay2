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

Writer *FFDecHWAccel::HWAccel() const
{
	return (Writer *)hwAccelWriter;
}

bool FFDecHWAccel::hasHWAccel(const char *hwaccelName)
{
	AVHWAccel *avHWAccel = NULL;
	while ((avHWAccel = av_hwaccel_next(avHWAccel)))
		if (avHWAccel->id == codec_ctx->codec_id && strstr(avHWAccel->name, hwaccelName))
			break;
	return avHWAccel;
}

int FFDecHWAccel::decode(Packet &encodedPacket, QByteArray &decoded, bool flush, unsigned)
{
	int frameFinished = 0;
	decodeFirstStep(encodedPacket, flush);
	const int bytes_consumed = avcodec_decode_video2(codec_ctx, frame, &frameFinished, packet);
	if (frameFinished)
	{
		VideoFrame::create(decoded, frame->data, frame->linesize, frame->interlaced_frame, frame->top_field_first);
		decodeLastStep(encodedPacket, frame);
	}
	else
		encodedPacket.ts.setInvalid();
	return bytes_consumed > 0 ? bytes_consumed : 0;
}
