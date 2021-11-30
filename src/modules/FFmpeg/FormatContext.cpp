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

#include <FFCommon.hpp>
#include <FormatContext.hpp>

#include <QMPlay2Core.hpp>
#include <Functions.hpp>
#include <OggHelper.hpp>
#include <Settings.hpp>
#include <Packet.hpp>

#include <limits>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/replaygain.h>
    #include <libavutil/spherical.h>
    #include <libavutil/pixdesc.h>
}

static void matroska_fix_ass_packet(AVRational stream_timebase, AVPacket *pkt)
{
    AVBufferRef *line;
    char *layer, *ptr = (char *)pkt->data, *end = ptr + pkt->size;
    for (; *ptr != ',' && ptr < end - 1; ptr++);
    if (*ptr == ',')
        ptr++;
    layer = ptr;
    for (; *ptr != ',' && ptr < end - 1; ptr++);
    if (*ptr == ',')
    {
        int64_t end_pts = pkt->pts + pkt->duration;
        int sc = pkt->pts * stream_timebase.num * 100 / stream_timebase.den;
        int ec = end_pts  * stream_timebase.num * 100 / stream_timebase.den;
        int sh, sm, ss, eh, em, es, len;
        sh     = sc / 360000;
        sc    -= 360000 * sh;
        sm     = sc / 6000;
        sc    -= 6000 * sm;
        ss     = sc / 100;
        sc    -= 100 * ss;
        eh     = ec / 360000;
        ec    -= 360000 * eh;
        em     = ec / 6000;
        ec    -= 6000 * em;
        es     = ec / 100;
        ec    -= 100 * es;
        *ptr++ = '\0';
        len    = 50 + end - ptr + AV_INPUT_BUFFER_PADDING_SIZE;
        if (!(line = av_buffer_alloc(len)))
            return;
        snprintf((char *)line->data, len, "Dialogue: %s,%d:%02d:%02d.%02d,%d:%02d:%02d.%02d,%s\r\n", layer, sh, sm, ss, sc, eh, em, es, ec, ptr);
        av_buffer_unref(&pkt->buf);
        pkt->buf  = line;
        pkt->data = line->data;
        pkt->size = strlen((char *)line->data);
    }
}

static QByteArray getTag(AVDictionary *metadata, const char *key, const bool deduplicate = true)
{
    AVDictionaryEntry *avTag = av_dict_get(metadata, key, nullptr, 0);
    if (avTag && avTag->value)
    {
        const QByteArray tag = QByteArray(avTag->value);
        if (deduplicate)
        {
            // Workaround for duplicated tags separated by ';'.
            // Check only when both tag has the same length and use only letters and numbers for
            // comparision (sometimes it differs in apostrophe or different/incorrect encoding).
            // Return the second tag (mostly better).
            const QList<QByteArray> tags = tag.split(';');
            if (tags.count() == 2)
            {
                const QByteArray first  = tags[0].trimmed();
                const QByteArray second = tags[1].trimmed();
                if (first.length() == second.length())
                {
                    bool ok = true;
                    for (int i = 0; i < second.length(); ++i)
                    {
                        const char c1 = first[i];
                        const char c2 = second[i];
                        if
                        (
                            (c2 >= '0' && c2 <= '9' && c1 != c2) ||
                            (
                                ((c2 >= 'a' && c2 <= 'z') || (c2 >= 'A' && c2 <= 'Z')) &&
                                ((c1 | 0x20) != (c2 | 0x20))
                            )
                        )
                        {
                            ok = false;
                            break;
                        }
                    }
                    if (ok)
                        return second;
                }
            }
        }
        return tag.trimmed();
    }
    return QByteArray();
}

