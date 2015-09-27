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

	/**/

	AVCodec *init( StreamInfo *streamInfo );
	bool openCodec( AVCodec *codec );

	void decodeFirstStep( AVPacket &packet, const Packet &encodedPacket, bool flush );
	void decodeLastStep( Packet &encodedPacket, AVFrame *frame );

	AVCodecContext *codec_ctx;
	AVFrame *frame;
	double time_base;
	bool codecIsOpen;

	QMutex &avcodec_mutex;
};

#endif
