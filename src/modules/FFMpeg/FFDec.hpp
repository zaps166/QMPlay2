#ifndef FFDEC_HPP
#define FFDEC_HPP

#include <Decoder.hpp>

#include <QString>
#include <QList>

class QMutex;
struct AVCodecContext;
struct AVPacket;
struct AVCodec;
struct AVFrame;

class FFDec : public Decoder
{
protected:
	FFDec( QMutex & );
	virtual ~FFDec();

	bool aspectRatioChanged() const;

	/**/

	AVCodec *init( StreamInfo *streamInfo );
	bool openCodec( AVCodec *codec );

	void decodeFirstStep( AVPacket &packet, const Packet &encodedPacket, bool flush );

	AVCodecContext *codec_ctx;
	AVFrame *frame;
	double time_base;
	mutable int last_aspect_ratio;
	bool codecIsOpen;

	QMutex &avcodec_mutex;
};

#endif
