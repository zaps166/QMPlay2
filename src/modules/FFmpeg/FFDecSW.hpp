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

#include <FFDec.hpp>

#include <QString>
#include <QList>

struct SwsContext;

class FFDecSW : public FFDec
{
public:
	FFDecSW(QMutex &, Module &);
private:
	class BitmapSubBuffer
	{
	public:
		inline BitmapSubBuffer(double pts, double duration) :
			pts(pts), duration(duration)
		{}
		inline BitmapSubBuffer(double pts) :
			x(0), y(0), w(0), h(0),
			pts(pts), duration(0.0)
		{}

		int x, y, w, h;
		double pts, duration;
		QByteArray bitmap;
	};

	~FFDecSW();

	bool set() override;

	QString name() const override;

	void setSupportedPixelFormats(const QMPlay2PixelFormats &pixelFormats) override;

	int  decodeAudio(Packet &encodedPacket, Buffer &decoded, quint8 &channels, quint32 &sampleRate, bool flush) override;
	int  decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurry_up) override;
	bool decodeSubtitle(const Packet &encodedPacket, double pos, QMPlay2OSD *&osd, int w, int h) override;

	bool open(StreamInfo &, VideoWriter *) override;

	/**/

	void setPixelFormat();

	inline void addBitmapSubBuffer(BitmapSubBuffer *buff, double pos);
	bool getFromBitmapSubsBuffer(QMPlay2OSD *&, double pts);
	inline void clearBitmapSubsBuffer();

	int threads, lowres;
	bool respectHurryUP, skipFrames, forceSkipFrames, thread_type_slice;
	int lastFrameW, lastFrameH, lastPixFmt;
	SwsContext *sws_ctx;

	QMPlay2PixelFormats supportedPixelFormats;
	quint8 chromaShiftW, chromaShiftH;
	int desiredPixFmt;
	bool dontConvert;

	QList<BitmapSubBuffer *> bitmapSubsBuffer;
};
