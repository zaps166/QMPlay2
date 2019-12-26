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

#include <FFDecSW.hpp>
#include <FFCommon.hpp>

#include <QMPlay2OSD.hpp>
#include <Frame.hpp>
#include <StreamInfo.hpp>
#include <Functions.hpp>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/pixdesc.h>
}

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

    return !restartPlaying && sets().getBool("DecoderEnabled");
}

QString FFDecSW::name() const
{
    return "FFmpeg";
}

void FFDecSW::setSupportedPixelFormats(const AVPixelFormats &pixelFormats)
{
    supportedPixelFormats = pixelFormats;
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
            const int samples_with_channels = frame->nb_samples * codec_ctx->channels;
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
                        for (int ch = 0; ch < codec_ctx->channels; ++ch)
                            *decoded_data++ = (data[ch][i] - 0x7F) / 128.0f;
                } break;
                case AV_SAMPLE_FMT_S16P:
                {
                    int16_t **data = (int16_t **)frame->extended_data;
                    for (int i = 0; i < frame->nb_samples; ++i)
                        for (int ch = 0; ch < codec_ctx->channels; ++ch)
                            *decoded_data++ = data[ch][i] / 32768.0f;
                } break;
                case AV_SAMPLE_FMT_S32P:
                {
                    int32_t **data = (int32_t **)frame->extended_data;
                    for (int i = 0; i < frame->nb_samples; ++i)
                        for (int ch = 0; ch < codec_ctx->channels; ++ch)
                            *decoded_data++ = data[ch][i] / 2147483648.0f;
                } break;
                case AV_SAMPLE_FMT_FLTP:
                {
                    float **data = (float **)frame->extended_data;
                    for (int i = 0; i < frame->nb_samples; ++i)
                        for (int ch = 0; ch < codec_ctx->channels; ++ch)
                            *decoded_data++ = data[ch][i];
                } break;
                case AV_SAMPLE_FMT_DBLP:
                {
                    double **data = (double **)frame->extended_data;
                    for (int i = 0; i < frame->nb_samples; ++i)
                        for (int ch = 0; ch < codec_ctx->channels; ++ch)
                            *decoded_data++ = data[ch][i];
                } break;
                /**/

                default:
                    decoded.clear();
                    break;
            }
            channels = codec_ctx->channels;
            sampleRate = codec_ctx->sample_rate;
        }
    }

    if (frameFinished)
    {
        if (frame->best_effort_timestamp != AV_NOPTS_VALUE)
            ts = frame->best_effort_timestamp * time_base;
        else
            ts = encodedPacket.ts();
    }
    else
    {
        ts = qQNaN();
    }

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
            if (desiredPixFmt != AV_PIX_FMT_NONE)
            {
                if (dontConvert && frame->buf[0] && frame->buf[1] && frame->buf[2])
                {
                    decoded = Frame(frame);
                }
                else
                {
                    decoded = Frame::createEmpty(frame, true, desiredPixFmt);
                    if (frame->width != lastFrameW || frame->height != lastFrameH || newFormat)
                    {
                        sws_ctx = sws_getCachedContext(sws_ctx, frame->width, frame->height, codec_ctx->pix_fmt, frame->width, frame->height, (AVPixelFormat)desiredPixFmt, SWS_BILINEAR, nullptr, nullptr, nullptr);
                        lastFrameW = frame->width;
                        lastFrameH = frame->height;
                    }
                    quint8 *decodedData[] = {
                        decoded.data(0),
                        decoded.data(1),
                        decoded.data(2),
                    };
                    sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, decodedData, decoded.linesize());
                }
            }
        }
    }

    decodeLastStep(encodedPacket, decoded, frameFinished);

    return bytesConsumed < 0 ? -1 : bytesConsumed;
}
bool FFDecSW::decodeSubtitle(const Packet &encodedPacket, double pos, QMPlay2OSD *&osd, const QSize &size, bool flush)
{
    if (codec_ctx->codec_type != AVMEDIA_TYPE_SUBTITLE)
        return false;

    if (flush)
        m_subtitles.clear();

    if (encodedPacket.isEmpty())
    {
        if (flush)
            return false;
        return getFromBitmapSubsBuffer(osd, pos);
    }

    decodeFirstStep(encodedPacket, false);

    int gotSubtitles = 0;
    AVSubtitle avSubtitle;
    if (avcodec_decode_subtitle2(codec_ctx, &avSubtitle, &gotSubtitles, packet) >= 0 && gotSubtitles && avSubtitle.format == 0)
    {
        m_subtitles.emplace_back();
        auto &subtitle = *m_subtitles.rbegin();

        subtitle.pts = avSubtitle.start_display_time + encodedPacket.ts();
        subtitle.duration = (avSubtitle.end_display_time != static_cast<uint32_t>(-1))
            ? (avSubtitle.end_display_time - avSubtitle.start_display_time) / 1000.0
            : -1.0;

        for (unsigned i = 0; i < avSubtitle.num_rects; ++i)
        {
            const AVSubtitleRect *avRect = avSubtitle.rects[i];

            subtitle.rects.emplace_back();
            auto &rect = *subtitle.rects.rbegin();

            rect.w = qBound(0, avRect->w, size.width());
            rect.h = qBound(0, avRect->h, size.height());
            rect.x = qBound(0, avRect->x, size.width()  - rect.w);
            rect.y = qBound(0, avRect->y, size.height() - rect.h);

            rect.data.resize(rect.w * rect.h * sizeof(uint32_t));

            const auto source   = (uint8_t  *)avRect->data[0];
            const auto palette  = (uint32_t *)avRect->data[1];
            const auto linesize = avRect->linesize[0];
            auto dest = (uint32_t *)rect.data.data();
            const int w = rect.w, h = rect.h;

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
    if (gotSubtitles)
        avsubtitle_free(&avSubtitle);

    return getFromBitmapSubsBuffer(osd, pos);
}

bool FFDecSW::open(StreamInfo &streamInfo, VideoWriter *)
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
    }
    if (!FFDec::openCodec(codec))
        return false;
    time_base = streamInfo.getTimeBase();
    if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO && codec_ctx->lowres)
    {
        streamInfo.width = codec_ctx->width;
        streamInfo.height = codec_ctx->height;
    }
    return true;
}


