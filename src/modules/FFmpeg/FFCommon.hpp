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

#pragma once

#define QMPLAY2_NOPTS_VALUE ((qint64)AV_NOPTS_VALUE)

#define DecoderName "FFmpeg Decoder"
#define DecoderVAAPIName "FFmpeg VA-API Decoder"
#define DecoderVDPAUName "FFmpeg VDPAU Decoder"
#define DecoderVDPAU_NWName DecoderVDPAUName " (no output)"
#define DecoderDXVA2Name "FFmpeg DXVA2 Decoder"
#define DecoderVTBName "FFmpeg VideoToolbox Decoder"
#define DemuxerName "FFmpeg"
#define VAAPIWriterName "VA-API"
#define VDPAUWriterName "VDPAU"
#define FFReaderName "FFmpeg Reader"

#ifdef QMPlay2_VDPAU
	struct AVVDPAUContext;
	struct AVCodecContext;
#endif
struct AVDictionary;
struct AVPacket;
class VideoFrame;

namespace FFCommon
{
	AVPacket *createAVPacket();
	void freeAVPacket(AVPacket *&packet);

#ifdef QMPlay2_VDPAU
	AVVDPAUContext *allocAVVDPAUContext(AVCodecContext *codecCtx);
#endif
}
