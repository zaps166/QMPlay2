#include <FFDecVDPAU.hpp>
#include <VDPAUWriter.hpp>
#include <FFCommon.hpp>

#include <StreamInfo.hpp>

extern "C"
{
	#include <libavformat/avformat.h>
	#include <libavcodec/vdpau.h>
}

static AVPixelFormat get_format( AVCodecContext *, const AVPixelFormat * )
{
	return AV_PIX_FMT_VDPAU;
}

/**/

FFDecVDPAU::FFDecVDPAU( QMutex &avcodec_mutex, Module &module ) :
	FFDecHWAccel( avcodec_mutex )
{
	SetModule( module );
}

bool FFDecVDPAU::set()
{
	return sets().getBool( "DecoderVDPAUEnabled" );
}

QString FFDecVDPAU::name() const
{
	return "FFmpeg/VDPAU";
}

bool FFDecVDPAU::open( StreamInfo *streamInfo, Writer *writer )
{
	if ( canUseHWAccel( streamInfo ) )
	{
		AVCodec *codec = init( streamInfo );
		if ( codec && hasHWAccel( "vdpau" ) )
		{
			if ( writer && writer->name() != VDPAUWriterName )
				writer = NULL;
			hwAccelWriter = writer ? ( VideoWriter * )writer : new VDPAUWriter( getModule() );
			if ( ( writer || hwAccelWriter->open() ) && hwAccelWriter->HWAccellInit( codec_ctx->width, codec_ctx->height, avcodec_get_name( codec_ctx->codec_id ) ) )
			{
				codec_ctx->hwaccel_context = av_mallocz( sizeof( AVVDPAUContext ) );
				( ( AVVDPAUContext * )codec_ctx->hwaccel_context )->decoder = ( ( VDPAUWriter * )hwAccelWriter )->getVdpDecoder();
				( ( AVVDPAUContext * )codec_ctx->hwaccel_context )->render  = ( ( VDPAUWriter * )hwAccelWriter )->getVdpDecoderRender();
				codec_ctx->thread_count   = 1;
				codec_ctx->get_buffer2    = HWAccelHelper::get_buffer;
				codec_ctx->get_format     = get_format;
				codec_ctx->slice_flags    = SLICE_FLAG_CODED_ORDER | SLICE_FLAG_ALLOW_FIELD;
				codec_ctx->opaque         = dynamic_cast< HWAccelHelper * >( hwAccelWriter );
				if ( openCodec( codec ) )
					return true;
			}
			else
			{
				if ( !writer )
					delete hwAccelWriter;
				hwAccelWriter = NULL;
			}
		}
	}
	return false;
}
