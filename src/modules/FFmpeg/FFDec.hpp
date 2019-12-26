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

#include <Decoder.hpp>

#include <QString>
#include <QList>

struct AVCodecContext;
struct AVPacket;
struct AVCodec;
struct AVFrame;

class FFDec : public Decoder
{
protected:
    FFDec();
    ~FFDec();

    int pendingFrames() const override;

    /**/

    void destroyDecoder();

    void clearFrames();

    AVCodec *init(StreamInfo &streamInfo);
    bool openCodec(AVCodec *codec);

    void decodeFirstStep(const Packet &encodedPacket, bool flush);
    int decodeStep(bool &frameFinished);

    bool maybeTakeFrame();

    AVCodecContext *codec_ctx;
    AVPacket *packet;
    AVFrame *frame;
    QList<AVFrame *> m_frames;
    double time_base;
    bool codecIsOpen;
};
