#ifndef FFDECHWACCEL_HPP
#define FFDECHWACCEL_HPP

#include <FFDec.hpp>

class VideoWriter;

class FFDecHWAccel : public FFDec
{
protected:
	FFDecHWAccel( QMutex &mutex );
	virtual ~FFDecHWAccel();

	Writer *HWAccel() const;

	bool canUseHWAccel( const StreamInfo *streamInfo );
	bool hasHWAccel( const char *name );

	virtual int decode( Packet &encodedPacket, QByteArray &decoded, bool flush, unsigned );

	/**/

	VideoWriter *hwAccelWriter;
};

#endif
