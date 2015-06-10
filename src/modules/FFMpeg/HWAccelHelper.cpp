#include <HWAccelHelper.hpp>

extern "C"
{
	#include <libavcodec/avcodec.h>
}

static void release_buffer( HWAccelHelper *hwAccelHelper /*opaque*/, QMPlay2SurfaceID surface_id /*frame->data[ 3 ]*/ )
{
	if ( surface_id != QMPlay2InvalidSurfaceID )
		hwAccelHelper->putSurface( surface_id );
}

typedef void ( *ReleaseBufferProc )( void *, uint8_t * );

/**/

int HWAccelHelper::get_buffer( AVCodecContext *codec_ctx, AVFrame *frame, int /*flags*/ )
{
	const QMPlay2SurfaceID surface_id = ( ( HWAccelHelper * )codec_ctx->opaque )->getSurface();
	if ( surface_id != QMPlay2InvalidSurfaceID )
	{
		frame->data[ 3 ] = ( uint8_t * )( uintptr_t )surface_id;
		frame->buf[ 0 ] = av_buffer_create( frame->data[ 3 ], 0, ( ReleaseBufferProc )release_buffer, codec_ctx->opaque, AV_BUFFER_FLAG_READONLY );
		return 0;
	}
	/* This should never happen */
	fprintf( stderr, "Surface queue is empty!\n" );
	return -1;
}
