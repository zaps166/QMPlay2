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

#include <ImgScaler.hpp>

#include <VideoFrame.hpp>

extern "C"
{
	#include <libswscale/swscale.h>
}

ImgScaler::ImgScaler() :
	img_convert_ctx(NULL),
	Hsrc(0), dstLinesize(0)
{}

bool ImgScaler::create(const VideoFrameSize &size, int newWdst, int newHdst)
{
	Hsrc = size.height;
	dstLinesize = newWdst << 2;
	return (img_convert_ctx = sws_getCachedContext(img_convert_ctx, size.width, Hsrc, (AVPixelFormat)size.getFormat(), newWdst, newHdst, AV_PIX_FMT_RGB32, SWS_BILINEAR, NULL, NULL, NULL));
}
void ImgScaler::scale(const VideoFrame &src, void *dst)
{
	const quint8 *srcData[3] = {
		src.buffer[0].constData(),
		src.buffer[1].constData(),
		src.buffer[2].constData()
	};
	sws_scale(img_convert_ctx, srcData, src.linesize, 0, Hsrc, (uint8_t **)&dst, &dstLinesize);
}
void ImgScaler::scale(const void *src, const int srcLinesize[], int HChromaSrc, void *dst)
{
	uint8_t *srcData[3];
	srcData[0] = (uint8_t *)src;
	srcData[2] = (uint8_t *)srcData[0] + (srcLinesize[0] * Hsrc);
	srcData[1] = (uint8_t *)srcData[2] + (srcLinesize[1] * HChromaSrc);
	sws_scale(img_convert_ctx, srcData, srcLinesize, 0, Hsrc, (uint8_t **)&dst, &dstLinesize);
}
void ImgScaler::destroy()
{
	if (img_convert_ctx)
	{
		sws_freeContext(img_convert_ctx);
		img_convert_ctx = NULL;
	}
}
