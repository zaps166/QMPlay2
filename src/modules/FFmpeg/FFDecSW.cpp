/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2025  Błażej Szczygieł

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

#include <FFDecSW.hpp>
#include <FFCommon.hpp>

#include <QMPlay2OSD.hpp>
#include <Frame.hpp>
#include <StreamInfo.hpp>
#include <Functions.hpp>

#ifdef USE_VULKAN
#   include "../qmvk/MemoryPropertyFlags.hpp"
#   include "../qmvk/PhysicalDevice.hpp"
#   include "../qmvk/Device.hpp"
#   include "../qmvk/Image.hpp"
#   include "../qmvk/Buffer.hpp"
#   include "../qmvk/BufferView.hpp"

#   include <vulkan/VulkanInstance.hpp>
#   include <vulkan/VulkanImagePool.hpp>
#   include <vulkan/VulkanBufferPool.hpp>
#endif

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/pixdesc.h>
}

using namespace std;

Subtitle::Subtitle()
{
    memset(av(), 0, sizeof(AVSubtitle));
}
Subtitle::~Subtitle()
{
    avsubtitle_free(av());
}

inline AVSubtitle *Subtitle::av()
{
    return static_cast<AVSubtitle *>(this);
}

double Subtitle::duration() const
{
    return (end_display_time != static_cast<uint32_t>(-1) && end_display_time != start_display_time)
        ? (end_display_time - start_display_time) / 1000.0
        : -1.0
    ;
}

/**/

FFDecSW::FFDecSW(Module &module) :
    threads(0), lowres(0),
    thread_type_slice(false),
    lastFrameW(-1), lastFrameH(-1),
    sws_ctx(nullptr)
{
    SetModule(module);
}
FFDecSW::~FFDecSW()
{
    sws_freeContext(sws_ctx);
}

bool FFDecSW::set()
{
    bool restartPlaying = false;

    if ((respectHurryUP = sets().getBool("HurryUP")))
    {
        if ((skipFrames = sets().getBool("SkipFrames")))
            forceSkipFrames = sets().getBool("ForceSkipFrames");
        else
            forceSkipFrames = false;
    }
    else
        skipFrames = forceSkipFrames = false;

    if (lowres != sets().getInt("LowresValue"))
    {
        lowres = sets().getInt("LowresValue");
        restartPlaying = true;
    }

    if (thread_type_slice != sets().getBool("ThreadTypeSlice"))
    {
        thread_type_slice = sets().getBool("ThreadTypeSlice");
        restartPlaying = true;
    }

    int _threads = sets().getInt("Threads");
    if (_threads < 0)
        _threads = 0; //Autodetect by FFmpeg
    else if (_threads > 16)
        _threads = 16;
    if (threads != _threads)
    {
        threads = _threads;
        restartPlaying = true;
    }

    int teletextPage = sets().getInt("TeletextPage");
    if (m_teletextPage != teletextPage)
    {
        m_teletextPage = teletextPage;
        restartPlaying = true;
    }

    bool teletextTransparent = sets().getBool("TeletextTransparent");
    if (m_teletextTransparent != teletextTransparent)
    {
        m_teletextTransparent = teletextTransparent;
        restartPlaying = true;
    }

#ifdef USE_VULKAN
    m_disableZeroCopy = sets().getBool("DisableZeroCopy");
#endif

    return !restartPlaying && sets().getBool("DecoderEnabled");
}

QString FFDecSW::name() const
{
    return "FFmpeg";
}

QByteArray FFDecSW::subtitleHeader() const
{
    if (codec_ctx->codec_type != AVMEDIA_TYPE_SUBTITLE)
        return QByteArray();

    return QByteArray::fromRawData(reinterpret_cast<const char *>(codec_ctx->subtitle_header), codec_ctx->subtitle_header_size);
}

void FFDecSW::setSupportedPixelFormats(const AVPixelFormats &pixelFormats)
{
    FFDec::setSupportedPixelFormats(pixelFormats);
    setPixelFormat();
}

