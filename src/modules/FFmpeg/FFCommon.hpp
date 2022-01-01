/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#define DecoderName "FFmpeg Decoder"
#define DecoderVAAPIName "FFmpeg VA-API Decoder"
#define DecoderVDPAUName "FFmpeg VDPAU Decoder"
#define DecoderVDPAU_NWName DecoderVDPAUName " (no output)"
#define DecoderDXVA2Name "FFmpeg DXVA2 Decoder"
#define DecoderD3D11VAName "FFmpeg D3D11VA Decoder"
#define DecoderVTBName "FFmpeg VideoToolbox Decoder"
#define DemuxerName "FFmpeg"
#define VAAPIWriterName "VA-API"
#define VDPAUWriterName "VDPAU"
#define FFReaderName "FFmpeg Reader"

#ifdef FIND_HWACCEL_DRIVERS_PATH
    class QByteArray;
    class QString;
#endif

namespace FFCommon
{
#ifdef FIND_HWACCEL_DRIVERS_PATH
    void setDriversPath(const QString &dirName, const QByteArray &envVar);
#endif
}