static void fixFontsAttachment(AVStream *stream)
{
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_ATTACHMENT && stream->codecpar->codec_id == AV_CODEC_ID_NONE)
    {
        // If codec for fonts is unknown - check the attachment file name extension
        const QString attachmentFileName = getTag(stream->metadata, "filename", false);
        if (attachmentFileName.endsWith(".otf", Qt::CaseInsensitive))
            stream->codecpar->codec_id = AV_CODEC_ID_OTF;
        else if (attachmentFileName.endsWith(".ttf", Qt::CaseInsensitive))
            stream->codecpar->codec_id = AV_CODEC_ID_TTF;
    }
}

static bool streamNotValid(AVStream *stream)
{
    return
    (
        (stream->disposition & AV_DISPOSITION_ATTACHED_PIC)    ||
        (stream->codecpar->codec_type == AVMEDIA_TYPE_DATA) ||
        (stream->codecpar->codec_type == AVMEDIA_TYPE_ATTACHMENT && stream->codecpar->codec_id != AV_CODEC_ID_TTF && stream->codecpar->codec_id != AV_CODEC_ID_OTF)
    );
}

static int interruptCB(bool &aborted)
{
    QCoreApplication::processEvents(); //Let the demuxer thread run the timer
    return aborted;
}

class AVPacketRAII
{
public:
    inline AVPacketRAII(AVPacket *packet) :
        packet(packet)
    {}
    inline ~AVPacketRAII()
    {
        av_packet_unref(packet);
    }

private:
    AVPacket *packet;
};

class OpenFmtCtxThr : public OpenThr
{
    AVFormatContext *m_formatCtx;
    AVInputFormat *m_inputFmt;

public:
    inline OpenFmtCtxThr(AVFormatContext *formatCtx, const QByteArray &url, AVInputFormat *inputFmt, AVDictionary *options, std::shared_ptr<AbortContext> &abortCtx) :
        OpenThr(url, options, abortCtx),
        m_formatCtx(formatCtx),
        m_inputFmt(inputFmt)
    {
        start();
    }

    inline AVFormatContext *getFormatCtx() const
    {
        return waitForOpened() ? m_formatCtx : nullptr;
    }

private:
    void run() override
    {
        avformat_open_input(&m_formatCtx, m_url, m_inputFmt, &m_options);
        if (!wakeIfNotAborted() && m_formatCtx)
            avformat_close_input(&m_formatCtx);
    }
};

/**/

FormatContext::FormatContext(bool reconnectStreamed) :
    isError(false),
    currPos(0.0),
    abortCtx(new AbortContext),
    formatCtx(nullptr),
    packet(nullptr),
    oggHelper(nullptr),
    reconnectStreamed(reconnectStreamed),
    isPaused(false), fixMkvAss(false),
    isMetadataChanged(false),
    lastTime(0.0),
    invalErrCount(0), errFromSeek(0),
    maybeHasFrame(false),
    artistWithTitle(true),
    stillImage(false),
    lengthToPlay(-1)
{}
FormatContext::~FormatContext()
{
    if (formatCtx)
    {
        avformat_close_input(&formatCtx);
        av_packet_free(&packet);
    }
    delete oggHelper;
    for (StreamInfo *streamInfo : qAsConst(streamsInfo))
        delete streamInfo;
}

bool FormatContext::metadataChanged() const
{
    if (formatCtx->event_flags & AVFMT_EVENT_FLAG_METADATA_UPDATED)
    {
        formatCtx->event_flags = 0;
        isMetadataChanged = true;
    }
    if (isMetadataChanged)
    {
        isMetadataChanged = false;
        return true;
    }
    return false;
}

bool FormatContext::isStillImage() const
{
    return stillImage;
}

