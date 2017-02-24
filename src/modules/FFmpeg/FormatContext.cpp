/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

#if LIBAVFORMAT_VERSION_INT >= 0x373000 // >= 55.48.00
	#define HAS_REPLAY_GAIN
#endif

extern "C"
{
	#include <libavformat/avformat.h>
#ifdef HAS_REPLAY_GAIN
	#include <libavutil/replaygain.h>
#endif
	#include <libavutil/pixdesc.h>
#if LIBAVFORMAT_VERSION_MAJOR <= 55
	#include <libavutil/opt.h>
#endif
}

#if LIBAVFORMAT_VERSION_MAJOR > 55
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
		len    = 50 + end - ptr + FF_INPUT_BUFFER_PADDING_SIZE;
		if (!(line = av_buffer_alloc(len)))
			return;
		snprintf((char *)line->data, len, "Dialogue: %s,%d:%02d:%02d.%02d,%d:%02d:%02d.%02d,%s\r\n", layer, sh, sm, ss, sc, eh, em, es, ec, ptr);
		av_buffer_unref(&pkt->buf);
		pkt->buf  = line;
		pkt->data = line->data;
		pkt->size = strlen((char *)line->data);
	}
}
#endif

#if LIBAVFORMAT_VERSION_INT >= 0x392900
	static inline AVCodecParameters *codecParams(AVStream *stream)
	{
		return stream->codecpar;
	}
	static inline const char *getPixelFormat(AVStream *stream)
	{
		return av_get_pix_fmt_name((AVPixelFormat)stream->codecpar->format);
	}
	static inline const char *getSampleFormat(AVStream *stream)
	{
		return av_get_sample_fmt_name((AVSampleFormat)stream->codecpar->format);
	}
#else
	static inline AVCodecContext *codecParams(AVStream *stream)
	{
		return stream->codec;
	}
	static inline const char *getPixelFormat(AVStream *stream)
	{
		return av_get_pix_fmt_name(stream->codec->pix_fmt);
	}
	static inline const char *getSampleFormat(AVStream *stream)
	{
		return av_get_sample_fmt_name(stream->codec->sample_fmt);
	}
#endif

