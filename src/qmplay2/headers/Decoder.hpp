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

#include <ModuleCommon.hpp>
#include <PixelFormats.hpp>
#include <VideoFrame.hpp>
#include <Packet.hpp>

#include <QByteArray>
#include <QString>

class QMPlay2OSD;
class VideoWriter;
class StreamInfo;
class LibASS;

class QMPLAY2SHAREDLIB_EXPORT Decoder : public ModuleCommon
{
public:
	static Decoder *create(StreamInfo &streamInfo, VideoWriter *writer = nullptr, const QStringList &modNames = {}, QString *modNameOutput = nullptr);

	virtual ~Decoder() = default;

	virtual QString name() const = 0;

	virtual VideoWriter *HWAccel() const;

	virtual void setSupportedPixelFormats(const QMPlay2PixelFormats &pixelFormats);

	/*
	 * hurry_up ==  0 -> no frame skipping, normal quality
	 * hurry_up >=  1 -> faster decoding, lower image quality, frame skipping during decode
	 * hurry_up == ~0 -> much faster decoding, no frame copying
	*/
	virtual int decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurry_up);
	virtual int decodeAudio(Packet &encodedPacket, Buffer &decoded, quint8 &channels, quint32 &sampleRate, bool flush = false);
	virtual bool decodeSubtitle(const Packet &encodedPacket, double pos, QMPlay2OSD *&osd, int w, int h);

	virtual bool hasCriticalError() const;

private:
	virtual bool open(StreamInfo &streamInfo, VideoWriter *writer = nullptr) = 0;
};
