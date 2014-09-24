#ifndef HWACCELHELPER_HPP
#define HWACCELHELPER_HPP

typedef unsigned long QMPlay2SurfaceID;

#define QMPlay2InvalidSurfaceID ((QMPlay2SurfaceID)-1)

struct AVCodecContext;
struct AVFrame;

class HWAccelHelper
{
public:
	virtual ~HWAccelHelper() {}

	static int get_buffer( AVCodecContext *codec_ctx, AVFrame *frame );
	static void release_buffer( AVCodecContext *codec_ctx, AVFrame *frame );

	virtual QMPlay2SurfaceID getSurface() = 0;
	virtual void putSurface( QMPlay2SurfaceID id ) = 0;
};

#endif // HWACCELHELPER_HPP