static QByteArray getTag(AVDictionary *metadata, const char *key, const bool deduplicate = true)
{
	AVDictionaryEntry *avTag = av_dict_get(metadata, key, nullptr, AV_DICT_IGNORE_SUFFIX);
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

static bool streamNotValid(AVStream *stream)
{
	return
	(
		(stream->disposition & AV_DISPOSITION_ATTACHED_PIC)    ||
		(codecParams(stream)->codec_type == AVMEDIA_TYPE_DATA) ||
		(codecParams(stream)->codec_type == AVMEDIA_TYPE_ATTACHMENT && codecParams(stream)->codec_id != AV_CODEC_ID_TTF && codecParams(stream)->codec_id != AV_CODEC_ID_OTF)
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
	inline OpenFmtCtxThr(AVFormatContext *formatCtx, const QByteArray &url, AVInputFormat *inputFmt, AVDictionary *options, QSharedPointer<AbortContext> &abortCtx) :
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

FormatContext::FormatContext(QMutex &avcodec_mutex) :
	isError(false),
	currPos(0.0),
	abortCtx(new AbortContext),
	formatCtx(nullptr),
	packet(nullptr),
	oggHelper(nullptr),
	isPaused(false), fixMkvAss(false),
	isMetadataChanged(false),
	lastTime(0.0),
	invalErrCount(0), errFromSeek(0),
	maybeHasFrame(false),
	stillImage(false),
	avcodec_mutex(avcodec_mutex)
{}
FormatContext::~FormatContext()
{
	if (formatCtx)
	{
		for (AVStream *stream : streams)
		{
			if (codecParams(stream) && !streamNotValid(stream))
			{
				//Data is allocated in QByteArray, so FFmpeg mustn't free it!
				codecParams(stream)->extradata = nullptr;
				codecParams(stream)->extradata_size = 0;
			}
		}
		avformat_close_input(&formatCtx);
		FFCommon::freeAVPacket(packet);
	}
	delete oggHelper;
}

bool FormatContext::metadataChanged() const
{
#if LIBAVFORMAT_VERSION_MAJOR > 55
	if (formatCtx->event_flags & AVFMT_EVENT_FLAG_METADATA_UPDATED)
	{
		formatCtx->event_flags = 0;
		isMetadataChanged = true;
	}
#endif
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
						const QMPlay2MediaType type = (QMPlay2MediaType)codecParams(streams[ff_idx])->codec_type;
						if (type != QMPLAY2_TYPE_UNKNOWN)
							programInfo.streams += qMakePair(idx, type);
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
	if (AVDictionary *dict = getMetadata())
	{
		const QString title  = getTag(dict, "title");
		const QString artist = getTag(dict, "artist");
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
			tagList << qMakePair(QString::number(QMPLAY2_TAG_NAME), value);
		if (!(value = getTag(formatCtx->metadata, "icy-description", false)).isEmpty())
			tagList << qMakePair(QString::number(QMPLAY2_TAG_DESCRIPTION), value);
	}
	if (isStreamed && !(value = getTag(formatCtx->metadata, "StreamTitle", false)).isEmpty())
	{
		int idx = value.indexOf(" - ");
		if (idx < 0)
			tagList << qMakePair(QString::number(QMPLAY2_TAG_TITLE), value);
		else
		{
			tagList << qMakePair(QString::number(QMPLAY2_TAG_TITLE), value.mid(idx + 3));
			tagList << qMakePair(QString::number(QMPLAY2_TAG_ARTIST), value.mid(0, idx));
		}
	}
	else if (AVDictionary *dict = getMetadata())
	{
		if (!(value = getTag(dict, "title")).isEmpty())
			tagList << qMakePair(QString::number(QMPLAY2_TAG_TITLE), value);
		if (!(value = getTag(dict, "artist")).isEmpty())
			tagList << qMakePair(QString::number(QMPLAY2_TAG_ARTIST), value);
		if (!(value = getTag(dict, "album")).isEmpty())
			tagList << qMakePair(QString::number(QMPLAY2_TAG_ALBUM), value);
		if (!(value = getTag(dict, "genre")).isEmpty())
			tagList << qMakePair(QString::number(QMPLAY2_TAG_GENRE), value);
		if (!(value = getTag(dict, "date")).isEmpty())
			tagList << qMakePair(QString::number(QMPLAY2_TAG_DATE), value);
		if (!(value = getTag(dict, "comment")).isEmpty())
			tagList << qMakePair(QString::number(QMPLAY2_TAG_COMMENT), value);
	}
	return tagList;
}
bool FormatContext::getReplayGain(bool album, float &gain_db, float &peak) const
{
#ifdef HAS_REPLAY_GAIN
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
#else
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
#endif
	return false;
}
double FormatContext::length() const
{
	if (!isStreamed && !stillImage && formatCtx->duration != QMPLAY2_NOPTS_VALUE)
		return formatCtx->duration / (double)AV_TIME_BASE;
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

bool FormatContext::seek(int pos, bool backward)
{
	bool isOk = false;
	abortCtx->isAborted = false;
	if (!isStreamed)
	{
		const int len = length();
		if (pos < 0)
			pos = 0;
		else if (len > 0 && pos > len)
			pos = len;
		pos += startTime;
#ifndef MP3_FAST_SEEK
		if (seekByByteOffset < 0)
		{
#endif
			const qint64 timestamp = (qint64)pos * AV_TIME_BASE + 250000LL;
			isOk = av_seek_frame(formatCtx, -1, timestamp, backward ? AVSEEK_FLAG_BACKWARD : 0) >= 0;
			if (!isOk)
			{
				const int ret = av_read_frame(formatCtx, packet);
				if (ret == AVERROR_EOF || ret == 0)
				{
					if (len <= 0 || pos < len)
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
#ifndef MP3_FAST_SEEK
		}
		else if (length() > 0)
			isOk = av_seek_frame(formatCtx, -1, (qint64)pos * (avio_size(formatCtx->pb) - seekByByteOffset) / length() + seekByByteOffset, AVSEEK_FLAG_BYTE | (backward ? AVSEEK_FLAG_BACKWARD : 0)) >= 0;
#endif
		if (isOk)
		{
			for (int i = 0; i < streamsTS.count(); ++i)
				streamsTS[i] = pos;
			currPos = pos;
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
		ret = av_read_frame(formatCtx, packet);
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

#if LIBAVFORMAT_VERSION_MAJOR > 55
	if (streams.at(ff_idx)->event_flags & AVSTREAM_EVENT_FLAG_METADATA_UPDATED)
	{
		streams.at(ff_idx)->event_flags = 0;
		isMetadataChanged = true;
	}
	if (fixMkvAss && codecParams(streams.at(ff_idx))->codec_id == AV_CODEC_ID_ASS)
		matroska_fix_ass_packet(streams.at(ff_idx)->time_base, packet);
#else
	if (isStreamed)
	{
		char *value = nullptr;
		av_opt_get(formatCtx, "icy_metadata_packet", AV_OPT_SEARCH_CHILDREN, (quint8 **)&value);
		QString icyPacket = value;
		av_free(value);
		int idx = icyPacket.indexOf("StreamTitle='");
		if (idx > -1)
		{
			int idx2 = icyPacket.indexOf("';", idx += 13);
			if (idx2 > -1)
			{
				AVDictionaryEntry *e = av_dict_get(formatCtx->metadata, "StreamTitle", nullptr, AV_DICT_IGNORE_SUFFIX);
				icyPacket = icyPacket.mid(idx, idx2-idx);
				if (!e || QString(e->value) != icyPacket)
				{
					av_dict_set(&formatCtx->metadata, "StreamTitle", icyPacket.toUtf8(), 0);
					isMetadataChanged = true;
				}
			}
		}
		else if (AVDictionary *dict = getMetadata())
		{
			if (metadata != dict)
			{
				metadata = dict;
				isMetadataChanged = true;
			}
		}
	}
#endif

	if (!packet->buf || forceCopy) //Buffer isn't reference-counted, so copy the data
		encoded.assign(packet->data, packet->size, packet->size + FF_INPUT_BUFFER_PADDING_SIZE);
	else
	{
		encoded.assign(packet->buf, packet->size);
		packet->buf = nullptr;
	}

	const double time_base = av_q2d(streams.at(ff_idx)->time_base);

#ifndef MP3_FAST_SEEK
	if (seekByByteOffset < 0)
#endif
		encoded.ts.set(packet->dts * time_base, packet->pts * time_base, startTime);
#ifndef MP3_FAST_SEEK
	else if (packet->pos > -1 && length() > 0.0)
		lastTime = encoded.ts = ((packet->pos - seekByByteOffset) * length()) / (avio_size(formatCtx->pb) - seekByByteOffset);
	else
		encoded.ts = lastTime;
#endif

	if (packet->duration > 0)
		encoded.duration = packet->duration * time_base;
	else if (!encoded.ts || (encoded.duration = encoded.ts - streamsTS.at(ff_idx)) < 0.0 /* Calculate packet duration if doesn't exists */)
		encoded.duration = 0.0;
	streamsTS[ff_idx] = encoded.ts;

	if (isStreamed)
	{
		if (!isOneStreamOgg)
			encoded.ts += streamsOffset.at(ff_idx);
		else
		{
			encoded.ts = lastTime;
			lastTime += encoded.duration;
		}
	}

	currPos = encoded.ts;

	encoded.hasKeyFrame = packet->flags & AV_PKT_FLAG_KEY;
	if (streams.at(ff_idx)->sample_aspect_ratio.num)
		encoded.sampleAspectRatio = av_q2d(streams.at(ff_idx)->sample_aspect_ratio);

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
		"tty" //txt files
	};

	const QByteArray scheme = Functions::getUrlScheme(_url).toUtf8();
	if (scheme.isEmpty())
		return false;

	qint64 oggOffset = -1, oggSize = -1;
	int oggTrack = -1;
	QString url;

	if (param.startsWith("OGG:")) //For chained OGG files
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
		inputFmt = av_find_input_format(scheme);
		if (inputFmt)
			url = _url.right(_url.length() - scheme.length() - 3);
		isLocal = false;
	}

	AVDictionary *options = nullptr;
	if (!inputFmt)
		url = Functions::prepareFFmpegUrl(_url, options);

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

	OpenFmtCtxThr *openThr = new OpenFmtCtxThr(formatCtx, url.toUtf8(), inputFmt, options, abortCtx);
	formatCtx = openThr->getFormatCtx();
	openThr->drop();
	if (!formatCtx || disabledDemuxers.contains(name()))
		return false;

	if (name().startsWith("image2") || name().endsWith("_pipe"))
	{
		if (!QMPlay2Core.getSettings().getBool("StillImages"))
			return false;
		stillImage = true;
	}

#ifdef MP3_FAST_SEEK
	if (name() == "mp3")
		formatCtx->flags |= AVFMT_FLAG_FAST_SEEK; //This should be set before "avformat_open_input", but seems to be working for MP3...
#else
	seekByByteOffset = formatCtx->pb ? avio_tell(formatCtx->pb) : -1; //formatCtx->data_offset, moved to private since FFmpeg 2.6
#endif

	avcodec_mutex.lock();
	if (avformat_find_stream_info(formatCtx, nullptr) < 0)
	{
		avcodec_mutex.unlock();
		return false;
	}
	avcodec_mutex.unlock();

	isStreamed = !isLocal && formatCtx->duration <= 0; //QMPLAY2_NOPTS_VALUE is negative
#ifndef MP3_FAST_SEEK
	if (seekByByteOffset > -1 && (isStreamed || name() != "mp3"))
		seekByByteOffset = -1;
#endif

#ifdef QMPlay2_libavdevice
	forceCopy = name().contains("v4l2"); //Workaround for v4l2 - if many buffers are referenced demuxer doesn't produce proper timestamps (FFmpeg BUG?).
#else
	forceCopy = false;
#endif

	if ((startTime = formatCtx->start_time / (double)AV_TIME_BASE) < 0.0)
		startTime = 0.0;

	index_map.resize(formatCtx->nb_streams);
	streamsTS.resize(formatCtx->nb_streams);
	streamsOffset.resize(formatCtx->nb_streams);
	for (unsigned i = 0; i < formatCtx->nb_streams; ++i)
	{
		StreamInfo *streamInfo = getStreamInfo(formatCtx->streams[i]);
		if (!streamInfo)
			index_map[i] = -1;
		else
		{
			index_map[i] = streamsInfo.count();
			streamsInfo += streamInfo;
		}
#if LIBAVFORMAT_VERSION_MAJOR > 55
		if (!fixMkvAss && codecParams(formatCtx->streams[i])->codec_id == AV_CODEC_ID_ASS && !strncasecmp(formatCtx->iformat->name, "matroska", 8))
			fixMkvAss = true;
		formatCtx->streams[i]->event_flags = 0;
#endif
		streams += formatCtx->streams[i];

		TimeStamp ts;
		ts.set(0.0, 0.0);
		streamsTS[i] = ts;
	}
	if (streamsInfo.isEmpty())
		return false;

	isOneStreamOgg = (name() == "ogg" && streamsInfo.count() == 1); //Workaround for OGG network streams

	if (isStreamed && streamsInfo.count() == 1 && streamsInfo.at(0)->type == QMPLAY2_TYPE_SUBTITLE && formatCtx->pb && avio_size(formatCtx->pb) > 0)
		isStreamed = false; //Allow subtitles streams to be non-streamed if size is known

#if LIBAVFORMAT_VERSION_MAJOR > 55
	formatCtx->event_flags = 0;
#else
	if (!isStreamed)
		metadata = nullptr;
	else
	{
		char *value = nullptr;
		av_opt_get(formatCtx, "icy_metadata_headers", AV_OPT_SEARCH_CHILDREN, (quint8 **)&value);
		QStringList icyHeaders = QString(value).split("\n", QString::SkipEmptyParts);
		av_free(value);
		for (const QString &icy : icyHeaders)
		{
			if (icy.startsWith("icy-name: "))
				av_dict_set(&formatCtx->metadata, "icy-name", icy.mid(10).toUtf8(), 0);
			else if (icy.startsWith("icy-description: "))
				av_dict_set(&formatCtx->metadata, "icy-description", icy.mid(17).toUtf8(), 0);
		}
		metadata = getMetadata();
	}
#endif

	packet = FFCommon::createAVPacket();

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

	StreamInfo *streamInfo = new StreamInfo;

	if (const AVCodec *codec = avcodec_find_decoder(codecParams(stream)->codec_id))
		streamInfo->codec_name = codec->name;

	streamInfo->must_decode = true;
	if (const AVCodecDescriptor *codecDescr = avcodec_descriptor_get(codecParams(stream)->codec_id))
	{
		if (codecDescr->props & AV_CODEC_PROP_TEXT_SUB)
			streamInfo->must_decode = false;

		if (streamInfo->codec_name.isEmpty())
			streamInfo->codec_name = codecDescr->name;
	}

	streamInfo->bitrate = codecParams(stream)->bit_rate;
	streamInfo->bpcs = codecParams(stream)->bits_per_coded_sample;
	streamInfo->codec_tag = codecParams(stream)->codec_tag;
	streamInfo->is_default = stream->disposition & AV_DISPOSITION_DEFAULT;
	streamInfo->time_base.num = stream->time_base.num;
	streamInfo->time_base.den = stream->time_base.den;
	streamInfo->type = (QMPlay2MediaType)codecParams(stream)->codec_type; //Enumy są takie same

	if (codecParams(stream)->extradata_size)
	{
		streamInfo->data.reserve(codecParams(stream)->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
		streamInfo->data.resize(codecParams(stream)->extradata_size);
		memcpy(streamInfo->data.data(), codecParams(stream)->extradata, streamInfo->data.capacity());
		av_free(codecParams(stream)->extradata);
		codecParams(stream)->extradata = (quint8 *)streamInfo->data.data();
	}

	if (streamInfo->type != QMPLAY2_TYPE_ATTACHMENT)
	{
		QString value;
		if (streamsInfo.count() > 1)
		{
			streamInfo->title  = getTag(stream->metadata, "title");
			streamInfo->artist = getTag(stream->metadata, "artist");
			if (!(value = getTag(stream->metadata, "album")).isEmpty())
				streamInfo->other_info << qMakePair(QString::number(QMPLAY2_TAG_ALBUM), value);
			if (!(value = getTag(stream->metadata, "genre")).isEmpty())
				streamInfo->other_info << qMakePair(QString::number(QMPLAY2_TAG_GENRE), value);
			if (!(value = getTag(stream->metadata, "date")).isEmpty())
				streamInfo->other_info << qMakePair(QString::number(QMPLAY2_TAG_DATE), value);
			if (!(value = getTag(stream->metadata, "comment")).isEmpty())
				streamInfo->other_info << qMakePair(QString::number(QMPLAY2_TAG_COMMENT), value);
		}
		if (!(value = getTag(stream->metadata, "language", false)).isEmpty() && value != "und")
			streamInfo->other_info << qMakePair(QString::number(QMPLAY2_TAG_LANGUAGE), value);
	}

	switch (streamInfo->type)
	{
		case QMPLAY2_TYPE_AUDIO:
			streamInfo->format = getSampleFormat(stream);
			streamInfo->channels = codecParams(stream)->channels;
			streamInfo->sample_rate = codecParams(stream)->sample_rate;
			streamInfo->block_align = codecParams(stream)->block_align;
			break;
		case QMPLAY2_TYPE_VIDEO:
			streamInfo->format = getPixelFormat(stream);
			if (stream->sample_aspect_ratio.num)
				streamInfo->sample_aspect_ratio = av_q2d(stream->sample_aspect_ratio);
			else if (codecParams(stream)->sample_aspect_ratio.num)
				streamInfo->sample_aspect_ratio = av_q2d(codecParams(stream)->sample_aspect_ratio);
			streamInfo->W = codecParams(stream)->width;
			streamInfo->H = codecParams(stream)->height;
			if (!stillImage)
				streamInfo->FPS = av_q2d(stream->r_frame_rate);
			break;
		case AVMEDIA_TYPE_ATTACHMENT:
			streamInfo->title = getTag(stream->metadata, "filename", false);
			switch (codecParams(stream)->codec_id)
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
		default:
			break;
	}

	return streamInfo;
}
