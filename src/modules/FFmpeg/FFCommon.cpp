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

#include <FFCommon.hpp>

#include <QMPlay2Core.hpp>
#include <Version.hpp>

extern "C"
{
	#include <libavformat/version.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/dict.h>
#ifdef QMPlay2_VDPAU
	#include <libavcodec/vdpau.h>
#endif
}

AVPacket *FFCommon::createAVPacket()
{
	AVPacket *packet;
#if LIBAVCODEC_VERSION_MAJOR >= 57
	packet = av_packet_alloc();
#else
	packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	av_init_packet(packet);
#endif
	return packet;
}
void FFCommon::freeAVPacket(AVPacket *&packet)
{
#if LIBAVCODEC_VERSION_MAJOR >= 57
	av_packet_free(&packet);
#else
	if (packet)
		av_packet_unref(packet);
	av_freep(&packet);
#endif
}

#ifdef QMPlay2_VDPAU
AVVDPAUContext *FFCommon::allocAVVDPAUContext(AVCodecContext *codecCtx)
{
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(56, 13, 100) // FFmpeg 2.5.0
	// Since FFmpeg 3.3 we must not use "av_vdpau_alloc_context()" or "AVVDPAUContext" structure size
	// for allocating "AVCodecContext::hwaccel_context", because internally it always uses field from
	// different internal structure which is larger. Using different struct inside FFmpeg was provided
	// earlier, but at least it was binary compatible with "AVVDPAUContext".
	// Since https://github.com/FFmpeg/FFmpeg/commit/7e4ba776a2240d40124d5540ea6b2118fa2fe26a it is no
	// longer binary compatible because of initialization of "reset" field of internal structure which
	// leads to buffer overflow. It causes random and weird crashes.
	if (av_vdpau_bind_context(codecCtx, 0, nullptr, 0) == 0)
		return (AVVDPAUContext *)codecCtx->hwaccel_context;
	return nullptr;
#else
	return av_vdpau_alloc_context();
#endif
}
#endif
