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

#ifndef IMGSCALER_HPP
#define IMGSCALER_HPP

#include <stddef.h>

/* YUV planar to RGB32 */

class VideoFrameSize;
struct SwsContext;
class VideoFrame;

class ImgScaler
{
public:
	ImgScaler();
	inline ~ImgScaler()
	{
		destroy();
	}

	bool create(const VideoFrameSize &size, int newWdst, int newHdst, bool isNV12 = false);
	void scale(const VideoFrame &videoFrame, void *dst = NULL);
	void scale(const void *src[], const int srcLinesize[], void *dst);
	void destroy();
private:
	SwsContext *m_swsCtx;
	int m_srcH, m_dstLinesize;
};

#endif
