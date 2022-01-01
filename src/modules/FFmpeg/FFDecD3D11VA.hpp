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

#include <FFDecHWAccel.hpp>

#include <d3d11.h>

#include <deque>

class D3D11VAVulkan;
struct SwsContext;

class FFDecD3D11VA final : public FFDecHWAccel
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