int FFDecSW::decodeAudio(const Packet &encodedPacket, QByteArray &decoded, double &ts, quint8 &channels, quint32 &sampleRate, bool flush)
{
    const bool onlyPendingFrames = (!flush && encodedPacket.isEmpty() && pendingFrames() > 0);

    bool frameFinished = false;
    int bytesConsumed = 0;

    if (!onlyPendingFrames)
        decodeFirstStep(encodedPacket, flush);
    if (codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO)
    {
        if (onlyPendingFrames)
            frameFinished = maybeTakeFrame();
        else
            bytesConsumed = decodeStep(frameFinished);

        if (frameFinished)
        {
            const int codecChannels = codec_ctx->CODECPAR_NB_CHANNELS;
            const int samples_with_channels = frame->nb_samples * codecChannels;
            const int decoded_size = samples_with_channels * sizeof(float);
            decoded.resize(decoded_size);
            float *decoded_data = (float *)decoded.data();
            switch (codec_ctx->sample_fmt)
            {
                case AV_SAMPLE_FMT_U8:
                {
                    uint8_t *data = (uint8_t *)*frame->data;
                    for (int i = 0; i < samples_with_channels; i++)
                        decoded_data[i] = (data[i] - 0x7F) / 128.0f;
                } break;
                case AV_SAMPLE_FMT_S16:
                {
                    int16_t *data = (int16_t *)*frame->data;
                    for (int i = 0; i < samples_with_channels; i++)
                        decoded_data[i] = data[i] / 32768.0f;
                } break;
                case AV_SAMPLE_FMT_S32:
                {
                    int32_t *data = (int32_t *)*frame->data;
                    for (int i = 0; i < samples_with_channels; i++)
                        decoded_data[i] = data[i] / 2147483648.0f;
                } break;
                case AV_SAMPLE_FMT_FLT:
                    memcpy(decoded_data, *frame->data, decoded_size);
                    break;
                case AV_SAMPLE_FMT_DBL:
                {
                    double *data = (double *)*frame->data;
                    for (int i = 0; i < samples_with_channels; i++)
                        decoded_data[i] = data[i];
                } break;

                /* Thanks Wang Bin for this patch */
                case AV_SAMPLE_FMT_U8P:
                {
                    uint8_t **data = (uint8_t **)frame->extended_data;
                    for (int i = 0; i < frame->nb_samples; ++i)
                        for (int ch = 0; ch < codecChannels; ++ch)
                            *decoded_data++ = (data[ch][i] - 0x7F) / 128.0f;
                } break;
                case AV_SAMPLE_FMT_S16P:
                {
                    int16_t **data = (int16_t **)frame->extended_data;
                    for (int i = 0; i < frame->nb_samples; ++i)
                        for (int ch = 0; ch < codecChannels; ++ch)
                            *decoded_data++ = data[ch][i] / 32768.0f;
                } break;
                case AV_SAMPLE_FMT_S32P:
                {
                    int32_t **data = (int32_t **)frame->extended_data;
                    for (int i = 0; i < frame->nb_samples; ++i)
                        for (int ch = 0; ch < codecChannels; ++ch)
                            *decoded_data++ = data[ch][i] / 2147483648.0f;
                } break;
                case AV_SAMPLE_FMT_FLTP:
                {
                    float **data = (float **)frame->extended_data;
                    for (int i = 0; i < frame->nb_samples; ++i)
                        for (int ch = 0; ch < codecChannels; ++ch)
                            *decoded_data++ = data[ch][i];
                } break;
                case AV_SAMPLE_FMT_DBLP:
                {
                    double **data = (double **)frame->extended_data;
                    for (int i = 0; i < frame->nb_samples; ++i)
                        for (int ch = 0; ch < codecChannels; ++ch)
                            *decoded_data++ = data[ch][i];
                } break;
                /**/

                default:
                    decoded.clear();
                    break;
            }
            channels = codecChannels;
            sampleRate = codec_ctx->sample_rate;
        }
    }

    if (frameFinished)
    {
        if (frame->best_effort_timestamp != AV_NOPTS_VALUE)
            ts = frame->best_effort_timestamp * av_q2d(m_timeBase);
        else if (encodedPacket.hasDts() || encodedPacket.hasPts())
            ts = encodedPacket.ts();
        else if (codec_ctx->codec_id == AV_CODEC_ID_APE && !qIsNaN(m_lastTs))
            ts = m_lastTs + static_cast<double>(frame->nb_samples) / static_cast<double>(sampleRate);
        else
            ts = qQNaN();
    }
    else
    {
        ts = qQNaN();
    }

    m_lastTs = ts;

    return (bytesConsumed <= 0) ? 0 : bytesConsumed;
}
int FFDecSW::decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurry_up)
{
    bool frameFinished = false;
    int bytesConsumed = 0;

    decodeFirstStep(encodedPacket, flush);

    if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        if (respectHurryUP && hurry_up)
        {
            if (skipFrames && !forceSkipFrames && hurry_up > 1)
                codec_ctx->skip_frame = AVDISCARD_NONREF;
            codec_ctx->skip_loop_filter = AVDISCARD_ALL;
            if (hurry_up > 1)
                codec_ctx->skip_idct = AVDISCARD_NONREF;
            codec_ctx->flags2 |= AV_CODEC_FLAG2_FAST;
        }
        else
        {
            if (!forceSkipFrames)
                codec_ctx->skip_frame = AVDISCARD_DEFAULT;
            codec_ctx->skip_loop_filter = codec_ctx->skip_idct = AVDISCARD_DEFAULT;
            codec_ctx->flags2 &= ~AV_CODEC_FLAG2_FAST;
        }

        bytesConsumed = decodeStep(frameFinished);

        if (forceSkipFrames) //Nie możemy pomijać na pierwszej klatce, ponieważ wtedy może nie być odczytany przeplot
            codec_ctx->skip_frame = AVDISCARD_NONREF;

        if (frameFinished && ~hurry_up)
        {
            bool newFormat = false;
            if (codec_ctx->pix_fmt != lastPixFmt)
            {
                newPixFmt = codec_ctx->pix_fmt;
                lastPixFmt = codec_ctx->pix_fmt;
                setPixelFormat();
                newFormat = true;
            }
            if (m_desiredPixFmt != AV_PIX_FMT_NONE)
            {
                if (m_dontConvert && frame->buf[0])
                {
                    decoded = Frame(frame);
#ifdef USE_VULKAN
                    if (frame->opaque)
                        decoded.setVulkanImage(reinterpret_cast<QmVk::Image *>(frame->opaque)->shared_from_this());
#endif
                }
                else
                {
                    if (frame->width != lastFrameW || frame->height != lastFrameH || newFormat)
                    {
                        sws_ctx = sws_getCachedContext(
                            sws_ctx,
                            frame->width,
                            frame->height,
                            codec_ctx->pix_fmt,
                            frame->width,
                            frame->height,
                            static_cast<AVPixelFormat>(m_desiredPixFmt),
                            SWS_BILINEAR,
                            nullptr,
                            nullptr,
                            nullptr
                        );
                        lastFrameW = frame->width;
                        lastFrameH = frame->height;
                    }

#ifdef USE_VULKAN
                    if (m_vkImagePool)
                    {
                        decoded = m_vkImagePool->takeToFrame(
                            vk::Extent2D(frame->width, frame->height),
                            frame,
                            m_desiredPixFmt
                        );
                    }
#endif
                    if (decoded.isEmpty())
                        decoded = Frame::createEmpty(frame, true, m_desiredPixFmt);

                    quint8 *decodedData[] = {
                        decoded.data(0),
                        decoded.data(1),
                        decoded.data(2),
                    };
                    sws_scale(
                        sws_ctx,
                        frame->data,
                        frame->linesize,
                        0,
                        frame->height,
                        decodedData,
                        decoded.linesize()
                    );
                }
            }
        }
    }

    decodeLastStep(encodedPacket, decoded, frameFinished);

    return bytesConsumed < 0 ? -1 : bytesConsumed;
}
bool FFDecSW::decodeSubtitle(const QVector<Packet> &encodedPackets, double pos, shared_ptr<QMPlay2OSD> &osd, const QSize &size, bool flush)
{
    if (codec_ctx->codec_type != AVMEDIA_TYPE_SUBTITLE)
        return false;

    if (flush)
        m_subtitles.clear();

    if (encodedPackets.isEmpty())
    {
        if (flush)
            return false;
    }
    else for (auto &&encodedPacket : encodedPackets)
    {
        decodeFirstStep(encodedPacket, false);

        m_subtitles.emplace_back();
        auto &subtitle = m_subtitles.back();

        int gotSubtitles = 0;
        if (avcodec_decode_subtitle2(codec_ctx, subtitle.av(), &gotSubtitles, packet) >= 0 && gotSubtitles && subtitle.format == 0)
        {
            subtitle.time = subtitle.start_display_time / 1000.0 + encodedPacket.ts();
            subtitle.frameSize = size;
        }
        else
        {
            m_subtitles.pop_back();
        }
    }

    return qIsNaN(pos)
        ? true
        : getFromBitmapSubsBuffer(osd, pos)
    ;
}
QList<QByteArray> FFDecSW::decodeSubtitle(const Packet &encodedPacket)
{
    if (codec_ctx->codec_type != AVMEDIA_TYPE_SUBTITLE || !(codec_ctx->codec_descriptor->props & AV_CODEC_PROP_TEXT_SUB))
        return {};

    decodeFirstStep(encodedPacket, false);

    int gotSubtitles = 0;
    AVSubtitle subtitle = {};
    if (avcodec_decode_subtitle2(codec_ctx, &subtitle, &gotSubtitles, packet) >= 0 && gotSubtitles && subtitle.format == 1)
    {
        QList<QByteArray> events;
        for (uint32_t i = 0; i < subtitle.num_rects; ++i)
        {
            events += subtitle.rects[i]->ass;
        }
        return events;
    }

    return {};
}

