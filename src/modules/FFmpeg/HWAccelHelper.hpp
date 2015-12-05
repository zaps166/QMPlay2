#ifndef HWACCELHELPER_HPP
#define HWACCELHELPER_HPP

#include <stdint.h>

typedef uintptr_t QMPlay2SurfaceID;

#define QMPlay2InvalidSurfaceID ((QMPlay2SurfaceID)-1)

struct AVCodecContext;
struct AVFrame;

class HWAccelHelper
{
public:
	virtual ~HWAccelHelper() {}

	static int get_buffer( AVCodecContext *codec_ctx, AVFrame *frame, int flags );

	virtual QMPlay2SurfaceID getSurface() = 0;
	virtual void putSurface( QMPlay2SurfaceID id ) = 0;
};

#endif // HWACCELHELPER_HPP
