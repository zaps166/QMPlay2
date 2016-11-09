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

FFDecVAAPI::FFDecVAAPI(QMutex &avcodec_mutex, Module &module) :
	FFDecHWAccel(avcodec_mutex)
{
	SetModule(module);
}
FFDecVAAPI::~FFDecVAAPI()
{
	if (codecIsOpen)
		avcodec_flush_buffers(codec_ctx);
}

bool FFDecVAAPI::set()
{
	return sets().getBool("DecoderVAAPIEnabled");
}

QString FFDecVAAPI::name() const
{
	return "FFmpeg/" VAAPIWriterName;
}

bool FFDecVAAPI::open(StreamInfo &streamInfo, VideoWriter *writer)
{
	const AVPixelFormat pix_fmt = av_get_pix_fmt(streamInfo.format);
	if ((pix_fmt == AV_PIX_FMT_YUV420P || pix_fmt == AV_PIX_FMT_YUVJ420P))
	{
		AVCodec *codec = init(streamInfo);
		if (codec && hasHWAccel("vaapi"))
		{
			if (writer && writer->name() != VAAPIWriterName)
				writer = NULL;
			VAAPIWriter *vaapiWriter = writer ? (VAAPIWriter *)writer : new VAAPIWriter(getModule());
			if ((writer || vaapiWriter->open()) && vaapiWriter->HWAccelInit(codec_ctx->width, codec_ctx->height, avcodec_get_name(codec_ctx->codec_id)))
			{
				vaapi_context *vaapiCtx = (vaapi_context *)av_mallocz(sizeof(vaapi_context));
				vaapiCtx->display    = vaapiWriter->getVADisplay();
				vaapiCtx->context_id = vaapiWriter->getVAContext();
				vaapiCtx->config_id  = vaapiWriter->getVAConfig();

				new HWAccelHelper(codec_ctx, AV_PIX_FMT_VAAPI_VLD, vaapiCtx, vaapiWriter->getSurfacesQueue());

				if (openCodec(codec))
				{
					time_base = streamInfo.getTimeBase();
					m_hwAccelWriter = vaapiWriter;
					return true;
				}
			}
			else if (!writer)
				delete vaapiWriter;
		}
	}
	return false;
}