bool FFDecSW::open(StreamInfo &streamInfo)
{
    AVCodec *codec = FFDec::init(streamInfo);
    if (!codec)
        return false;
    if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        if ((codec_ctx->thread_count = threads) != 1)
        {
            if (!thread_type_slice)
                codec_ctx->thread_type = FF_THREAD_FRAME;
            else
                codec_ctx->thread_type = FF_THREAD_SLICE;
        }
        codec_ctx->lowres = qMin<int>(codec->max_lowres, lowres);
        lastPixFmt = codec_ctx->pix_fmt;
#ifdef USE_VULKAN
        if ((codec->capabilities & AV_CODEC_CAP_DR1) && QMPlay2Core.isVulkanRenderer())
        {
            if (m_disableZeroCopy)
            {
                qDebug() << "Vulkan :: Zero-copy decoding is disabled";
            }
            else try
            {
                static_pointer_cast<QmVk::Instance>(QMPlay2Core.gpuInstance())->physicalDevice()->findMemoryType(
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostCached
                );

                codec_ctx->opaque = this;
                codec_ctx->get_buffer2 = vulkanGetVideoBufferStatic;
#if LIBAVCODEC_VERSION_MAJOR < 60
                codec_ctx->thread_safe_callbacks = 1;
#endif
            }
            catch (const vk::SystemError &e)
            {
                Q_UNUSED(e)
                qDebug() << "Vulkan :: Zero-copy decoding is not supported";
            }
        }
#endif
    }
    else if (codec_ctx->codec_type == AVMEDIA_TYPE_SUBTITLE)
    {
        if (codec->id == AV_CODEC_ID_DVB_TELETEXT)
        {
            av_dict_set(&m_options, "txt_page", (m_teletextPage > 0) ? QByteArray::number(m_teletextPage).constData() : "subtitle", 0);
            av_dict_set(&m_options, "txt_transparent", QByteArray::number(m_teletextTransparent), 0);
        }
#ifdef USE_VULKAN
        if (QMPlay2Core.isVulkanRenderer())
            m_vkBufferPool = static_pointer_cast<QmVk::Instance>(QMPlay2Core.gpuInstance())->createBufferPool();
#endif
    }
    if (!FFDec::openCodec(codec))
        return false;
    m_timeBase = streamInfo.time_base;
    if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO && codec_ctx->lowres)
    {
        streamInfo.params->width = codec_ctx->width;
        streamInfo.params->height = codec_ctx->height;
    }
    return true;
}


