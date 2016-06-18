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

#include <VideoWriter.hpp>

QMPlay2PixelFormats VideoWriter::supportedPixelFormats() const
{
	return QMPlay2PixelFormats()
			<< QMPLAY2_PIX_FMT_YUV420P
	;
}

qint64 VideoWriter::write(const QByteArray &)
{
	return -1;
}

bool VideoWriter::HWAccellInit(int W, int H, const char *codec_name)
{
	Q_UNUSED(W)
	Q_UNUSED(H)
	Q_UNUSED(codec_name)
	return false;
}
bool VideoWriter::HWAccellGetImg(const VideoFrame &videoFrame, void *dest, ImgScaler *yv12ToRGB32) const
{
	Q_UNUSED(videoFrame)
	Q_UNUSED(dest)
	Q_UNUSED(yv12ToRGB32)
	return false;
}
