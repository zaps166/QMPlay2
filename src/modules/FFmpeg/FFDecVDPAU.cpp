#include <FFDecVDPAU.hpp>
#include <VDPAUWriter.hpp>
#include <FFCommon.hpp>

#include <StreamInfo.hpp>

extern "C"
{
	#include <libavformat/avformat.h>
	#include <libavcodec/vdpau.h>
}

static AVPixelFormat get_format(AVCodecContext *, const AVPixelFormat *)
{
	return AV_PIX_FMT_VDPAU;
}

/**/

FFDecVDPAU::FFDecVDPAU(QMutex &avcodec_mutex, Module &module) :
	FFDecHWAccel(avcodec_mutex)
{
	SetModule(module);
}

bool FFDecVDPAU::set()
{
	return sets().getBool("DecoderVDPAUEnabled");
}

QString FFDecVDPAU::name() const
{
	return "FFmpeg/" VDPAUWriterName;
}
bool FFDecVDPAU::open(StreamInfo *streamInfo, Writer *writer)
{
	/*
	 * AV_PIX_FMT_YUVJ420P doesn't work on FFmpeg/VDPAU, but works on VAAPI over VDPAU.
	 * I tested FFmpeg 2.7 and it works, but crashes (assertion failed) in FFmpeg >= 2.8.
	*/
	const bool canUseYUVJ420P = avcodec_version() < 0x383C64;
	if (streamInfo->img_fmt == AV_PIX_FMT_YUV420P || (canUseYUVJ420P && streamInfo->img_fmt == AV_PIX_FMT_YUVJ420P))
	{
		AVCodec *codec = init(streamInfo);
		if (codec && hasHWAccel("vdpau"))
		{
			if (writer && writer->name() != VDPAUWriterName)
				writer = NULL;
			hwAccelWriter = writer ? (VideoWriter *)writer : new VDPAUWriter(getModule());
			if ((writer || hwAccelWriter->open()) && hwAccelWriter->HWAccellInit(codec_ctx->width, codec_ctx->height, avcodec_get_name(codec_ctx->codec_id)))
			{
				codec_ctx->hwaccel_context = av_mallocz(sizeof(AVVDPAUContext));
				((AVVDPAUContext *)codec_ctx->hwaccel_context)->decoder = ((VDPAUWriter *)hwAccelWriter)->getVdpDecoder();
				((AVVDPAUContext *)codec_ctx->hwaccel_context)->render  = ((VDPAUWriter *)hwAccelWriter)->getVdpDecoderRender();
				codec_ctx->thread_count   = 1;
				codec_ctx->get_buffer2    = HWAccelHelper::get_buffer;
				codec_ctx->get_format     = get_format;
				codec_ctx->slice_flags    = SLICE_FLAG_CODED_ORDER | SLICE_FLAG_ALLOW_FIELD;
				codec_ctx->opaque         = dynamic_cast<HWAccelHelper *>(hwAccelWriter);
				if (openCodec(codec))
					return true;
			}
			else
			{
				if (!writer)
					delete hwAccelWriter;
				hwAccelWriter = NULL;
			}
		}
	}
	return false;
}
