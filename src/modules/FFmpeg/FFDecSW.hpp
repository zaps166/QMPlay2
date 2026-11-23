// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <FFmpegHeaders.hpp>

#include <FFDec.hpp>

#ifdef USE_VULKAN
# include <vulkan/VulkanBufferPool.hpp>
#endif

class Subtitle : public AVSubtitle
{
public:
    Subtitle();
    ~Subtitle();

    inline AVSubtitle *av();

    double duration() const;

public:
    double time = 0.0;
    QSize frameSize;
};

/**/

class FFMPEG_EXPORT FFDecSW final : public FFDec
{
public:
    FFDecSW(Module &);

private:
    ~FFDecSW();

    bool set() override;

    QString name() const override;

    QByteArray subtitleHeader() const override;

    void setSupportedPixelFormats(const AVPixelFormats &pixelFormats) override;

    int  decodeAudio(const Packet &encodedPacket, QByteArray &decoded, double &ts, quint8 &channels, quint32 &sampleRate, bool flush) override;
    int  decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurry_up) override;
    bool decodeSubtitle(const QVector<Packet> &encodedPackets, double pos, std::shared_ptr<QMPlay2OSD> &osd, const QSize &size, bool flush) override;
    QList<QByteArray> decodeSubtitle(const Packet &encodedPacket) override;

    bool open(StreamInfo &) override;

    /**/

    void setPixelFormat();

    bool getFromBitmapSubsBuffer(std::shared_ptr<QMPlay2OSD> &osd, double pts);

#ifdef USE_VULKAN
private:
    static int vulkanGetVideoBufferStatic(AVCodecContext *codecCtx, AVFrame *frame, int flags);
    int vulkanGetVideoBuffer(AVCodecContext *codecCtx, AVFrame *frame, int flags);
#endif

private:
    int threads, lowres;
    bool respectHurryUP, skipFrames, forceSkipFrames, thread_type_slice;
    int lastFrameW, lastFrameH, lastPixFmt;
    int m_teletextPage = 0;
    bool m_teletextTransparent = false;
    SwsContext *sws_ctx;

    const AVPixFmtDescriptor *m_origPixDesc = nullptr;
    AVPixelFormat m_desiredPixFmt = AV_PIX_FMT_NONE;
    bool m_dontConvert = false;

    std::deque<Subtitle> m_subtitles;

    double m_lastTs = qQNaN();

#ifdef USE_VULKAN
    std::shared_ptr<QmVk::BufferPool> m_vkBufferPool;
    bool m_disableZeroCopy = false;
    bool m_defaultGetBufferUsed = false;
    decltype(AVFrame::linesize) m_vkLineSizes = {};
#endif
};
