/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2021  Błażej Szczygieł

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

struct SwsContext;

class FFDecDXVA2 final : public FFDecHWAccel
{
public:
    FFDecDXVA2(Module &module);
    ~FFDecDXVA2();

    bool set() override;

    QString name() const override;

    std::shared_ptr<VideoFilter> hwAccelFilter() const override;

    void downloadVideoFrame(Frame &decoded) override;

    bool open(StreamInfo &streamInfo) override;

private:
    AVBufferRef *m_hwDeviceBufferRef = nullptr;
    std::shared_ptr<VideoFilter> m_filter;
    AVPixelFormat m_pixFmt = AV_PIX_FMT_NONE;
    SwsContext *m_swsCtx = nullptr;
};
