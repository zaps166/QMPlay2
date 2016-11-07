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

#include <HWAccelHelper.hpp>

extern "C"
{
	#include <libavcodec/avcodec.h>
}

static void release_buffer(HWAccelHelper *hwAccelHelper /*opaque*/, QMPlay2SurfaceID surface_id /*frame->data[3]*/)
{
	if (surface_id != QMPlay2InvalidSurfaceID)
		hwAccelHelper->putSurface(surface_id);
}

typedef void (*ReleaseBufferProc)(void *, uint8_t *);

/**/

bool HWAccelHelper::hasHWAccel(AVCodecContext *codec_ctx, const char *hwaccelName)
{
	AVHWAccel *avHWAccel = NULL;
	while ((avHWAccel = av_hwaccel_next(avHWAccel)))
		if (avHWAccel->id == codec_ctx->codec_id && strstr(avHWAccel->name, hwaccelName))
			break;
	return avHWAccel;
}

int HWAccelHelper::get_buffer(AVCodecContext *codec_ctx, AVFrame *frame, int /*flags*/)
{
	const QMPlay2SurfaceID surface_id = ((HWAccelHelper *)codec_ctx->opaque)->getSurface();
	if (surface_id != QMPlay2InvalidSurfaceID)
	{
		frame->data[3] = (uint8_t *)(uintptr_t)surface_id;
		frame->buf[0] = av_buffer_create(frame->data[3], 0, (ReleaseBufferProc)release_buffer, codec_ctx->opaque, AV_BUFFER_FLAG_READONLY);
		return 0;
	}
	/* This should never happen */
	fprintf(stderr, "Surface queue is empty!\n");
	return -1;
}
