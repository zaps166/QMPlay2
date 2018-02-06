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

#pragma once

#include <QVector>

enum class QMPlay2PixelFormat //Compatible with FFmpeg
{
	YUV420P =  0,
	YUV422P =  4,
	YUV444P =  5,
	YUV410P =  6,
	YUV411P =  7,
	YUV440P = 33,

	Count   =  6
};
using QMPlay2PixelFormats = QVector<QMPlay2PixelFormat>;
