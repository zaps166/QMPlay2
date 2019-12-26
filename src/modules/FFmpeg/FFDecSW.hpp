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

#include <FFDec.hpp>

#include <vector>
#include <deque>

struct SwsContext;

class FFDecSW final : public FFDec
{
public:
    FFDecSW(Module &);

private:
    struct Subtitle
    {
        struct Rect
        {
            int x, y, w, h;
            QByteArray data;
        };

        double pts, duration;
        std::vector<Rect> rects;
    };

private:
    ~FFDecSW();

    bool set() override;

    QString name() const override;

    void setSupportedPixelFormats(const AVPixelFormats &pixelFormats) override;

    int  decodeAudio(Packet &encodedPacket, Buffer &decoded, quint8 &channels, quint32 &sampleRate, bool flush) override;
    int  decodeVideo(Packet &encodedPacket, Frame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurry_up) override;
    bool decodeSubtitle(const Packet &encodedPacket, double pos, QMPlay2OSD *&osd, const QSize &size, bool flush) override;

    bool open(StreamInfo &, VideoWriter *) override;

    /**/

    void setPixelFormat();

    bool getFromBitmapSubsBuffer(QMPlay2OSD *&, double pts);

private:
    int threads, lowres;
    bool respectHurryUP, skipFrames, forceSkipFrames, thread_type_slice;
    int lastFrameW, lastFrameH, lastPixFmt;
    SwsContext *sws_ctx;

    AVPixelFormats supportedPixelFormats;
    AVPixelFormat desiredPixFmt = AV_PIX_FMT_NONE;
    bool dontConvert;

    std::deque<Subtitle> m_subtitles;
};
