#ifndef FFDECHWACCEL_HPP
#define FFDECHWACCEL_HPP

#include <FFDec.hpp>

class VideoWriter;

class FFDecHWAccel : public FFDec
{
protected:
	FFDecHWAccel(QMutex &mutex);
	virtual ~FFDecHWAccel();

	Writer *HWAccel() const;

	bool hasHWAccel(const char *hwaccelName);

	int decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &, bool flush, unsigned);

	/**/

	VideoWriter *hwAccelWriter;
};

#endif