QList<ProgramInfo> FormatContext::getPrograms() const
{
    QList<ProgramInfo> programs;
    for (unsigned i = 0; i < formatCtx->nb_programs; ++i)
    {
        const AVProgram &program = *formatCtx->programs[i];
        if (program.discard != AVDISCARD_ALL)
        {
            ProgramInfo programInfo(program.program_num);
            if (program.nb_stream_indexes)
            {
                for (unsigned s = 0; s < program.nb_stream_indexes; ++s)
                {
                    const int ff_idx = program.stream_index[s];
                    const int idx = index_map[ff_idx];
                    if (idx > -1)
                    {
                        const AVMediaType type = streams[ff_idx]->codecpar->codec_type;
                        if (type != AVMEDIA_TYPE_UNKNOWN)
                            programInfo.streams += {idx, type};
                    }
                }
            }
            programs += programInfo;
        }
    }
    return programs;
}
QList<ChapterInfo> FormatContext::getChapters() const
{
    QList<ChapterInfo> chapters;
    for (unsigned i = 0; i < formatCtx->nb_chapters; ++i)
    {
        const AVChapter &chapter = *formatCtx->chapters[i];
        const double mul = (double)chapter.time_base.num / (double)chapter.time_base.den;
        ChapterInfo chapterInfo(chapter.start * mul, chapter.end * mul);
        chapterInfo.title = getTag(chapter.metadata, "title", false);
        chapters += chapterInfo;
    }
    return chapters;
}

