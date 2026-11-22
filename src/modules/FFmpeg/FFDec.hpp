// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Decoder.hpp>

#include <QString>
#include <QList>

#if defined(FFMPEG_MODULE)
    #define FFMPEG_EXPORT Q_DECL_EXPORT
#else
    #define FFMPEG_EXPORT Q_DECL_IMPORT
#endif

#ifdef USE_VULKAN
namespace QmVk {
class ImagePool;
}
#endif

struct AVCodecContext;
struct AVPacket;
struct AVCodec;
struct AVFrame;

class FFMPEG_EXPORT FFDec : public Decoder
{
protected:
    FFDec();
    ~FFDec();

    void setSupportedPixelFormats(const AVPixelFormats &pixelFormats) override;

    int pendingFrames() const override;

    /**/

    void destroyDecoder();

    void clearFrames();

    virtual AVCodec *init(StreamInfo &streamInfo);
    bool openCodec(AVCodec *codec);

    void decodeFirstStep(const Packet &encodedPacket, bool flush);
    int decodeStep(bool &frameFinished);
    void decodeLastStep(const Packet &encodedPacket, Frame &decoded, bool frameFinished);

    bool maybeTakeFrame();

    AVCodecContext *codec_ctx;
    AVPacket *packet;
    AVFrame *frame;
    QList<AVFrame *> m_frames;
    AVRational m_timeBase;
    AVDictionary *m_options = nullptr;

    bool m_libError = false;

    AVPixelFormats m_supportedPixelFormats;

#ifdef USE_VULKAN
    std::shared_ptr<QmVk::ImagePool> m_vkImagePool;
#endif
};
