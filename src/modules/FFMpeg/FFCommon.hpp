#ifndef FFCOMMON_H
#define FFCOMMON_H

#include <QString>

#define QMPLAY2_NOPTS_VALUE ( int64_t )AV_NOPTS_VALUE

#define DecoderName "FFMpeg Decoder"
#define DecoderVAAPIName "FFMpeg VAAPI Decoder"
#define DecoderVDPAUName "FFMpeg VDPAU Decoder"
#define DecoderVDPAU_NWName "FFMpeg VDPAU Decoder (no writer)"
#define DemuxerName "FFMpeg Demuxer"
#define VAApiWriterName "VAApi Writer"
#define VDPAUWriterName "VDPAU Writer"
#define FFReaderName "FFMpeg Reader"

struct AVDictionary;
class VideoFrame;

namespace FFCommon
{
	QString prepareUrl( QString url, AVDictionary *&options, bool *isLocal = NULL );

	int getField( const VideoFrame *videoFrame, int deinterlace, int fullFrame, int topField, int bottomField );
}

#endif