void FFDecSW::setPixelFormat()
{
    m_origPixDesc = av_pix_fmt_desc_get(codec_ctx->pix_fmt);
    if (Q_UNLIKELY(!m_origPixDesc)) // Invalid pixel format
    {
        m_dontConvert = false;
        return;
    }

    m_dontConvert = m_supportedPixelFormats.contains(codec_ctx->pix_fmt);
    if (m_dontConvert)
    {
        m_desiredPixFmt = codec_ctx->pix_fmt;
        return;
    }

    enum ScoreFmtTypes {
        FlagsMatch,
        RGBMatch,
        PlannarMatch,
        Other,
        OtherGray,
        ScoreFmtTypesCount,
    };

    vector<pair<int, AVPixelFormat>> scoreFmts[ScoreFmtTypesCount];

    auto srcPixDesc = (codec_ctx->pix_fmt == AV_PIX_FMT_PAL8)
        ? av_pix_fmt_desc_get(AV_PIX_FMT_RGB32)
        : m_origPixDesc
    ;

    const QByteArray srcPixFmtName = srcPixDesc->name;

    for (const AVPixelFormat pixFmt : std::as_const(m_supportedPixelFormats))
    {
        if (!sws_isSupportedOutput(pixFmt))
            continue;

        auto pixDesc = av_pix_fmt_desc_get(pixFmt);
        Q_ASSERT(pixDesc);

        int score = 0;

        if (srcPixDesc->nb_components == pixDesc->nb_components)
            score += 3;
        else if (srcPixDesc->nb_components < pixDesc->nb_components)
            score += 2;
        else if (srcPixDesc->nb_components == 4 && pixDesc->nb_components == 3)
            score += 1;

        if (srcPixDesc->log2_chroma_w == pixDesc->log2_chroma_w)
            score += (srcPixDesc->nb_components > 1) ? 2 : -1;
        if (srcPixDesc->log2_chroma_h == pixDesc->log2_chroma_h)
            score += (srcPixDesc->nb_components > 1) ? 2 : -1;

        if (srcPixDesc->comp[0].depth == pixDesc->comp[0].depth)
            score += 3;
        else if (srcPixDesc->comp[0].depth < pixDesc->comp[0].depth)
            score += 2;

        [&] {
            if (srcPixDesc->nb_components > 1 && pixDesc->nb_components == 1 && !(srcPixDesc->nb_components == 2 && (srcPixDesc->flags & AV_PIX_FMT_FLAG_ALPHA)))
                return &scoreFmts[OtherGray];

            constexpr auto flagsMask = (AV_PIX_FMT_FLAG_PLANAR | AV_PIX_FMT_FLAG_RGB);
            const auto flagsOrig = (srcPixDesc->flags & flagsMask);
            const auto flags = (pixDesc->flags & flagsMask);
            if (flagsOrig == flags)
                return &scoreFmts[FlagsMatch];
            else if ((flagsOrig & AV_PIX_FMT_FLAG_RGB) == (flags & AV_PIX_FMT_FLAG_RGB))
                return &scoreFmts[RGBMatch];
            else if ((flagsOrig & AV_PIX_FMT_FLAG_PLANAR) == (flags & AV_PIX_FMT_FLAG_PLANAR))
                return &scoreFmts[PlannarMatch];
            return &scoreFmts[Other];
        }()->emplace_back(score, pixFmt);
    }

    for (auto &&scoreFmt : scoreFmts)
    {
        if (scoreFmt.empty())
            continue;

        sort(scoreFmt.rbegin(), scoreFmt.rend());

        m_desiredPixFmt = scoreFmt[0].second;

        QByteArray desiredPixFmtName = av_get_pix_fmt_name(m_desiredPixFmt);

        if (scoreFmt.size() > 1 && srcPixFmtName.startsWith("yuv") && desiredPixFmtName.startsWith("yuv"))
        {
            auto isYUVJFromName = [](const QByteArray &name) {
                return name.startsWith("yuvj");
            };
            auto yuvWithoutJ = [](const QByteArray &name) {
                return QByteArray(name).replace("yuvj", "yuv");
            };

            const bool srcIsYUVj = isYUVJFromName(srcPixFmtName);
            if (srcIsYUVj != isYUVJFromName(desiredPixFmtName))
            {
                for (size_t i = 1; i < scoreFmt.size(); ++i)
                {
                    if (scoreFmt[0].first != scoreFmt[i].first)
                        break;

                    const AVPixelFormat pixFmt = scoreFmt[i].second;
                    const QByteArray pixFmtName = av_get_pix_fmt_name(pixFmt);
                    if (srcIsYUVj == isYUVJFromName(pixFmtName) && yuvWithoutJ(desiredPixFmtName) == yuvWithoutJ(pixFmtName))
                    {
                        m_desiredPixFmt = pixFmt;
                        desiredPixFmtName = pixFmtName;
                        break;
                    }
                }
            }
        }

        QMPlay2Core.logInfo(QString("Converting to pixel format: " + desiredPixFmtName));
        break;
    }
}

