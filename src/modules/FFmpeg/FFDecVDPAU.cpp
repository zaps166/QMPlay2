/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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
#include <HWAccelHelper.hpp>
#include <VDPAUWriter.hpp>
#include <FFCommon.hpp>

#include <StreamInfo.hpp>

extern "C"
{
	#include <libavformat/avformat.h>
	#include <libavutil/pixdesc.h>
	#include <libavcodec/vdpau.h>
}

FFDecVDPAU::FFDecVDPAU(QMutex &avcodec_mutex, Module &module) :
	FFDecHWAccel(avcodec_mutex)
{
	SetModule(module);
}
FFDecVDPAU::~FFDecVDPAU()
{
	if (codecIsOpen)
		avcodec_flush_buffers(codec_ctx);
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
	if (pix_fmt == AV_PIX_FMT_YUV420P || pix_fmt == AV_PIX_FMT_YUVJ420P)
	{
		AVCodec *codec = init(streamInfo);
		if (codec && hasHWAccel("vdpau"))
		{
			if (writer && writer->name() != VDPAUWriterName)
				writer = nullptr;
			VDPAUWriter *vdpauWriter = writer ? (VDPAUWriter *)writer : new VDPAUWriter(getModule());
			if ((writer || vdpauWriter->open()) && vdpauWriter->hwAccelInit(codec_ctx->width, codec_ctx->height, avcodec_get_name(codec_ctx->codec_id)))
			{
				AVVDPAUContext *vdpauCtx = (AVVDPAUContext *)av_mallocz(sizeof(AVVDPAUContext));
				vdpauCtx->decoder = vdpauWriter->getVdpDecoder();
				vdpauCtx->render  = vdpauWriter->getVdpDecoderRender();

				new HWAccelHelper(codec_ctx, AV_PIX_FMT_VDPAU, vdpauCtx, vdpauWriter->getSurfacesQueue());

				if (pix_fmt == AV_PIX_FMT_YUVJ420P && avcodec_version() >= 0x383C64)
					codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P; //Force full color range

				if (openCodec(codec))
				{
					time_base = streamInfo.getTimeBase();
					m_hwAccelWriter = vdpauWriter;
					return true;
				}
			}
			else if (!writer)
				delete vdpauWriter;
		}
	}
	return false;
}
