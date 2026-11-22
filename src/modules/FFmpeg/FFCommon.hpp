// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#define DecoderName "FFmpeg Decoder"
#define DecoderVkVideoName "FFmpeg Vulkan Decoder"
#define DecoderVAAPIName "FFmpeg VA-API Decoder"
#define DecoderDXVA2Name "FFmpeg DXVA2 Decoder"
#define DecoderD3D11VAName "FFmpeg D3D11VA Decoder"
#define DecoderVTBName "FFmpeg VideoToolbox Decoder"
#define DemuxerName "FFmpeg"
#define VAAPIWriterName "VA-API"
#define VkVideoWriterName "VkVideo"
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
