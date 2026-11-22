// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <FFDecHWAccel.hpp>

#include <d3d11.h>

#include <deque>

class D3D11VAVulkan;
struct SwsContext;

class FFMPEG_EXPORT FFDecD3D11VA final : public FFDecHWAccel
{
public:
    FFDecD3D11VA(Module &module);
    ~FFDecD3D11VA();

    bool set() override;

    QString name() const override;

    int decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurryUp) override;

    bool open(StreamInfo &streamInfo) override;

private:
    bool comparePrimaryDevice() const;

private:
    AVBufferRef *m_hwDeviceBufferRef = nullptr;

    std::shared_ptr<D3D11VAVulkan> m_d3d11vaVulkan;
    bool m_zeroCopyAllowed = false;

    std::deque<Frame> m_frames;
};