QString FormatContext::name() const
{
    return formatCtx->iformat->name;
}
QString FormatContext::title() const
{
    if (isStreamed)
    {
        const QByteArray icyName = getTag(formatCtx->metadata, "icy-name", false);
        if (!icyName.isEmpty())
            return icyName;
    }
    if (lengthToPlay > 0.0)
        return QString();
    if (AVDictionary *dict = getMetadata())
    {
        const QString title  = getTag(dict, "title");
        const QString artist = artistWithTitle ? getTag(dict, "artist") : QString();
        if (!title.isEmpty() && !artist.isEmpty())
            return artist + " - " + title;
        else if (title.isEmpty() && !artist.isEmpty())
            return artist;
        else if (!title.isEmpty() && artist.isEmpty())
            return title;
    }
    if (oggHelper)
        return tr("Track") + " " + QString::number(oggHelper->track);
    return QString();
}
QList<QMPlay2Tag> FormatContext::tags() const
{
    QList<QMPlay2Tag> tagList;
    QString value;
    if (isStreamed)
    {
        if (!(value = getTag(formatCtx->metadata, "icy-name", false)).isEmpty())
            tagList += {QString::number(QMPLAY2_TAG_NAME), value};
        if (!(value = getTag(formatCtx->metadata, "icy-description", false)).isEmpty())
            tagList += {QString::number(QMPLAY2_TAG_DESCRIPTION), value};
    }
    if (isStreamed && !(value = getTag(formatCtx->metadata, "StreamTitle", false)).isEmpty())
    {
        int idx = value.indexOf(" - ");
        if (idx < 0)
            tagList += {QString::number(QMPLAY2_TAG_TITLE), value};
        else
        {
            tagList += {QString::number(QMPLAY2_TAG_TITLE), value.mid(idx + 3)};
            tagList += {QString::number(QMPLAY2_TAG_ARTIST), value.mid(0, idx)};
        }
    }
    else if (AVDictionary *dict = getMetadata())
    {
        if (!(value = getTag(dict, "title")).isEmpty())
            tagList += {QString::number(QMPLAY2_TAG_TITLE), value};
        if (!(value = getTag(dict, "artist")).isEmpty())
            tagList += {QString::number(QMPLAY2_TAG_ARTIST), value};
        if (!(value = getTag(dict, "album")).isEmpty())
            tagList += {QString::number(QMPLAY2_TAG_ALBUM), value};
        if (!(value = getTag(dict, "genre")).isEmpty())
            tagList += {QString::number(QMPLAY2_TAG_GENRE), value};
        if (!(value = getTag(dict, "date")).isEmpty())
            tagList += {QString::number(QMPLAY2_TAG_DATE), value};
        if (!(value = getTag(dict, "comment")).isEmpty())
            tagList += {QString::number(QMPLAY2_TAG_COMMENT), value};
        if (!(value = getTag(dict, "lyrics")).isEmpty())
            tagList += {QString::number(QMPLAY2_TAG_LYRICS), value};
    }
    return tagList;
}
bool FormatContext::getReplayGain(bool album, float &gain_db, float &peak) const
{
    const int streamIdx = av_find_best_stream(formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (streamIdx > -1)
    {
        if (void *sideData = av_stream_get_side_data(streams[streamIdx], AV_PKT_DATA_REPLAYGAIN, nullptr))
        {
            AVReplayGain replayGain = *(AVReplayGain *)sideData;
            qint32  tmpGain;
            quint32 tmpPeak;
            if (replayGain.album_gain == INT32_MIN && replayGain.track_gain != INT32_MIN)
                replayGain.album_gain = replayGain.track_gain;
            if (replayGain.album_gain != INT32_MIN && replayGain.track_gain == INT32_MIN)
                replayGain.track_gain = replayGain.album_gain;
            if (replayGain.album_peak == 0 && replayGain.track_peak != 0)
                replayGain.album_peak = replayGain.track_peak;
            if (replayGain.album_peak != 0 && replayGain.track_peak == 0)
                replayGain.track_peak = replayGain.album_peak;
            if (album)
            {
                tmpGain = replayGain.album_gain;
                tmpPeak = replayGain.album_peak;
            }
            else
            {
                tmpGain = replayGain.track_gain;
                tmpPeak = replayGain.track_peak;
            }
            if (tmpGain == INT32_MIN)
                return false;
            gain_db = tmpGain / 100000.0;
            if (tmpPeak != 0)
                peak = tmpPeak / 100000.0;
            return true;
        }
    }

    if (AVDictionary *dict = getMetadata())
    {
        QString album_gain_db = getTag(dict, "REPLAYGAIN_ALBUM_GAIN", false);
        QString album_peak    = getTag(dict, "REPLAYGAIN_ALBUM_PEAK", false);
        QString track_gain_db = getTag(dict, "REPLAYGAIN_TRACK_GAIN", false);
        QString track_peak    = getTag(dict, "REPLAYGAIN_TRACK_PEAK", false);

        if (album_gain_db.isEmpty() && !track_gain_db.isEmpty())
            album_gain_db = track_gain_db;
        if (!album_gain_db.isEmpty() && track_gain_db.isEmpty())
            track_gain_db = album_gain_db;
        if (album_peak.isEmpty() && !track_peak.isEmpty())
            album_peak = track_peak;
        if (!album_peak.isEmpty() && track_peak.isEmpty())
            track_peak = album_peak;

        QString str_gain_db, str_peak;
        if (album)
        {
            str_gain_db = album_gain_db;
            str_peak = album_peak;
        }
        else
        {
            str_gain_db = track_gain_db;
            str_peak = track_peak;
        }

        int space_idx = str_gain_db.indexOf(' ');
        if (space_idx > -1)
            str_gain_db.remove(space_idx, str_gain_db.length() - space_idx);

        bool ok;
        float tmp = str_peak.toFloat(&ok);
        if (ok)
            peak = tmp;
        tmp = str_gain_db.toFloat(&ok);
        if (ok)
            gain_db = tmp;

        return ok;
    }
    return false;
}
qint64 FormatContext::size() const
{
    if (!isStreamed && !stillImage && formatCtx->pb)
        return avio_size(formatCtx->pb);
    return -1;
}
double FormatContext::length() const
{
    if (!isStreamed && !stillImage && formatCtx->duration != AV_NOPTS_VALUE)
    {
        if (lengthToPlay > 0.0)
            return lengthToPlay;
        return formatCtx->duration / (double)AV_TIME_BASE;
    }
    return -1.0;
}
int FormatContext::bitrate() const
{
    return formatCtx->bit_rate / 1000;
}
QByteArray FormatContext::image(bool forceCopy) const
{
    for (const AVStream *stream : streams)
        if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC)
            return forceCopy ? QByteArray((const char *)stream->attached_pic.data, stream->attached_pic.size) : QByteArray::fromRawData((const char *)stream->attached_pic.data, stream->attached_pic.size);
    return QByteArray();
}