void FFDecSW::setPixelFormat()
{
    const AVPixFmtDescriptor *pixDesc = av_pix_fmt_desc_get(codec_ctx->pix_fmt);
    if (!pixDesc) //Invalid pixel format
        return;
    dontConvert = supportedPixelFormats.contains(codec_ctx->pix_fmt);
    if (dontConvert)
    {
        desiredPixFmt = codec_ctx->pix_fmt;
    }
    else for (int i = 0; i < supportedPixelFormats.count(); ++i)
    {
        const AVPixelFormat pixFmt = supportedPixelFormats.at(i);
        const AVPixFmtDescriptor *supportedPixDesc = av_pix_fmt_desc_get(pixFmt);
        if (i == 0 || (supportedPixDesc->log2_chroma_w == pixDesc->log2_chroma_w && supportedPixDesc->log2_chroma_h == pixDesc->log2_chroma_h))
        {
            //Use first format as default (mostly AV_PIX_FMT_YUV420P) and look at next formats,
            //otherwise break the loop if found proper format.
            desiredPixFmt = pixFmt;
            if (i != 0)
                break;
        }
    }
}

bool FFDecSW::getFromBitmapSubsBuffer(QMPlay2OSD *&osd, double pos)
{
    bool ret = true;
    for (auto i = m_subtitles.size(); i > 0; --i)
    {
        const auto &subtitle = m_subtitles[i - 1];
        if (subtitle.pts > pos)
            continue;

        if (!subtitle.rects.empty())
        {
            bool mustUnlock = false;
            if (osd)
            {
                osd->lock();
                osd->clear();
                mustUnlock = true;
            }
            else
            {
                osd = new QMPlay2OSD;
            }
            osd->setDuration(subtitle.duration);
            osd->setPTS(subtitle.pts);
            for (auto &&rect : subtitle.rects)
                osd->addImage(QRect(rect.x, rect.y, rect.w, rect.h), rect.data);
            osd->setNeedsRescale();
            osd->genId();
            if (mustUnlock)
                osd->unlock();
        }
        else
        {
            ret = false;
        }

        m_subtitles.erase(m_subtitles.begin(), m_subtitles.begin() + i);
        break;
    }
    return ret;
}
