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
#include <VAAPI.hpp>

#ifdef USE_VULKAN
class VAAPIVulkan;
#endif
struct SwsContext;

class FFDecVAAPI final : public FFDecHWAccel
{
public:
    FFDecVAAPI(Module &);
    ~FFDecVAAPI();

    bool set() override;

    QString name() const override;

    std::shared_ptr<VideoFilter> hwAccelFilter() const override;

    int decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurryUp) override;
    void downloadVideoFrame(Frame &decoded) override;

    bool open(StreamInfo &streamInfo) override;

private:
    VAProcDeinterlacingType m_vppDeintType = VAProcDeinterlacingNone;
    std::shared_ptr<VAAPI> m_vaapi;
    std::shared_ptr<VideoFilter> m_filter;
#ifdef USE_VULKAN
    std::shared_ptr<VAAPIVulkan> m_vaapiVulkan;
#endif
    SwsContext *m_swsCtx = nullptr;
};