bool FFDecSW::getFromBitmapSubsBuffer(shared_ptr<QMPlay2OSD> &osd, double pos)
{
    bool ret = true;
    for (auto i = m_subtitles.size(); i > 0; --i)
    {
        const auto &subtitle = m_subtitles[i - 1];
        if (subtitle.time > pos)
            continue;

        if (subtitle.num_rects > 0)
        {
            auto locker = QMPlay2OSD::ensure(osd);
            if (locker.owns_lock())
                osd->clear();
            osd->setDuration(subtitle.duration());
            osd->setPTS(subtitle.time);

            auto getRect = [this](AVSubtitleRect *avRect) {
                const QPoint point(
                    qBound(0, avRect->x, codec_ctx->width),
                    qBound(0, avRect->y, codec_ctx->height)
                );
                const QSize size(
                    qBound(0, avRect->w, codec_ctx->width  - point.x()),
                    qBound(0, avRect->h, codec_ctx->height - point.y())
                );
                return QRect(point, size);
            };
            auto scaleRect = [this](const QRect &rect, const QSize &frameSize) {
                const auto rx = static_cast<qreal>(frameSize.width())  / static_cast<qreal>(codec_ctx->width);
                const auto ry = static_cast<qreal>(frameSize.height()) / static_cast<qreal>(codec_ctx->height);
                const auto r = ry / rx;
                QRectF newRect = rect;
                if (r < 0.95)
                {
                    newRect.setWidth(newRect.width() * r);
                    newRect.translate(rect.width() / 2.0 - newRect.width() / 2.0, 0.0);
                }
                return QRectF(
                    newRect.x() * rx,
                    newRect.y() * ry,
                    newRect.width() * rx,
                    newRect.height() * ry
                );
            };

#ifdef USE_VULKAN
            if (m_vkBufferPool)
            {
                using namespace QmVk;

                auto device = m_vkBufferPool->instance()->device();
                if (!device)
                    break;

                const auto alignment = device->physicalDevice()->limits().minTexelBufferOffsetAlignment;

                constexpr vk::DeviceSize palSize = 256 * sizeof(uint32_t);
                const auto palBuffSize = FFALIGN(palSize, alignment);

                auto getRectBuffSize = [&](AVSubtitleRect *rect) {
                    return FFALIGN(rect->linesize[0] * rect->h, alignment);
                };

                vk::DeviceSize buffSize = 0;
                vk::DeviceSize buffOffset = 0;

                for (uint32_t i = 0; i < subtitle.num_rects; ++i)
                    buffSize += getRectBuffSize(subtitle.rects[i]) + palBuffSize;

                auto buffer = m_vkBufferPool->take(buffSize);
                if (!buffer)
                    break;

                auto data = buffer->map<uint8_t>();

                for (uint32_t i = 0; i < subtitle.num_rects; ++i) try
                {
                    const auto avRect = subtitle.rects[i];
                    const auto rectBuffSize = getRectBuffSize(avRect);
                    const auto rectSize = avRect->linesize[0] * avRect->h;

                    auto &osdImg = osd->add();

                    const auto rect = getRect(avRect);
                    osdImg.rect = scaleRect(rect, subtitle.frameSize);
                    osdImg.size = rect.size();

                    memcpy(data + buffOffset, avRect->data[0], rectSize);
                    osdImg.dataBufferView = BufferView::create(buffer, vk::Format::eR8Uint, buffOffset, rectSize);
                    buffOffset += rectBuffSize;

                    osdImg.linesize = avRect->linesize[0];

                    memcpy(data + buffOffset, avRect->data[1], palSize);
                    osdImg.paletteBufferView = BufferView::create(buffer, vk::Format::eB8G8R8A8Unorm, buffOffset, palSize);
                    buffOffset += palBuffSize;
                }
                catch (const vk::SystemError &e)
                {
                    Q_UNUSED(e)
                    osd->clear();
                    break;
                }

                osd->setReturnVkBufferFn(m_vkBufferPool, move(buffer));
            }
            else
#endif
            {
                for (uint32_t i = 0; i < subtitle.num_rects; ++i)
                {
                    const auto avRect = subtitle.rects[i];

                    auto &osdImg = osd->add();

                    const auto rect = getRect(avRect);
                    osdImg.rect = scaleRect(rect, subtitle.frameSize);
                    osdImg.size = rect.size();

                    osdImg.rgba = QByteArray(osdImg.size.width() * osdImg.size.height() * sizeof(uint32_t), Qt::Uninitialized);

                    const auto source   = (uint8_t  *)avRect->data[0];
                    const auto palette  = (uint32_t *)avRect->data[1];
                    const auto linesize = avRect->linesize[0];
                    auto dest = (uint32_t *)osdImg.rgba.data();
                    const int w = osdImg.size.width();
                    const int h = osdImg.size.height();

                    for (int y = 0; y < h; ++y)
                    {
                        for (int x = 0; x < w; ++x)
                        {
                            const uint32_t color = palette[source[y * linesize + x]];
                            *(dest++) = (color & 0xFF00FF00) | ((color << 16) & 0x00FF0000) | ((color >> 16) & 0x000000FF);
                        }
                    }
                }

            }

            osd->setNeedsRescale();
            osd->genId();
        }
        else
        {
            ret = false;
        }

        if (i > 1) // Keep one for second subtitles buffer
        {
            m_subtitles.erase(m_subtitles.begin(), m_subtitles.begin() + i - 1);
        }

        break;
    }
    return ret;
}

