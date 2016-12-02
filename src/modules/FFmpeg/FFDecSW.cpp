/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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
#include <VideoFrame.hpp>
#include <StreamInfo.hpp>
#include <Functions.hpp>

extern "C"
{
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavutil/pixdesc.h>
}

FFDecSW::FFDecSW(QMutex &avcodec_mutex, Module &module) :
	FFDec(avcodec_mutex),
	threads(0), lowres(0),
	thread_type_slice(false),
	lastFrameW(-1), lastFrameH(-1),
	sws_ctx(NULL),
	desiredPixFmt(-1)
{
	SetModule(module);
}
FFDecSW::~FFDecSW()
{
	clearBitmapSubsBuffer();
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

void FFDecSW::setSupportedPixelFormats(const QMPlay2PixelFormats &pixelFormats)
{
	supportedPixelFormats = pixelFormats;
	setPixelFormat();
}

int FFDecSW::decodeAudio(Packet &encodedPacket, Buffer &decoded, quint8 &channels, quint32 &sampleRate, bool flush)
{
	int bytes_consumed = 0, frameFinished = 0;

	decodeFirstStep(encodedPacket, flush);
	if (codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO)
	{
		bytes_consumed = avcodec_decode_audio4(codec_ctx, frame, &frameFinished, packet);
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
		decodeLastStep(encodedPacket, frame);
	else
		encodedPacket.ts.setInvalid();

	if (bytes_consumed < 0)
		bytes_consumed = 0;
	return bytes_consumed;
}
int FFDecSW::decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurry_up)
{
	int bytes_consumed = 0, frameFinished = 0;

	decodeFirstStep(encodedPacket, flush);

	if  (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		if (respectHurryUP && hurry_up)
		{
			if (skipFrames && !forceSkipFrames && hurry_up > 1)
				codec_ctx->skip_frame = AVDISCARD_NONREF;
			codec_ctx->skip_loop_filter = AVDISCARD_ALL;
			if (hurry_up > 1)
				codec_ctx->skip_idct = AVDISCARD_NONREF;
			codec_ctx->flags2 |= CODEC_FLAG2_FAST;
		}
		else
		{
			if (!forceSkipFrames)
				codec_ctx->skip_frame = AVDISCARD_DEFAULT;
			codec_ctx->skip_loop_filter = codec_ctx->skip_idct = AVDISCARD_DEFAULT;
			codec_ctx->flags2 &= ~CODEC_FLAG2_FAST;
		}

		bytes_consumed = avcodec_decode_video2(codec_ctx, frame, &frameFinished, packet);

		if (forceSkipFrames) //Nie możemy pomijać na pierwszej klatce, ponieważ wtedy może nie być odczytany przeplot
			codec_ctx->skip_frame = AVDISCARD_NONREF;

		if (frameFinished && ~hurry_up)
		{
			bool newFormat = false;
			if (codec_ctx->pix_fmt != lastPixFmt)
			{
				newPixFmt = av_get_pix_fmt_name(codec_ctx->pix_fmt);
				lastPixFmt = codec_ctx->pix_fmt;
				setPixelFormat();
				newFormat = true;
			}
			if (desiredPixFmt != AV_PIX_FMT_NONE)
			{
				const VideoFrameSize frameSize(frame->width, frame->height, chromaShiftW, chromaShiftH);
				if (dontConvert && frame->buf[0] && frame->buf[1] && frame->buf[2])
					decoded = VideoFrame(frameSize, frame->buf, frame->linesize, frame->interlaced_frame, frame->top_field_first);
				else
				{
					const int aligned8W = Functions::aligned(frame->width, 8);
					const int linesize[] = {
						aligned8W,
						aligned8W >> chromaShiftW,
						aligned8W >> chromaShiftW
					};
					decoded = VideoFrame(frameSize, linesize, frame->interlaced_frame, frame->top_field_first);
					if (frame->width != lastFrameW || frame->height != lastFrameH || newFormat)
					{
						sws_ctx = sws_getCachedContext(sws_ctx, frame->width, frame->height, codec_ctx->pix_fmt, frame->width, frame->height, (AVPixelFormat)desiredPixFmt, SWS_BILINEAR, NULL, NULL, NULL);
						lastFrameW = frame->width;
						lastFrameH = frame->height;
					}
					quint8 *decodedData[] = {
						decoded.buffer[0].data(),
						decoded.buffer[1].data(),
						decoded.buffer[2].data()
					};
					sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, decodedData, decoded.linesize);
				}
			}
		}
	}

	if (frameFinished)
		decodeLastStep(encodedPacket, frame);
	else
		encodedPacket.ts.setInvalid();

	return bytes_consumed < 0 ? 0 : bytes_consumed;
}
bool FFDecSW::decodeSubtitle(const Packet &encodedPacket, double pos, QMPlay2OSD *&osd, int w, int h)
{
	if (codec_ctx->codec_type != AVMEDIA_TYPE_SUBTITLE)
		return false;

	if (encodedPacket.isEmpty())
		return getFromBitmapSubsBuffer(osd, pos);

	decodeFirstStep(encodedPacket, false);

	int gotSubtitles = 0;
	AVSubtitle subtitle;
	if (avcodec_decode_subtitle2(codec_ctx, &subtitle, &gotSubtitles, packet) >= 0 && gotSubtitles && subtitle.format == 0)
	{
		const double pts = subtitle.start_display_time + encodedPacket.ts;
		if (!subtitle.num_rects)
			addBitmapSubBuffer(new BitmapSubBuffer(pts), pos); //If no bitmaps - disable subs at current packet PTS
		else for (unsigned i = 0; i < subtitle.num_rects; ++i)
		{
			const AVSubtitleRect *rect = subtitle.rects[i];

			BitmapSubBuffer *buff = new BitmapSubBuffer(pts, (subtitle.end_display_time - subtitle.start_display_time) / 1000.0);
			buff->w = av_clip(rect->w, 0, w);
			buff->h = av_clip(rect->h, 0, h);
			buff->x = av_clip(rect->x, 0, w - buff->w);
			buff->y = av_clip(rect->y, 0, h - buff->h);
			buff->bitmap.resize((buff->w * buff->h) << 2);

#if LIBAVCODEC_VERSION_MAJOR >= 57
			const uint8_t  *source   = (uint8_t  *)rect->data[0];
			const uint32_t *palette  = (uint32_t *)rect->data[1];
			const int       linesize = rect->linesize[0];
#else
			const uint8_t  *source   = (uint8_t  *)rect->pict.data[0];
			const uint32_t *palette  = (uint32_t *)rect->pict.data[1];
			const int       linesize = rect->pict.linesize[0];
#endif

			uint32_t       *dest     = (uint32_t *)buff->bitmap.data();
			for (int y = 0; y < buff->h; ++y)
				for (int x = 0; x < buff->w; ++x)
				{
					const uint32_t color = palette[source[y * linesize + x]];
					*(dest++) = (color & 0xFF00FF00) | ((color << 16) & 0x00FF0000) | ((color >> 16) & 0x000000FF);
				}

			addBitmapSubBuffer(buff, pos);
			getFromBitmapSubsBuffer(osd, pos);
		}
	}
	if (gotSubtitles)
		avsubtitle_free(&subtitle);

	return true;
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
		av_codec_set_lowres(codec_ctx, qMin(av_codec_get_max_lowres(codec), lowres));
		codec_ctx->refcounted_frames = true;
		lastPixFmt = codec_ctx->pix_fmt;
	}
	if (!FFDec::openCodec(codec))
		return false;
	time_base = streamInfo.getTimeBase();
	if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO && codec_ctx->lowres)
	{
		streamInfo.W = codec_ctx->width;
		streamInfo.H = codec_ctx->height;
	}
	return true;
}