bool FormatContext::seek(double pos, bool backward)
{
    bool isOk = false;
    abortCtx->isAborted = false;
    if (!isStreamed)
    {
        const double len = length();
        if (pos < 0.0)
            pos = 0.0;
        else if (len > 0.0 && pos > len)
            pos = len;

        const double posToSeek = pos + startTime;
        const qint64 timestamp = ((streamsInfo.count() == 1) ? posToSeek : (backward ? floor(posToSeek) : ceil(posToSeek))) * AV_TIME_BASE;

        isOk = av_seek_frame(formatCtx, -1, timestamp, backward ? AVSEEK_FLAG_BACKWARD : 0) >= 0;
        if (!isOk)
        {
            const int ret = av_read_frame(formatCtx, packet);
            if (ret == AVERROR_EOF || ret == 0)
            {
                if (len <= 0.0 || pos < len)
                    isOk = av_seek_frame(formatCtx, -1, timestamp, !backward ? AVSEEK_FLAG_BACKWARD : 0) >= 0; //Negate "backward" and try again
                else if (ret == AVERROR_EOF)
                    isOk = true; //Allow seek to the end of the file, clear buffers and finish the playback
                if (isOk)
                    av_packet_unref(packet);
            }
            if (!isOk) //If seek failed - allow to use the packet
            {
                errFromSeek = ret;
                maybeHasFrame = true;
            }
        }
        if (isOk)
        {
            for (int i = 0; i < streamsTS.count(); ++i)
                streamsTS[i] = pos;
            currPos = pos;
            nextDts.fill(pos);
            isError = false;
        }
    }
    return isOk;
}
bool FormatContext::read(Packet &encoded, int &idx)
{
    if (abortCtx->isAborted)
    {
        isError = true;
        return false;
    }

    if (isPaused)
    {
        isPaused = false;
        av_read_play(formatCtx);
    }

    AVPacketRAII avPacketRAII(packet);

    int ret;
    if (!maybeHasFrame)
    {
        ret = av_read_frame(formatCtx, packet);
    }
    else
    {
        maybeHasFrame = false;
        ret = errFromSeek;
        errFromSeek = 0;
    }

    if (ret == AVERROR_INVALIDDATA)
    {
        if (invalErrCount < 1000)
        {
            ++invalErrCount;
            return true;
        }
        isError = true;
        return false;
    }
    else
        invalErrCount = 0;
    if (ret == AVERROR(EAGAIN))
        return true;
    else if (ret)
    {
        isError = true;
        return false;
    }

    const int ff_idx = packet->stream_index;
    if (ff_idx >= streams.count())
    {
        QMPlay2Core.log("Stream index out of range: " + QString::number(ff_idx), ErrorLog | LogOnce | DontShowInGUI);
        return true;
    }

    const auto stream = streams.at(ff_idx);

    if (stream->event_flags & AVSTREAM_EVENT_FLAG_METADATA_UPDATED)
    {
        stream->event_flags = 0;
        isMetadataChanged = true;
    }
    if (fixMkvAss && stream->codecpar->codec_id == AV_CODEC_ID_ASS)
        matroska_fix_ass_packet(stream->time_base, packet);

    encoded = Packet(packet, forceCopy);
    encoded.setTimeBase(stream->time_base);

    if (!qFuzzyIsNull(startTime))
        encoded.setOffsetTS(startTime);

    if (packet->duration <= 0)
    {
        double newDuration = 0.0;
        if (encoded.ts())
        {
            // Calculate packet duration if doesn't exist
            newDuration = qMax(0.0, encoded.ts() - streamsTS.at(ff_idx));
        }
        encoded.setDuration(newDuration);
    }
    streamsTS[ff_idx] = encoded.ts();

    if (isStreamed)
    {
        if (!isOneStreamOgg)
        {
            encoded.setTS(encoded.ts() + streamsOffset.at(ff_idx));
        }
        else
        {
            encoded.setTS(lastTime);
            lastTime += encoded.duration();
        }
    }
    else if (lengthToPlay > 0.0 && encoded.ts() > lengthToPlay)
    {
        isError = true;
        return false;
    }

    // Generate DTS for key frames if DTS doesn't exist (workaround for some M3U8 seekable streams)
    if (encoded.hasKeyFrame() && !encoded.hasDts())
        encoded.setDts(nextDts.at(ff_idx));
    nextDts[ff_idx] = encoded.ts() + encoded.duration();

    currPos = encoded.ts();

    idx = index_map.at(ff_idx);

    return true;
}
void FormatContext::pause()
{
    av_read_pause(formatCtx);
    isPaused = true;
}
void FormatContext::abort()
{
    abortCtx->abort();
}

