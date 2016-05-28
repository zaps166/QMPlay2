#include <FFDecVAAPI.hpp>
#include <VAAPIWriter.hpp>
#include <FFCommon.hpp>

#include <StreamInfo.hpp>

extern "C"
{
	#include <libavformat/avformat.h>
	#include <libavutil/pixdesc.h>
	#include <libavcodec/vaapi.h>
}

static AVPixelFormat get_format(AVCodecContext *, const AVPixelFormat *)
{
	return AV_PIX_FMT_VAAPI_VLD;
}

/**/

FFDecVAAPI::FFDecVAAPI(QMutex &avcodec_mutex, Module &module) :
	FFDecHWAccel(avcodec_mutex)
{
	SetModule(module);
}

bool FFDecVAAPI::set()
{
	return sets().getBool("DecoderVAAPIEnabled");
}

QString FFDecVAAPI::name() const
{
	return "FFmpeg/" VAAPIWriterName;
}

bool FFDecVAAPI::open(StreamInfo &streamInfo, Writer *writer)
{
	const AVPixelFormat pix_fmt = av_get_pix_fmt(streamInfo.format);
	if ((pix_fmt == AV_PIX_FMT_YUV420P || pix_fmt == AV_PIX_FMT_YUVJ420P))
	{
		AVCodec *codec = init(streamInfo);
		if (codec && hasHWAccel("vaapi"))
		{
			if (writer && writer->name() != VAAPIWriterName)
				writer = NULL;
			hwAccelWriter = writer ? (VideoWriter *)writer : new VAAPIWriter(getModule());
			if ((writer || hwAccelWriter->open()) && hwAccelWriter->HWAccellInit(codec_ctx->width, codec_ctx->height, avcodec_get_name(codec_ctx->codec_id)))
			{
				codec_ctx->hwaccel_context = av_mallocz(sizeof(vaapi_context));
				((vaapi_context *)codec_ctx->hwaccel_context)->display    = ((VAAPIWriter *)hwAccelWriter)->getVADisplay();
				((vaapi_context *)codec_ctx->hwaccel_context)->context_id = ((VAAPIWriter *)hwAccelWriter)->getVAContext();
				((vaapi_context *)codec_ctx->hwaccel_context)->config_id  = ((VAAPIWriter *)hwAccelWriter)->getVAConfig();
				codec_ctx->thread_count   = 1;
				codec_ctx->get_buffer2    = HWAccelHelper::get_buffer;
				codec_ctx->get_format     = get_format;
				codec_ctx->slice_flags    = SLICE_FLAG_CODED_ORDER | SLICE_FLAG_ALLOW_FIELD;
				codec_ctx->opaque         = dynamic_cast<HWAccelHelper *>(hwAccelWriter);
				if (openCodec(codec))
				{
					time_base = streamInfo.getTimeBase();
					return true;
				}
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
