#ifndef FFCOMMON_H
#define FFCOMMON_H

#include <QString>

#define QMPLAY2_NOPTS_VALUE (int64_t)AV_NOPTS_VALUE

#define DecoderName "FFmpeg Decoder"
#define DecoderVAAPIName "FFmpeg VA-API Decoder"
#define DecoderVDPAUName "FFmpeg VDPAU Decoder"
#define DecoderVDPAU_NWName "FFmpeg VDPAU Decoder (no writer)"
#define DemuxerName "FFmpeg"
#define VAAPIWriterName "VA-API"
#define VDPAUWriterName "VDPAU"
#define FFReaderName "FFmpeg Reader"

struct AVDictionary;
struct AVPacket;
class VideoFrame;

namespace FFCommon
{
	QString prepareUrl(QString url, AVDictionary *&options);

	int getField(const VideoFrame &videoFrame, int deinterlace, int fullFrame, int topField, int bottomField);

	AVPacket *createAVPacket();
	void freeAVPacket(AVPacket *&packet);
}

#endif
