/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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
	m_swsCtx(nullptr),
	m_srcH(0), m_dstLinesize(0)
{}

bool ImgScaler::create(const VideoFrameSize &size, int newWdst, int newHdst, bool isNV12)
{
	m_srcH = size.height;
	m_dstLinesize = newWdst << 2;
	return (m_swsCtx = sws_getCachedContext(m_swsCtx, size.width, m_srcH, isNV12 ? AV_PIX_FMT_NV12 : (AVPixelFormat)size.getFormat(), newWdst, newHdst, AV_PIX_FMT_RGB32, SWS_BILINEAR, nullptr, nullptr, nullptr));
}
void ImgScaler::scale(const VideoFrame &src, void *dst)
{
	const quint8 *srcData[3] = {
		src.buffer[0].constData(),
		src.buffer[1].constData(),
		src.buffer[2].constData() //Ignored for NV12
	};
	sws_scale(m_swsCtx, srcData, src.linesize, 0, m_srcH, (uint8_t **)&dst, &m_dstLinesize);
}
void ImgScaler::scale(const void *src[], const int srcLinesize[], void *dst)
{
	sws_scale(m_swsCtx, (const uint8_t **)src, srcLinesize, 0, m_srcH, (uint8_t **)&dst, &m_dstLinesize);
}
void ImgScaler::destroy()
{
	if (m_swsCtx)
	{
		sws_freeContext(m_swsCtx);
		m_swsCtx = nullptr;
	}
}
