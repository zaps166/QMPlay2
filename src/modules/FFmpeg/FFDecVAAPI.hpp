/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

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

struct SwsContext;

class FFDecVAAPI final : public FFDecHWAccel
{
public:
    FFDecVAAPI(Module &);
    ~FFDecVAAPI();

    bool set() override;

    QString name() const override;

    int decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurryUp) override;
    void downloadVideoFrame(VideoFrame &decoded) override;

    bool open(StreamInfo &, VideoWriter *) override;

private:
    bool m_useOpenGL;
    Qt::CheckState m_copyVideo;
    VAProcDeinterlacingType m_vppDeintType;
    VAAPI *m_vaapi;
    SwsContext *m_swsCtx;
};
