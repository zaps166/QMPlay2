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

#include <PixelFormats.hpp>
#include <Writer.hpp>

class HWAccelInterface;
class QMPlay2OSD;
class VideoFrame;
class ImgScaler;

class QMPLAY2SHAREDLIB_EXPORT VideoWriter : public Writer
{
public:
	static VideoWriter *createOpenGL2(HWAccelInterface *hwAccelInterface);

	VideoWriter();
	virtual ~VideoWriter();

	virtual QMPlay2PixelFormats supportedPixelFormats() const;

	virtual bool hwAccelError() const; // Must be thread-safe

	qint64 write(const QByteArray &) override final;

	virtual void writeVideo(const VideoFrame &videoFrame) = 0;
	virtual void writeOSD(const QList<const QMPlay2OSD *> &osd) = 0;

	virtual void setHWAccelInterface(HWAccelInterface *hwAccelInterface);
	inline HWAccelInterface *getHWAccelInterface() const
	{
		return m_hwAccelInterface;
	}

	virtual bool hwAccelGetImg(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) const;

	virtual bool open() override = 0;

protected:
	HWAccelInterface *m_hwAccelInterface;
};
