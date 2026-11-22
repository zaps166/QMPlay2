// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <FFDec.hpp>

class FFMPEG_EXPORT FFDecHWAccel : public FFDec
{
protected:
    FFDecHWAccel();
    ~FFDecHWAccel();

    static bool hasHWAccel(const char *hwaccelName);

    AVCodec *init(StreamInfo &streamInfo) override;

    bool hasHWDecContext() const override final;

    int decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurryUp) override;

    bool hasCriticalError() const override;

protected:
    bool m_hasCriticalError = false;
    bool m_hasHWDecContext = false;
    SwsContext *m_swsCtx = nullptr;
};
