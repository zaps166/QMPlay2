#ifndef DECODER_HPP
#define DECODER_HPP

#include <ModuleCommon.hpp>
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
	static Decoder *create(StreamInfo *streamInfo, Writer *writer = NULL, const QStringList &modNames = QStringList());

	virtual QString name() const = 0;

	virtual Writer *HWAccel() const {return NULL;}

	/*
	 * hurry_up ==  0 -> no frame skipping, normal quality
	 * hurry_up >=  1 -> faster decoding, lower image quality, frame skipping during decode
	 * hurry_up == ~0 -> much faster decoding, no frame copying
	*/
	virtual int decodeVideo(Packet &encodedPacket, VideoFrame &decoded, bool flush = false, unsigned hurry_up = 0)
	{
		Q_UNUSED(encodedPacket)
		Q_UNUSED(decoded)
		Q_UNUSED(flush)
		Q_UNUSED(hurry_up)
		return 0;
	}
	virtual int decodeAudio(Packet &encodedPacket, Buffer &decoded, bool flush = false)
	{
		Q_UNUSED(flush)
		return (decoded = encodedPacket).size();
	}
	virtual bool decodeSubtitle(const Packet &encodedPacket, double pos, QMPlay2_OSD *&osd, int w, int h)
	{
		Q_UNUSED(encodedPacket)
		Q_UNUSED(pos)
		Q_UNUSED(osd)
		Q_UNUSED(w)
		Q_UNUSED(h)
		return false;
	}

	virtual ~Decoder() {}
private:
	virtual bool open(StreamInfo *streamInfo, Writer *writer = NULL) = 0;
protected:
	StreamInfo *streamInfo;
};

#endif