void FFDecSW::setPixelFormat()
{
	const AVPixFmtDescriptor *pixDesc = av_pix_fmt_desc_get(codec_ctx->pix_fmt);
	if (!pixDesc) //Invalid pixel format
		return;
	dontConvert = supportedPixelFormats.contains((QMPlay2PixelFormat)codec_ctx->pix_fmt);
	if (dontConvert)
	{
		chromaShiftW = pixDesc->log2_chroma_w;
		chromaShiftH = pixDesc->log2_chroma_h;
		desiredPixFmt = codec_ctx->pix_fmt;
	}
	else for (int i = 0; i < supportedPixelFormats.count(); ++i)
	{
		const AVPixFmtDescriptor *supportedPixDesc = av_pix_fmt_desc_get((AVPixelFormat)supportedPixelFormats.at(i));
		if (i == 0 || (supportedPixDesc->log2_chroma_w == pixDesc->log2_chroma_w && supportedPixDesc->log2_chroma_h == pixDesc->log2_chroma_h))
		{
			//Use first format as default (mostly QMPLAY2_PIX_FMT_YUV420P) and look at next formats,
			//otherwise break the loop if found proper format.
			chromaShiftW = supportedPixDesc->log2_chroma_w;
			chromaShiftH = supportedPixDesc->log2_chroma_h;
			desiredPixFmt = supportedPixelFormats.at(i);
			if (i != 0)
				break;
		}
	}
}

inline void FFDecSW::addBitmapSubBuffer(BitmapSubBuffer *buff, double pos)
{
	if (buff->pts <= pos)
		clearBitmapSubsBuffer();
	bitmapSubsBuffer += buff;
}
bool FFDecSW::getFromBitmapSubsBuffer(QMPlay2OSD *&osd, double pos)
{
	bool cantDelete = true, doClear = true;
	for (int i = bitmapSubsBuffer.size() - 1; i >= 0 ; --i)
	{
		BitmapSubBuffer *buff = bitmapSubsBuffer.at(i);
		if (!buff->bitmap.isEmpty() && buff->pts + buff->duration < pos)
		{
			delete buff;
			bitmapSubsBuffer.removeAt(i);
			continue;
		}
		if (buff->pts <= pos)
		{
			if (buff->bitmap.isEmpty())
				cantDelete = false;
			else
			{
				const bool old_osd = osd;
				if (!old_osd)
					osd = new QMPlay2OSD;
				else
				{
					osd->lock();
					if (doClear)
						osd->clear();
				}
				osd->setDuration(buff->duration);
				osd->setPTS(buff->pts);
				osd->addImage(QRect(buff->x, buff->y, buff->w, buff->h), buff->bitmap);
				osd->setNeedsRescale();
				osd->genChecksum();
				if (old_osd)
					osd->unlock();
				cantDelete = true;
				doClear = false;
			}
			delete buff;
			bitmapSubsBuffer.removeAt(i);
		}
	}
	return cantDelete;
}
inline void FFDecSW::clearBitmapSubsBuffer()
{
	while (!bitmapSubsBuffer.isEmpty())
		delete bitmapSubsBuffer.takeFirst();
}
