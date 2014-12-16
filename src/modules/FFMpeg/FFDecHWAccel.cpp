#include <FFDecHWAccel.hpp>
#include <FFCommon.hpp>

#include <VideoFrame.hpp>
#include <StreamInfo.hpp>

extern "C"
{
	#include <libavformat/avformat.h>
}

FFDecHWAccel::FFDecHWAccel( QMutex &mutex ) :
	FFDec( mutex ),
	hwAccelWriter( NULL )
{}
FFDecHWAccel::~FFDecHWAccel()
{
	if ( hwAccelWriter )
		av_free( codec_ctx->hwaccel_context );
}

Writer *FFDecHWAccel::HWAccel() const
{
	return ( Writer * )hwAccelWriter;
}

bool FFDecHWAccel::canUseHWAccel( const StreamInfo *streamInfo )
{
	return streamInfo->type == QMPLAY2_TYPE_VIDEO && ( streamInfo->img_fmt == PIX_FMT_YUV420P || streamInfo->img_fmt == PIX_FMT_YUVJ420P );
}
bool FFDecHWAccel::hasHWAccel( const char *name )
{
	AVHWAccel *avHWAccel = NULL;
	while ( ( avHWAccel = av_hwaccel_next( avHWAccel ) ) )
		if ( avHWAccel->id == codec_ctx->codec_id && strstr( avHWAccel->name, name ) )
			break;
	return avHWAccel;
}

int FFDecHWAccel::decode( Packet &encodedPacket, QByteArray &decoded, bool flush, unsigned )
{
	AVPacket packet;
	int frameFinished = 0;
	decodeFirstStep( packet, encodedPacket, flush );
	const int bytes_consumed = avcodec_decode_video2( codec_ctx, frame, &frameFinished, &packet );
	if ( frameFinished )
	{
		VideoFrame::create( decoded, frame->data, frame->linesize, frame->interlaced_frame, frame->top_field_first );
		if ( frame->best_effort_timestamp != QMPLAY2_NOPTS_VALUE )
			encodedPacket.ts = frame->best_effort_timestamp * time_base;
	}
	else
		encodedPacket.ts.setInvalid();
	return bytes_consumed > 0 ? bytes_consumed : 0;
}
