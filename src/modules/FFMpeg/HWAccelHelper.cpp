#include <HWAccelHelper.hpp>
#include <QtGlobal>

extern "C"
{
	#include <libavcodec/avcodec.h>
}

int HWAccelHelper::get_buffer( AVCodecContext *codec_ctx, AVFrame *frame )
{
	const QMPlay2SurfaceID surface_id = ( ( HWAccelHelper * )codec_ctx->opaque )->getSurface();
	if ( surface_id != QMPlay2InvalidSurfaceID )
	{
		frame->data[ 0 ] = frame->data[ 3 ] = ( unsigned char * )surface_id;
		frame->type = FF_BUFFER_TYPE_USER;
		return 0;
	}
	qDebug( "Invalid surface id, surface queue empty!" );
	return -1;
}
void HWAccelHelper::release_buffer( AVCodecContext *codec_ctx, AVFrame *frame )
{
	const QMPlay2SurfaceID surface_id = ( QMPlay2SurfaceID )frame->data[ 3 ];
	if ( surface_id != QMPlay2InvalidSurfaceID )
		( ( HWAccelHelper * )codec_ctx->opaque )->putSurface( surface_id );
	frame->data[ 0 ] = frame->data[ 3 ] = NULL;
}
