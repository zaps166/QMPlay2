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

#include <FFCommon.hpp>

#include <Version.hpp>

extern "C"
{
	#include <libavformat/version.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/dict.h>
}

QString FFCommon::prepareUrl(QString url, AVDictionary *&options)
{
	if (url.startsWith("file://"))
		url.remove(0, 7);
	else
	{
		if (url.startsWith("mms:"))
			url.insert(3, 'h');
#if LIBAVFORMAT_VERSION_MAJOR <= 55
		if (url.startsWith("http"))
			av_dict_set(&options, "icy", "1", 0);
#endif
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 56, 100)
		av_dict_set(&options, "user_agent", QMPlay2UserAgent, 0);
#else
		av_dict_set(&options, "user-agent", QMPlay2UserAgent, 0);
#endif
	}
	return url;
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
