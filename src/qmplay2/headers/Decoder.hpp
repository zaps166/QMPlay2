#ifndef DECODER_HPP
#define DECODER_HPP

#include <ModuleCommon.hpp>
#include <PixelFormats.hpp>
#include <VideoFrame.hpp>
#include <Packet.hpp>

#include <QByteArray>
#include <QString>

class QMPlay2_OSD;
class StreamInfo;
class Writer;
class LibASS;

class Decoder : public ModuleCommon
{
public:
	static Decoder *create(StreamInfo &streamInfo, Writer *writer = NULL, const QStringList &modNames = QStringList());

	virtual QString name() const = 0;

	virtual Writer *HWAccel() const;

	virtual void setSupportedPixelFormats(const QMPlay2PixelFormats &pixelFormats);

	/*
	 * hurry_up ==  0 -> no frame skipping, normal quality
	 * hurry_up >=  1 -> faster decoding, lower image quality, frame skipping during decode
	 * hurry_up == ~0 -> much faster decoding, no frame copying
	*/
	virtual int decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurry_up);
	virtual int decodeAudio(Packet &encodedPacket, Buffer &decoded, bool flush = false);
	virtual bool decodeSubtitle(const Packet &encodedPacket, double pos, QMPlay2_OSD *&osd, int w, int h);

	virtual ~Decoder();
private:
	virtual bool open(StreamInfo &streamInfo, Writer *writer = NULL) = 0;
};

#endif
