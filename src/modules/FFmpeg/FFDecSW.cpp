#include <FFDecSW.hpp>
#include <FFCommon.hpp>

#include <QMPlay2_OSD.hpp>
#include <VideoFrame.hpp>
#include <StreamInfo.hpp>

extern "C"
{
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
}

FFDecSW::FFDecSW(QMutex &avcodec_mutex, Module &module) :
	FFDec(avcodec_mutex),
	threads(0), lowres(0),
	thread_type_slice(false),
	lastFrameW(-1), lastFrameH(-1),
	sws_ctx(NULL)
{
	SetModule(module);
}
FFDecSW::~FFDecSW()
{
	while (!bitmapSubBuffer.isEmpty())
		delete bitmapSubBuffer.takeFirst();
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

int FFDecSW::decodeAudio(Packet &encodedPacket, Buffer &decoded, bool flush)
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
			if (decoded.size() != decoded_size)
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
int FFDecSW::decodeVideo(Packet &encodedPacket, VideoFrame &decoded, bool flush, unsigned hurry_up)
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
			if (codec_ctx->pix_fmt == AV_PIX_FMT_YUV420P && frame->width == streamInfo->W && frame->height == streamInfo->H)
				decoded = VideoFrame(streamInfo->H, streamInfo->H >> 1, frame->buf, frame->linesize, frame->interlaced_frame, frame->top_field_first);
			else
			{
				const int linesize[] = {streamInfo->W, streamInfo->W >> 1, streamInfo->W >> 1};
				decoded = VideoFrame(streamInfo->H, streamInfo->H >> 1, linesize, frame->interlaced_frame, frame->top_field_first);
				if (frame->width != lastFrameW || frame->height != lastFrameH)
				{
					sws_ctx = sws_getCachedContext(sws_ctx, frame->width, frame->height, codec_ctx->pix_fmt, streamInfo->W, streamInfo->H, AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
					lastFrameW = frame->width;
					lastFrameH = frame->height;
				}
				quint8 *decodedData[] = {decoded.buffer[0].data(), decoded.buffer[1].data(), decoded.buffer[2].data()};
				sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, decodedData, decoded.linesize);
			}
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
bool FFDecSW::decodeSubtitle(const Packet &encodedPacket, double pos, QMPlay2_OSD *&osd, int w, int h)
{
	if (codec_ctx->codec_type != AVMEDIA_TYPE_SUBTITLE)
		return false;

	if (encodedPacket.isEmpty())
		return getFromBitmapSubsBuffer(osd, pos);

	decodeFirstStep(encodedPacket, false);

	bool ret = true;
	int got_sub_ptr;
	AVSubtitle subtitle;
	if (avcodec_decode_subtitle2(codec_ctx, &subtitle, &got_sub_ptr, packet) && got_sub_ptr)
	{
		if (!subtitle.num_rects)
		{
			BitmapSubBuffer *buff = new BitmapSubBuffer;
			buff->x = buff->y = buff->w = buff->h = 0;
			buff->pts = subtitle.start_display_time + encodedPacket.ts;
			buff->duration = 0.0;
			bitmapSubBuffer += buff;
		}
		else for (unsigned i = 0; i < subtitle.num_rects; i++)
		{
			AVSubtitleRect *rect = subtitle.rects[i];
			switch (rect->type)
			{
				case SUBTITLE_BITMAP:
				{
					BitmapSubBuffer *buff = new BitmapSubBuffer;
					buff->duration = (subtitle.end_display_time - subtitle.start_display_time) / 1000.0;
					buff->pts = subtitle.start_display_time + encodedPacket.ts;
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
							dest[y * buff->w + x] = palette[source[y * linesize + x]];

					if (buff->pts <= pos)
						while (bitmapSubBuffer.size())
							delete bitmapSubBuffer.takeFirst();
					bitmapSubBuffer += buff;
					getFromBitmapSubsBuffer(osd, pos);
				} break;
				default:
					ret = false;
					break;
			}
		}
		avsubtitle_free(&subtitle);
	}

	return ret;
}

bool FFDecSW::open(StreamInfo *streamInfo, Writer *)
{
	AVCodec *codec = FFDec::init(streamInfo);
	if (!codec)
		return false;
	if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		if (codec_ctx->pix_fmt == AV_PIX_FMT_NONE || streamInfo->W <= 0 || streamInfo->H <= 0)
			return false;
		if (codec->capabilities & CODEC_CAP_DR1)
			codec_ctx->flags |= CODEC_FLAG_EMU_EDGE; //Does nothing since FFmpeg 2.2
		if ((codec_ctx->thread_count = threads) != 1)
		{
			if (!thread_type_slice)
				codec_ctx->thread_type = FF_THREAD_FRAME;
			else
				codec_ctx->thread_type = FF_THREAD_SLICE;
		}
		av_codec_set_lowres(codec_ctx, qMin(av_codec_get_max_lowres(codec), lowres));
		codec_ctx->refcounted_frames = true;
	}
	if (!FFDec::openCodec(codec))
		return false;
	if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		if (codec_ctx->lowres)
		{
			streamInfo->W = codec_ctx->width;
			streamInfo->H = codec_ctx->height;
		}
	}
	return true;
}


bool FFDecSW::getFromBitmapSubsBuffer(QMPlay2_OSD *&osd, double pos)
{
	bool cantDelete = true;
	for (int i = bitmapSubBuffer.size() - 1; i >= 0 ; --i)
	{
		BitmapSubBuffer *buff = bitmapSubBuffer[i];
		if (!buff->bitmap.isEmpty() && buff->pts + buff->duration < pos)
		{
			delete buff;
			bitmapSubBuffer.removeAt(i);
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
					osd = new QMPlay2_OSD;
				else
				{
					osd->lock();
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
			}
			delete buff;
			bitmapSubBuffer.removeAt(i);
		}
	}
	return cantDelete;
}
