// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <FFDecHWAccel.hpp>
#include <VAAPI.hpp>

#ifdef USE_OPENGL
class VAAPIOpenGL;
#endif
#ifdef USE_VULKAN
class VAAPIVulkan;
#endif

class FFMPEG_EXPORT FFDecVAAPI final : public FFDecHWAccel
{
public:
    FFDecVAAPI(Module &);
    ~FFDecVAAPI();

    bool set() override;

    QString name() const override;

    std::shared_ptr<VideoFilter> hwAccelFilter() const override;

    int decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurryUp) override;

    bool open(StreamInfo &streamInfo) override;

private:
    void maybeClearHwSurfaces();

private:
    VAProcDeinterlacingType m_vppDeintType = VAProcDeinterlacingNone;
    std::shared_ptr<VAAPI> m_vaapi;
    std::shared_ptr<VideoFilter> m_filter;
#ifdef USE_OPENGL
    std::shared_ptr<VAAPIOpenGL> m_vaapiOpenGL;
#endif
#ifdef USE_VULKAN
    std::shared_ptr<VAAPIVulkan> m_vaapiVulkan;
#endif
};
