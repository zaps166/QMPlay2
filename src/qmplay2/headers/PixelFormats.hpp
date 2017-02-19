/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

#ifndef PIXELFORMATS_HPP
#define PIXELFORMATS_HPP

#include <QVector>

enum QMPlay2PixelFormat //Compatible with FFmpeg
{
	QMPLAY2_PIX_FMT_YUV420P =  0,
	QMPLAY2_PIX_FMT_YUV422P =  4,
	QMPLAY2_PIX_FMT_YUV444P =  5,
	QMPLAY2_PIX_FMT_YUV410P =  6,
	QMPLAY2_PIX_FMT_YUV411P =  7,
	QMPLAY2_PIX_FMT_YUV440P = 33,

	QMPLAY2_PIX_FMT_COUNT   =  6
};
using QMPlay2PixelFormats = QVector<QMPlay2PixelFormat>;

#endif // PIXELFORMATS_HPP
