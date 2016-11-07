/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <FFDecVDPAU.hpp>
#include <VDPAUWriter.hpp>
#include <FFCommon.hpp>

#include <StreamInfo.hpp>

extern "C"
{
	#include <libavformat/avformat.h>
	#include <libavutil/pixdesc.h>
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
bool FFDecVDPAU::open(StreamInfo &streamInfo, VideoWriter *writer)
{
	/*
	 * AV_PIX_FMT_YUVJ420P doesn't work on FFmpeg/VDPAU, but works on VAAPI over VDPAU.
	 * I tested FFmpeg 2.7 and it works, but crashes (assertion failed) in FFmpeg >= 2.8.
	*/
	const AVPixelFormat pix_fmt = av_get_pix_fmt(streamInfo.format);
	const bool canUseYUVJ420P = avcodec_version() < 0x383C64;
	if (pix_fmt == AV_PIX_FMT_YUV420P || (canUseYUVJ420P && pix_fmt == AV_PIX_FMT_YUVJ420P))
	{
		AVCodec *codec = init(streamInfo);
		if (codec && HWAccelHelper::hasHWAccel(codec_ctx, "vdpau"))
		{
			if (writer && writer->name() != VDPAUWriterName)
				writer = NULL;
			VDPAUWriter *vdpauWriter = writer ? (VDPAUWriter *)writer : new VDPAUWriter(getModule());
			if ((writer || vdpauWriter->open()) && vdpauWriter->hwAccellInit(codec_ctx->width, codec_ctx->height, avcodec_get_name(codec_ctx->codec_id)))
			{
				codec_ctx->hwaccel_context = av_mallocz(sizeof(AVVDPAUContext));
				((AVVDPAUContext *)codec_ctx->hwaccel_context)->decoder = vdpauWriter->getVdpDecoder();
				((AVVDPAUContext *)codec_ctx->hwaccel_context)->render  = vdpauWriter->getVdpDecoderRender();
				codec_ctx->thread_count   = 1;
				codec_ctx->get_buffer2    = HWAccelHelper::get_buffer;
				codec_ctx->get_format     = get_format;
				codec_ctx->slice_flags    = SLICE_FLAG_CODED_ORDER | SLICE_FLAG_ALLOW_FIELD;
				codec_ctx->opaque         = (HWAccelHelper *)vdpauWriter;
				if (openCodec(codec))
				{
					time_base = streamInfo.getTimeBase();
					hwAccelWriter = (VideoWriter *)vdpauWriter;
					return true;
				}
			}
			else if (!writer)
				delete vdpauWriter;
		}
	}
	return false;
}
