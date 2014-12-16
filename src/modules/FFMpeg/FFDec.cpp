#include <FFDec.hpp>
#include <StreamInfo.hpp>

extern "C"
{
	#include <libavformat/avformat.h>
}

FFDec::FFDec( QMutex &avcodec_mutex ) :
	codec_ctx( NULL ),
	frame( NULL ),
	codecIsOpen( false ),
	avcodec_mutex( avcodec_mutex )
{}
FFDec::~FFDec()
{
	av_free( frame );
	if ( codecIsOpen )
	{
		avcodec_mutex.lock();
		avcodec_close( codec_ctx );
		avcodec_mutex.unlock();
	}
	av_free( codec_ctx );
}

bool FFDec::aspect_ratio_changed() const
{
	if ( codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO )
	{
		const int aspect_ratio = av_q2d( codec_ctx->sample_aspect_ratio ) * 1000;
		const bool b = last_aspect_ratio != aspect_ratio;
		last_aspect_ratio = aspect_ratio;
		return b;
	}
	return false;
}


AVCodec *FFDec::init( StreamInfo *_streamInfo )
{
	streamInfo = _streamInfo;
	AVCodec *codec = avcodec_find_decoder_by_name( streamInfo->codec_name );
	if ( codec )
	{
		codec_ctx = avcodec_alloc_context3( codec );
		codec_ctx->codec_id = codec->id;
		codec_ctx->bit_rate = streamInfo->bitrate;
		codec_ctx->channels = streamInfo->channels;
		codec_ctx->sample_rate = streamInfo->sample_rate;
		codec_ctx->block_align = streamInfo->block_align;
		codec_ctx->bits_per_coded_sample = streamInfo->bpcs;
		codec_ctx->pix_fmt = ( AVPixelFormat )streamInfo->img_fmt;
		codec_ctx->coded_width = codec_ctx->width = streamInfo->W;
		codec_ctx->coded_height = codec_ctx->height = streamInfo->H;
	//	codec_ctx->debug_mv = FF_DEBUG_VIS_MV_P_FOR | FF_DEBUG_VIS_MV_B_FOR || FF_DEBUG_VIS_MV_B_BACK;
		if ( codec->type != AVMEDIA_TYPE_SUBTITLE && !streamInfo->data.isEmpty() )
		{
			codec_ctx->extradata = ( uint8_t * )streamInfo->data.data();
			codec_ctx->extradata_size = streamInfo->data.size();
		}
	}
	return codec;
}
bool FFDec::openCodec( AVCodec *codec )
{
	avcodec_mutex.lock();
	if ( avcodec_open2( codec_ctx, codec, NULL ) )
	{
		avcodec_mutex.unlock();
		return false;
	}
	avcodec_mutex.unlock();
	time_base = streamInfo->getTimeBase();
	switch ( codec_ctx->codec_type )
	{
		case AVMEDIA_TYPE_VIDEO:
			last_aspect_ratio = av_q2d( codec_ctx->sample_aspect_ratio ) * 1000.0;
		case AVMEDIA_TYPE_AUDIO:
			frame = avcodec_alloc_frame();
			break;
		default:
			break;
	}
	return ( codecIsOpen = true );
}

void FFDec::decodeFirstStep( AVPacket &packet, Packet &encodedPacket, bool flush )
{
	av_init_packet( &packet );
	packet.data = ( quint8 * )encodedPacket.data();
	packet.size = encodedPacket.size();
	packet.dts = round( encodedPacket.ts.dts() / time_base );
	packet.pts = round( encodedPacket.ts.pts() / time_base );
	if ( flush )
		avcodec_flush_buffers( codec_ctx );
}