#ifdef USE_VULKAN
int FFDecSW::vulkanGetVideoBufferStatic(AVCodecContext *codecCtx, AVFrame *frame, int flags)
{
    return reinterpret_cast<FFDecSW *>(codecCtx->opaque)->vulkanGetVideoBuffer(codecCtx, frame, flags);
}
int FFDecSW::vulkanGetVideoBuffer(AVCodecContext *codecCtx, AVFrame *frame, int flags)
{
    if (m_dontConvert && !m_defaultGetBufferUsed)
    {
        int w = frame->width;
        int h = frame->height;
        int linesizeAligns[AV_NUM_DATA_POINTERS] = {};
        avcodec_align_dimensions2(codec_ctx, &w, &h, linesizeAligns);

        const int lineSizeAlign = (linesizeAligns[0] << m_origPixDesc->log2_chroma_w);
        w = FFALIGN(w, lineSizeAlign);

        // Increase padding height of 1 to prevent buffer overflow in some FFmpeg codecs ("avcodec_align_dimensions2" should do this...)
        uint32_t paddingHeight = h - codec_ctx->height + 1;

        if (codec_ctx->codec_id == AV_CODEC_ID_H264 && w == 4096)
        {
            // H.264 decoder can be slower with linesize of 4096 for unknown reason...
            w += lineSizeAlign;
        }

        if (m_vkImagePool->takeToAVFrame(vk::Extent2D(w, codec_ctx->height), frame, paddingHeight))
        {
            memcpy(m_vkLineSizes, frame->linesize, sizeof(m_vkLineSizes));
            return 0;
        }

        // If Vulkan allocation fails (e.g. DeviceLost), allocate frames with same linesize to make FFmpeg happy
        size_t buffSize = 0;
        size_t offsets[AV_NUM_DATA_POINTERS] = {};
        for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i)
        {
            if (m_vkLineSizes[i] == 0)
                break;

            auto log2_chroma_h = m_origPixDesc->log2_chroma_h;
            if (i == 0)
                log2_chroma_h = 0;

            offsets[i] = buffSize;
            buffSize += m_vkLineSizes[i] * AV_CEIL_RSHIFT((codec_ctx->height + paddingHeight), log2_chroma_h);
        }
        if (buffSize > 0)
        {
            // It's temporary, no need to use buffer pool
            frame->buf[0] = av_buffer_alloc(buffSize);
        }
        if (frame->buf[0])
        {
            for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i)
            {
                if (m_vkLineSizes[i] == 0)
                    break;

                frame->linesize[i] = m_vkLineSizes[i];
                frame->data[i] = frame->buf[0]->data + offsets[i];
            }
            frame->extended_data = frame->data;
            return 0;
        }
    }

    // FFmpeg can't use reference frames with different linesize (Vulkan uses different linesize)
    if (!m_defaultGetBufferUsed && m_dontConvert)
        qDebug() << "Vulkan :: Zero-copy decoding disabled due to an error";
    m_defaultGetBufferUsed = true;

    return avcodec_default_get_buffer2(codecCtx, frame, flags);
}
#endif