bool FormatContext::open(const QString &_url, const QString &param)
{
    static const QStringList disabledDemuxers {
        "ass",
        "tty", //txt files
        "srt",
    };

    const QByteArray scheme = Functions::getUrlScheme(_url).toUtf8();
    if (scheme.isEmpty() || scheme == "sftp")
        return false;

    const Settings &settings = QMPlay2Core.getSettings();

    artistWithTitle = !settings.getBool("HideArtistMetadata");

    bool limitedLength = false;
    qint64 oggOffset = -1, oggSize = -1;
    int oggTrack = -1;
    QString url;

    if (param.startsWith("CUE:")) //For CUE files
    {
        const QStringList splitted = param.split(':');
        if (splitted.count() != 3)
            return false;
        bool ok1 = false, ok2 = false;
        startTime = splitted[1].toDouble(&ok1);
        lengthToPlay = splitted[2].toDouble(&ok2);
        if (!ok1 || !ok2 || startTime < 0.0 || (!qFuzzyCompare(lengthToPlay, -1.0) && lengthToPlay <= 0.0))
            return false;
        if (lengthToPlay > 0.0)
            lengthToPlay -= startTime;
        limitedLength = true;
    }
    else if (param.startsWith("OGG:")) //For chained OGG files
    {
        const QStringList splitted = param.split(':');
        if (splitted.count() != 4)
            return false;
        oggTrack = splitted[1].toInt();
        oggOffset = splitted[2].toLongLong();
        oggSize = splitted[3].toLongLong();
        if (oggTrack <= 0 || oggOffset < 0 || (oggSize != -1 && oggSize <= 0))
            return false;
    }

    AVInputFormat *inputFmt = nullptr;
    if (scheme == "file")
        isLocal = true;
    else
    {
        if (scheme != "rtsp")
        {
            // It is needed for QMPlay2 schemes like "alsa://", "v4l2://", etc.
            inputFmt = av_find_input_format(scheme);
            if (inputFmt)
                url = _url.right(_url.length() - scheme.length() - 3);
        }
        isLocal = false;
    }

    AVDictionary *options = nullptr;
    if (!inputFmt)
    {
        url = Functions::prepareFFmpegUrl(_url, options, false);
        if (!isLocal && reconnectStreamed)
            av_dict_set(&options, "reconnect_streamed", "1", 0);
    }

    formatCtx = avformat_alloc_context();
    formatCtx->interrupt_callback.callback = (int(*)(void *))interruptCB;
    formatCtx->interrupt_callback.opaque = &abortCtx->isAborted;

    if (oggOffset >= 0)
    {
        oggHelper = new OggHelper(url, oggTrack, oggSize, formatCtx->interrupt_callback);
        if (!oggHelper->pb)
            return false;
        formatCtx->pb = oggHelper->pb;
        av_dict_set(&options, "skip_initial_bytes", QString::number(oggOffset).toLatin1(), 0);
    }

    // Useful, e.g. CUVID decoder needs valid PTS
    formatCtx->flags |= AVFMT_FLAG_GENPTS;

    if (url.endsWith("sdp"))
        av_dict_set(&options, "protocol_whitelist", "file,crypto,data,udp,rtp", 0);

    OpenFmtCtxThr *openThr = new OpenFmtCtxThr(formatCtx, url.toUtf8(), inputFmt, options, abortCtx);
    formatCtx = openThr->getFormatCtx();
    openThr->drop();
    if (!formatCtx || disabledDemuxers.contains(name()))
        return false;

    if (name() == "sdp")
    {
        isLocal = false;
        formatCtx->flags |= AVFMT_FLAG_NOBUFFER;
    }
    else if (name().startsWith("image2") || name().endsWith("_pipe"))
    {
        if (!settings.getBool("StillImages"))
            return false;
        stillImage = true;
    }
    else if (name() == "mp3")
    {
        formatCtx->flags |= AVFMT_FLAG_FAST_SEEK; //This should be set before "avformat_open_input", but seems to be working for MP3...
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0)
        return false;

    // Determine the duration of WavPack if not known
    if (isLocal && formatCtx->nb_streams == 1 && formatCtx->duration == AV_NOPTS_VALUE)
    {
        const auto stream = formatCtx->streams[0];
        if (stream->codecpar->codec_id == AV_CODEC_ID_WAVPACK)
        {
            if (av_seek_frame(formatCtx, 0, std::numeric_limits<int64_t>::max(), AVSEEK_FLAG_BACKWARD) >= 0)
            {
                auto pkt = av_packet_alloc();
                if (av_read_frame(formatCtx, pkt) == 0)
                {
                    if (pkt->dts != AV_NOPTS_VALUE)
                        formatCtx->duration = av_rescale_q(pkt->dts + pkt->duration, stream->time_base, {1, AV_TIME_BASE});
                }
                av_packet_free(&pkt);
            }
            av_seek_frame(formatCtx, 0, 0, AVSEEK_FLAG_BACKWARD);
        }
    }

    isStreamed = !isLocal && formatCtx->duration <= 0; // AV_NOPTS_VALUE is negative

#ifdef QMPlay2_libavdevice
    forceCopy = name().contains("v4l2"); //Workaround for v4l2 - if many buffers are referenced demuxer doesn't produce proper timestamps (FFmpeg BUG?).
#else
    forceCopy = false;
#endif

    if (!limitedLength && (startTime = formatCtx->start_time / (double)AV_TIME_BASE) < 0.0)
        startTime = 0.0;

    if (limitedLength && lengthToPlay < 0.0)
    {
        lengthToPlay = length() - startTime;
        if (lengthToPlay <= 0.0)
            return false;
    }

    index_map.resize(formatCtx->nb_streams);
    streamsTS.resize(formatCtx->nb_streams);
    streamsOffset.resize(formatCtx->nb_streams);
    nextDts.resize(formatCtx->nb_streams);
    for (unsigned i = 0; i < formatCtx->nb_streams; ++i)
    {
        fixFontsAttachment(formatCtx->streams[i]);
        StreamInfo *streamInfo = getStreamInfo(formatCtx->streams[i]);
        if (!streamInfo)
            index_map[i] = -1;
        else
        {
            index_map[i] = streamsInfo.count();
            streamsInfo += streamInfo;
        }
        if (!fixMkvAss && formatCtx->streams[i]->codecpar->codec_id == AV_CODEC_ID_ASS && !strncasecmp(formatCtx->iformat->name, "matroska", 8))
            fixMkvAss = true;
        formatCtx->streams[i]->event_flags = 0;
        streams += formatCtx->streams[i];

        streamsTS[i] = 0.0;
    }
    if (streamsInfo.isEmpty())
        return false;

    isOneStreamOgg = (name() == "ogg" && streamsInfo.count() == 1); //Workaround for OGG network streams

    if (isStreamed && streamsInfo.count() == 1 && streamsInfo.at(0)->codec_type == AVMEDIA_TYPE_SUBTITLE && formatCtx->pb && avio_size(formatCtx->pb) > 0)
        isStreamed = false; //Allow subtitles streams to be non-streamed if size is known

    formatCtx->event_flags = 0;

    packet = av_packet_alloc();

    if (lengthToPlay > 0.0)
        return seek(0.0, false);
    return true;
}

