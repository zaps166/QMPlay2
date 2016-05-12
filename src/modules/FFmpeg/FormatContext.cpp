#include <FFCommon.hpp>
#include <FormatContext.hpp>

#include <QMPlay2Core.hpp>
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

static int interruptCB(bool &aborted)
{
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

/**/

FormatContext::FormatContext(QMutex &avcodec_mutex) :
	isError(false),
	currPos(0.0),
	formatCtx(NULL),
	packet(NULL),
	isPaused(false), isAborted(false), fixMkvAss(false),
	isMetadataChanged(false),
	lastTime(0.0),
	lastErr(0), errFromSeek(0),
	maybeHasFrame(false),
	avcodec_mutex(avcodec_mutex)
{}
FormatContext::~FormatContext()
{
	if (formatCtx)
	{
		foreach (AVStream *stream, streams)
			if (stream->codec)
				switch ((quintptr)stream->codec->opaque)
				{
					case 1:
						stream->codec->extradata = NULL;
						stream->codec->extradata_size = 0;
						break;
					case 2:
						stream->codec->subtitle_header = NULL;
						stream->codec->subtitle_header_size = 0;
						break;
				}
		avformat_close_input(&formatCtx);
		FFCommon::freeAVPacket(packet);
	}
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

QList<ChapterInfo> FormatContext::getChapters() const
{
	QList<ChapterInfo> chapters;
	for (unsigned i = 0; i < formatCtx->nb_chapters; i++)
	{
		AVChapter &chapter = *formatCtx->chapters[i];
		ChapterInfo chapterInfo(chapter.start * chapter.time_base.num / (double)chapter.time_base.den, chapter.end * chapter.time_base.num / (double)chapter.time_base.den);
		if (AVDictionaryEntry *avtag = av_dict_get(chapter.metadata, "title", NULL, AV_DICT_IGNORE_SUFFIX))
			chapterInfo.title = avtag->value;
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
	AVDictionaryEntry *avtag;
	if (isStreamed && (avtag = av_dict_get(formatCtx->metadata, "icy-name", NULL, AV_DICT_IGNORE_SUFFIX)))
		return avtag->value;
	if (AVDictionary *dict = getMetadata())
	{
		QString title, artist;
		if ((avtag = av_dict_get(dict, "title", NULL, AV_DICT_IGNORE_SUFFIX)))
			title = avtag->value;
		if ((avtag = av_dict_get(dict, "artist", NULL, AV_DICT_IGNORE_SUFFIX)))
			artist = avtag->value;
		if (!title.simplified().isEmpty() && !artist.simplified().isEmpty())
			return artist + " - " + title;
		else if (title.simplified().isEmpty() && !artist.simplified().isEmpty())
			return artist;
		else if (!title.simplified().isEmpty() && artist.simplified().isEmpty())
			return title;
	}
	return QString();
}
QList<QMPlay2Tag> FormatContext::tags() const
{
	QList<QMPlay2Tag> tagList;
	AVDictionaryEntry *avtag;
	QString value;
	if (isStreamed)
	{
		if ((avtag = av_dict_get(formatCtx->metadata, "icy-name", NULL, AV_DICT_IGNORE_SUFFIX)) && !(value = avtag->value).simplified().isEmpty())
			tagList << qMakePair(QString::number(QMPLAY2_TAG_NAME), value);
		if ((avtag = av_dict_get(formatCtx->metadata, "icy-description", NULL, AV_DICT_IGNORE_SUFFIX)) && !(value = avtag->value).simplified().isEmpty())
			tagList << qMakePair(QString::number(QMPLAY2_TAG_DESCRIPTION), value);
	}
	if (isStreamed && (avtag = av_dict_get(formatCtx->metadata, "StreamTitle", NULL, AV_DICT_IGNORE_SUFFIX)) && !(value = avtag->value).simplified().isEmpty())
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
		if ((avtag = av_dict_get(dict, "title", NULL, AV_DICT_IGNORE_SUFFIX)) && !(value = avtag->value).simplified().isEmpty())
			tagList << qMakePair(QString::number(QMPLAY2_TAG_TITLE), value);
		if ((avtag = av_dict_get(dict, "artist", NULL, AV_DICT_IGNORE_SUFFIX)) && !(value = avtag->value).simplified().isEmpty())
			tagList << qMakePair(QString::number(QMPLAY2_TAG_ARTIST), value);
		if ((avtag = av_dict_get(dict, "album", NULL, AV_DICT_IGNORE_SUFFIX)) && !(value = avtag->value).simplified().isEmpty())
			tagList << qMakePair(QString::number(QMPLAY2_TAG_ALBUM), value);
		if ((avtag = av_dict_get(dict, "genre", NULL, AV_DICT_IGNORE_SUFFIX)) && !(value = avtag->value).simplified().isEmpty())
			tagList << qMakePair(QString::number(QMPLAY2_TAG_GENRE), value);
		if ((avtag = av_dict_get(dict, "date", NULL, AV_DICT_IGNORE_SUFFIX)) && !(value = avtag->value).simplified().isEmpty())
			tagList << qMakePair(QString::number(QMPLAY2_TAG_DATE), value);
		if ((avtag = av_dict_get(dict, "comment", NULL, AV_DICT_IGNORE_SUFFIX)) && !(value = avtag->value).simplified().isEmpty())
			tagList << qMakePair(QString::number(QMPLAY2_TAG_COMMENT), value);
	}
	return tagList;
}
bool FormatContext::getReplayGain(bool album, float &gain_db, float &peak) const
{
#ifdef HAS_REPLAY_GAIN
	const int streamIdx = av_find_best_stream(formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (streamIdx > -1)
	{
		if (void *sideData = av_stream_get_side_data(streams[streamIdx], AV_PKT_DATA_REPLAYGAIN, NULL))
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
		AVDictionaryEntry *avtag;
		QString album_gain_db, album_peak, track_gain_db, track_peak;

		if ((avtag = av_dict_get(dict, "REPLAYGAIN_ALBUM_GAIN", NULL, AV_DICT_IGNORE_SUFFIX)) && avtag->value)
			album_gain_db = avtag->value;
		if ((avtag = av_dict_get(dict, "REPLAYGAIN_ALBUM_PEAK", NULL, AV_DICT_IGNORE_SUFFIX)) && avtag->value)
			album_peak = avtag->value;
		if ((avtag = av_dict_get(dict, "REPLAYGAIN_TRACK_GAIN", NULL, AV_DICT_IGNORE_SUFFIX)) && avtag->value)
			track_gain_db = avtag->value;
		if ((avtag = av_dict_get(dict, "REPLAYGAIN_TRACK_PEAK", NULL, AV_DICT_IGNORE_SUFFIX)) && avtag->value)
			track_peak = avtag->value;

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
	if (!isStreamed && formatCtx->duration != QMPLAY2_NOPTS_VALUE)
		return formatCtx->duration / (double)AV_TIME_BASE;
	return -1.0;
}
int FormatContext::bitrate() const
{
	return formatCtx->bit_rate / 1000;
}
QByteArray FormatContext::image(bool forceCopy) const
{
	foreach (AVStream *stream, streams)
		if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC)
			return forceCopy ? QByteArray((const char *)stream->attached_pic.data, stream->attached_pic.size) : QByteArray::fromRawData((const char *)stream->attached_pic.data, stream->attached_pic.size);
	return QByteArray();
}

bool FormatContext::seek(int pos, bool backward)
{
	bool isOk = false;
	isAborted = false;
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
	if (isAborted)
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
		if (lastErr != AVERROR_INVALIDDATA)
		{
			lastErr = AVERROR_INVALIDDATA;
			return true;
		}
		isError = true;
		return false;
	}
	else
		lastErr = 0;
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
	if (fixMkvAss && streams.at(ff_idx)->codec->codec_id == AV_CODEC_ID_ASS)
		matroska_fix_ass_packet(streams.at(ff_idx)->time_base, packet);
#else
	if (isStreamed)
	{
		char *value = NULL;
		av_opt_get(formatCtx, "icy_metadata_packet", AV_OPT_SEARCH_CHILDREN, (quint8 **)&value);
		QString icyPacket = value;
		av_free(value);
		int idx = icyPacket.indexOf("StreamTitle='");
		if (idx > -1)
		{
			int idx2 = icyPacket.indexOf("';", idx += 13);
			if (idx2 > -1)
			{
				AVDictionaryEntry *e = av_dict_get(formatCtx->metadata, "StreamTitle", NULL, AV_DICT_IGNORE_SUFFIX);
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
		packet->buf = NULL;
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
	isAborted = true;
}

bool FormatContext::open(const QString &_url)
{
	static const QStringList disabledDemuxers = QStringList()
		<< "bmp_pipe"
		<< "dxp_pipe"
		<< "exr_pipe"
		<< "j2k_pipe"
		<< "jpeg_pipe"
		<< "jpegls_pipe"
		<< "pictor_pipe"
		<< "png_pipe"
		<< "sgi_pipe"
		<< "sunrast_pipe"
		<< "tiff_pipe"
		<< "webp_pipe"
		<< "image2"
		<< "tty" //txt files
	;

	const int idx = _url.indexOf("://");
	if (idx < 0)
		return false;

	const QByteArray scheme = _url.left(idx).toLower().toUtf8();
	QString url;

	AVInputFormat *inputFmt = NULL;
	if (scheme == "file")
		isLocal = true;
	else
	{
		inputFmt = av_find_input_format(scheme);
		if (inputFmt)
			url = _url.right(_url.length() - scheme.length() - 3);
		isLocal = false;
	}

	AVDictionary *options = NULL;
	if (!inputFmt)
		url = FFCommon::prepareUrl(_url, options);

	formatCtx = avformat_alloc_context();
	formatCtx->interrupt_callback.callback = (int(*)(void *))interruptCB;
	formatCtx->interrupt_callback.opaque = &isAborted;

	if (avformat_open_input(&formatCtx, url.toUtf8(), inputFmt, &options) || !formatCtx || disabledDemuxers.contains(name()))
		return false;

#ifdef MP3_FAST_SEEK
	if (name() == "mp3")
		formatCtx->flags |= AVFMT_FLAG_FAST_SEEK; //This should be set before "avformat_open_input", but seems to be working for MP3...
#else
	seekByByteOffset = formatCtx->pb ? avio_tell(formatCtx->pb) : -1; //formatCtx->data_offset, moved to private since FFmpeg 2.6
#endif

	avcodec_mutex.lock();
	if (avformat_find_stream_info(formatCtx, NULL) < 0)
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

	isOneStreamOgg = name() == "ogg" && formatCtx->nb_streams == 1; //Workaround for some OGG network streams
#ifdef QMPlay2_libavdevice
	forceCopy = name().contains("v4l2"); //Workaround for v4l2 - if many buffers are referenced demuxer doesn't produce proper timestamps (FFmpeg BUG?).
#elif
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
		if (!fixMkvAss && formatCtx->streams[i]->codec->codec_id == AV_CODEC_ID_ASS && !strncasecmp(formatCtx->iformat->name, "matroska", 8))
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

	if (isStreamed && streamsInfo.count() == 1 && streamsInfo.at(0)->type == QMPLAY2_TYPE_SUBTITLE && formatCtx->pb && avio_size(formatCtx->pb) > 0)
		isStreamed = false; //Allow subtitles streams to be non-streamed if size is known

#if LIBAVFORMAT_VERSION_MAJOR > 55
	formatCtx->event_flags = 0;
#else
	if (!isStreamed)
		metadata = NULL;
	else
	{
		char *value = NULL;
		av_opt_get(formatCtx, "icy_metadata_headers", AV_OPT_SEARCH_CHILDREN, (quint8 **)&value);
		QStringList icyHeaders = QString(value).split("\n", QString::SkipEmptyParts);
		av_free(value);
		foreach (const QString &icy, icyHeaders)
		{
			if (icy.left(10) == "icy-name: ")
				av_dict_set(&formatCtx->metadata, "icy-name", icy.mid(10).toUtf8(), 0);
			else if (icy.left(17) == "icy-description: ")
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
	const AVCodecID codecID = stream->codec->codec_id;
	if
	(
		(stream->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
		(stream->codec->codec_type == AVMEDIA_TYPE_DATA)    ||
		(stream->codec->codec_type == AVMEDIA_TYPE_ATTACHMENT && codecID != AV_CODEC_ID_TTF && codecID != AV_CODEC_ID_OTF)
	) return NULL;

	StreamInfo *streamInfo = new StreamInfo;

	if (AVCodec *codec = avcodec_find_decoder(codecID))
		streamInfo->codec_name = codec->name;

	streamInfo->must_decode = true;
	if (const AVCodecDescriptor *codecDescr = avcodec_descriptor_get(codecID))
	{
		if (codecDescr->props & AV_CODEC_PROP_TEXT_SUB)
			streamInfo->must_decode = false;
	}

	streamInfo->bitrate = stream->codec->bit_rate;
	streamInfo->bpcs = stream->codec->bits_per_coded_sample;
	streamInfo->codec_tag = stream->codec->codec_tag;
	streamInfo->is_default = stream->disposition & AV_DISPOSITION_DEFAULT;
	streamInfo->time_base.num = stream->time_base.num;
	streamInfo->time_base.den = stream->time_base.den;
	streamInfo->type = (QMPlay2MediaType)stream->codec->codec_type; //Enumy są takie same

	if (streamInfo->type != QMPLAY2_TYPE_SUBTITLE && stream->codec->extradata_size)
	{
		streamInfo->data.reserve(stream->codec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
		streamInfo->data.resize(stream->codec->extradata_size);
		memcpy(streamInfo->data.data(), stream->codec->extradata, streamInfo->data.capacity());
		av_free(stream->codec->extradata);
		stream->codec->extradata = (quint8 *)streamInfo->data.data();
		stream->codec->opaque = (void *)1;
	}

	AVDictionaryEntry *avtag;
	if (streamInfo->type != QMPLAY2_TYPE_ATTACHMENT)
	{
		if (streamsInfo.count() > 1)
		{
			if ((avtag = av_dict_get(stream->metadata, "title", NULL, AV_DICT_IGNORE_SUFFIX)))
				streamInfo->title = avtag->value;
			if ((avtag = av_dict_get(stream->metadata, "artist", NULL, AV_DICT_IGNORE_SUFFIX)))
				streamInfo->artist = avtag->value;
			if ((avtag = av_dict_get(stream->metadata, "album", NULL, AV_DICT_IGNORE_SUFFIX)))
				streamInfo->other_info << qMakePair(QString::number(QMPLAY2_TAG_ALBUM), QString(avtag->value));
			if ((avtag = av_dict_get(stream->metadata, "genre", NULL, AV_DICT_IGNORE_SUFFIX)))
				streamInfo->other_info << qMakePair(QString::number(QMPLAY2_TAG_GENRE), QString(avtag->value));
			if ((avtag = av_dict_get(stream->metadata, "date", NULL, AV_DICT_IGNORE_SUFFIX)))
				streamInfo->other_info << qMakePair(QString::number(QMPLAY2_TAG_DATE), QString(avtag->value));
			if ((avtag = av_dict_get(stream->metadata, "comment", NULL, AV_DICT_IGNORE_SUFFIX)))
				streamInfo->other_info << qMakePair(QString::number(QMPLAY2_TAG_COMMENT), QString(avtag->value));
		}
		if ((avtag = av_dict_get(stream->metadata, "language", NULL, AV_DICT_IGNORE_SUFFIX)))
		{
			const QString value = avtag->value;
			if (value != "und")
				streamInfo->other_info << qMakePair(QString::number(QMPLAY2_TAG_LANGUAGE), value);
		}
	}

	switch (streamInfo->type)
	{
		case QMPLAY2_TYPE_AUDIO:
			streamInfo->channels = stream->codec->channels;
			streamInfo->sample_rate = stream->codec->sample_rate;
			streamInfo->block_align = stream->codec->block_align;
			streamInfo->other_info << qMakePair(tr("format"), QString(av_get_sample_fmt_name(stream->codec->sample_fmt)));
			break;
		case QMPLAY2_TYPE_VIDEO:
			streamInfo->aspect_ratio = 1.0;
			if (stream->sample_aspect_ratio.num)
				streamInfo->aspect_ratio = av_q2d(stream->sample_aspect_ratio);
			else if (stream->codec->sample_aspect_ratio.num)
				streamInfo->aspect_ratio = av_q2d(stream->codec->sample_aspect_ratio);
			if (streamInfo->aspect_ratio <= 0.0)
				streamInfo->aspect_ratio = 1.0;
			if (stream->codec->width > 0 && stream->codec->height > 0)
				streamInfo->aspect_ratio *= (double)stream->codec->width / (double)stream->codec->height;
			streamInfo->W = stream->codec->width;
			streamInfo->H = stream->codec->height;
			streamInfo->img_fmt = stream->codec->pix_fmt;
			streamInfo->FPS = av_q2d(stream->r_frame_rate);
			streamInfo->other_info << qMakePair(tr("format"), QString(av_get_pix_fmt_name(stream->codec->pix_fmt)));
			break;
		case QMPLAY2_TYPE_SUBTITLE:
			if (stream->codec->subtitle_header_size)
			{
				streamInfo->data = QByteArray((char *)stream->codec->subtitle_header, stream->codec->subtitle_header_size);
				av_free(stream->codec->subtitle_header);
				stream->codec->subtitle_header = (quint8 *)streamInfo->data.data();
				stream->codec->opaque = (void *)2;
			}
			break;
		case AVMEDIA_TYPE_ATTACHMENT:
			if ((avtag = av_dict_get(stream->metadata, "filename", NULL, AV_DICT_IGNORE_SUFFIX)))
				streamInfo->title = avtag->value;
			switch (codecID)
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