void FormatContext::setStreamOffset(double offset)
{
    if (isOneStreamOgg)
        lastTime = offset;
    else for (int i = 0; i < streamsOffset.count(); ++i)
        streamsOffset[i] = offset - streamsTS.at(i);
}

/**/

AVDictionary *FormatContext::getMetadata() const
{
    return (isStreamed || (!formatCtx->metadata && streamsInfo.count() == 1)) ? streams[0]->metadata : formatCtx->metadata;
}
StreamInfo *FormatContext::getStreamInfo(AVStream *stream) const
{
    if (streamNotValid(stream))
        return nullptr;

    StreamInfo *streamInfo = new StreamInfo(stream->codecpar);

    streamInfo->must_decode = true;
    if (const AVCodecDescriptor *codecDescr = avcodec_descriptor_get(streamInfo->codec_id))
    {
        if (codecDescr->props & AV_CODEC_PROP_TEXT_SUB)
            streamInfo->must_decode = false;

        if (streamInfo->codec_name.isEmpty())
            streamInfo->codec_name = codecDescr->name;
    }
    else if (streamInfo->codec_type == AVMEDIA_TYPE_SUBTITLE)
    {
        streamInfo->must_decode = false;
    }

    streamInfo->is_default = stream->disposition & AV_DISPOSITION_DEFAULT;
    streamInfo->time_base = stream->time_base;

    if (streamInfo->codec_type != AVMEDIA_TYPE_ATTACHMENT)
    {
        QString value;
        if (streamsInfo.count() > 1)
        {
            streamInfo->title  = getTag(stream->metadata, "title");
            streamInfo->artist = getTag(stream->metadata, "artist");
            if (!(value = getTag(stream->metadata, "album")).isEmpty())
                streamInfo->other_info += {QString::number(QMPLAY2_TAG_ALBUM), value};
            if (!(value = getTag(stream->metadata, "genre")).isEmpty())
                streamInfo->other_info += {QString::number(QMPLAY2_TAG_GENRE), value};
            if (!(value = getTag(stream->metadata, "date")).isEmpty())
                streamInfo->other_info += {QString::number(QMPLAY2_TAG_DATE), value};
            if (!(value = getTag(stream->metadata, "comment")).isEmpty())
                streamInfo->other_info += {QString::number(QMPLAY2_TAG_COMMENT), value};
        }
        if (!(value = getTag(stream->metadata, "language", false)).isEmpty() && value != "und")
            streamInfo->other_info += {QString::number(QMPLAY2_TAG_LANGUAGE), value};
    }

    switch (streamInfo->codec_type)
    {
        case AVMEDIA_TYPE_VIDEO:
        {
            if (stream->sample_aspect_ratio.num)
                streamInfo->sample_aspect_ratio = stream->sample_aspect_ratio;

            if (!stillImage)
                streamInfo->fps = stream->r_frame_rate;

            bool ok = false;
            const double rotation = getTag(stream->metadata, "rotate", false).toDouble(&ok);
            if (ok)
                streamInfo->rotation = rotation;

            if (void *sideData = av_stream_get_side_data(stream, AV_PKT_DATA_SPHERICAL, nullptr))
            {
                streamInfo->spherical = (((AVSphericalMapping *)sideData)->projection == AV_SPHERICAL_EQUIRECTANGULAR);
            }

            break;
        }
        case AVMEDIA_TYPE_ATTACHMENT:
        {
            streamInfo->title = getTag(stream->metadata, "filename", false);
            switch (streamInfo->codec_id)
            {
                case AV_CODEC_ID_TTF:
                    streamInfo->codec_name = "TTF";
                    break;
                case AV_CODEC_ID_OTF:
                    streamInfo->codec_name = "OTF";
                    break;
                default:
                    break;
            }
            break;
        }
        default:
        {
            break;
        }
    }

    return streamInfo;
}
