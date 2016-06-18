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

#include <VideoThr.hpp>

#include <PlayClass.hpp>
#include <Main.hpp>

#include <VideoWriter.hpp>
#include <QMPlay2_OSD.hpp>
#include <VideoFrame.hpp>
#include <Settings.hpp>
#include <Decoder.hpp>
#include <LibASS.hpp>

#include <ImgScaler.hpp>
#include <Functions.hpp>
using Functions::gettime;

#include <QDebug>
#include <QImage>
#include <QDir>

#include <math.h>

#ifdef X11_RESET_SCREEN_SAVER
	#if QT_VERSION >= 0x050000
		#include <QGuiApplication>
	#endif
	#include <QLibrary>

	class X11ResetScreenSaver
	{
		typedef void *(*XOpenDisplayType)(const char *name);
		typedef int (*XResetScreenSaverType)(void *dpy);
		typedef int (*XCloseDisplayType)(void *dpy);
		typedef int (*XFlushType)(void *dpy);
		XOpenDisplayType XOpenDisplayFunc;
		XResetScreenSaverType XResetScreenSaverFunc;
		XCloseDisplayType XCloseDisplayFunc;
		XFlushType XFlushFunc;
	public:
		inline X11ResetScreenSaver() :
			disp(NULL)
		{
#if QT_VERSION >= 0x050000
			if (QGuiApplication::platformName() != "xcb")
				return;
#endif
			QLibrary libX11("X11");
			if (libX11.load())
			{
				XOpenDisplayFunc = (XOpenDisplayType)libX11.resolve("XOpenDisplay");
				XResetScreenSaverFunc = (XResetScreenSaverType)libX11.resolve("XResetScreenSaver");
				XFlushFunc = (XFlushType)libX11.resolve("XFlush");
				XCloseDisplayFunc = (XCloseDisplayType)libX11.resolve("XCloseDisplay");
				if (XOpenDisplayFunc && XResetScreenSaverFunc && XFlushFunc && XCloseDisplayFunc)
				{
					disp = XOpenDisplayFunc(NULL);
					lastT = gettime();
				}
			}
		}
		inline ~X11ResetScreenSaver()
		{
			if (disp)
				XCloseDisplayFunc(disp);
		}

		inline void reset()
		{
			const double t = gettime();
			if (t - lastT >= 0.75)
			{
				XResetScreenSaverFunc(disp);
				XFlushFunc(disp);
				lastT = t;
			}
		}

		double lastT;
		void *disp;
	};
#endif

VideoThr::VideoThr(PlayClass &playC, Writer *HWAccelWriter, const QStringList &pluginsName) :
	AVThread(playC, "video:", HWAccelWriter, pluginsName),
	doScreenshot(false),
	deleteOSD(false), deleteFrame(false),
	W(0), H(0),
	sDec(NULL),
	HWAccelWriter(HWAccelWriter),
	subtitles(NULL)
{
#ifdef X11_RESET_SCREEN_SAVER
	x11ResetScreenSaver.reset(new X11ResetScreenSaver);
	if (!x11ResetScreenSaver->disp)
		x11ResetScreenSaver.reset();
#endif
}
VideoThr::~VideoThr()
{
	delete playC.osd;
	playC.osd = NULL;
	delete subtitles;
	delete sDec;
}

QMPlay2PixelFormats VideoThr::getSupportedPixelFormats() const
{
	return videoWriter()->supportedPixelFormats();
}

void VideoThr::destroySubtitlesDecoder()
{
	deleteSubs = true;
	if (sDec)
	{
		delete sDec;
		sDec = NULL;
	}
}

bool VideoThr::setSpherical()
{
	return writer->modParam("Spherical", playC.spherical);
}
bool VideoThr::setFlip()
{
	return writer->modParam("Flip", playC.flip);
}
bool VideoThr::setRotate90()
{
	return writer->modParam("Rotate90", playC.rotate90);
}
void VideoThr::setVideoEqualizer()
{
	writer->modParam("Brightness", playC.Brightness);
	writer->modParam("Saturation", playC.Saturation);
	writer->modParam("Contrast", playC.Contrast);
	writer->modParam("Hue", playC.Hue);
}
void VideoThr::setFrameSize(int w, int h)
{
	W = w;
	H = h;
	writer->modParam("W", W);
	writer->modParam("H", H);
	deleteSubs = deleteOSD = deleteFrame = true;
}
void VideoThr::setARatio(double aRatio, double sar)
{
	updateSubs();
	writer->modParam("AspectRatio", aRatio);
	lastSampleAspectRatio = sar;
}
void VideoThr::setZoom()
{
	updateSubs();
	writer->modParam("Zoom", playC.zoom);
}

void VideoThr::initFilters(bool processParams)
{
	Settings &QMPSettings = QMPlay2Core.getSettings();

	if (processParams)
		filtersMutex.lock();

	filters.clear();

	if (QMPSettings.getBool("Deinterlace/ON")) //Deinterlacing filters as first
	{
		const bool autoDeint = QMPSettings.getBool("Deinterlace/Auto");
		const bool doubleFramerate = QMPSettings.getBool("Deinterlace/Doubler");
		const bool autoParity = QMPSettings.getBool("Deinterlace/AutoParity");
		const bool topFieldFirst = QMPSettings.getBool("Deinterlace/TFF");
		const quint8 deintFlags = autoDeint | doubleFramerate << 1 | autoParity << 2 | topFieldFirst << 3;
		bool HWDeint = false, PrepareForHWBobDeint = false;
		if ((HWDeint = writer->modParam("Deinterlace", 1 | deintFlags << 1)))
			PrepareForHWBobDeint = doubleFramerate && writer->getParam("PrepareForHWBobDeint").toBool();
		if (!HWDeint || PrepareForHWBobDeint)
		{
			const QString deintFilterName = PrepareForHWBobDeint ? "PrepareForHWBobDeint" : QMPSettings.getString("Deinterlace/SoftwareMethod");
			VideoFilter *deintFilter = filters.on(deintFilterName);
			bool ok = false;
			if (deintFilter && deintFilter->modParam("DeinterlaceFlags", deintFlags))
			{
				deintFilter->modParam("W", W);
				deintFilter->modParam("H", H);
				ok = deintFilter->processParams();
			}
			if (!ok)
			{
				if (deintFilter)
					filters.off(deintFilter);
				if (W > 0 && H > 0)
					QMPlay2Core.logError(tr("Cannot initialize the deinterlacing filter") + " \"" + deintFilterName + '"');
			}
		}
	}
	else
		writer->modParam("Deinterlace", 0);

	if (!HWAccelWriter)
		foreach (QString filterName, QMPSettings.get("VideoFilters").toStringList())
			if (filterName.left(1).toInt()) //if filter is enabled
			{
				VideoFilter *filter = filters.on((filterName = filterName.mid(1)));
				bool ok = false;
				if (filter)
				{
					filter->modParam("W", W);
					filter->modParam("H", H);
					if (!(ok = filter->processParams()))
						filters.off(filter);
				}
				if (!ok && W > 0 && H > 0)
					QMPlay2Core.logError(tr("Error initializing filter") + " \"" + filterName + '"');
			}

	if (processParams)
	{
		filtersMutex.unlock();
		if (writer->hasParam("Deinterlace"))
			writer->processParams();
	}

	filters.start();
}

bool VideoThr::processParams()
{
	return writer->processParams();
}

void VideoThr::updateSubs()
{
	if (playC.ass)
	{
		playC.subsMutex.lock();
		if (subtitles)
			playC.ass->getASS(subtitles, playC.frame_last_pts + playC.frame_last_delay  - playC.subtitlesSync);
		playC.subsMutex.unlock();
	}
}

inline VideoWriter *VideoThr::videoWriter() const
{
	return (VideoWriter *)writer;
}

void VideoThr::run()
{
	bool skip = false, paused = false, oneFrame = false, useLastDelay = false, lastOSDListEmpty = true, maybeFlush = false, lastAVDesync = false;
	double tmp_time = 0.0, sync_last_pts = 0.0, frame_timer = -1.0, sync_timer = 0.0;
	QMutex emptyBufferMutex;
	VideoFrame videoFrame;
	unsigned fast = 0;
	int tmp_br = 0, frames = 0;
	canWrite = true;

	while (!br)
	{
		if (deleteFrame)
		{
			videoFrame.clear();
			frame_timer = -1.0;
			deleteFrame = false;
		}

		if (doScreenshot && !videoFrame.isEmpty())
		{
			QMetaObject::invokeMethod(this, "screenshot", Q_ARG(VideoFrame, videoFrame));
			doScreenshot = false;
		}

		const bool mustFetchNewPacket = !filters.readyRead();
		playC.vPackets.lock();
		const bool hasVPackets = playC.vPackets.canFetch();
		if (maybeFlush)
			maybeFlush = playC.endOfStream && !hasVPackets;
		if ((playC.paused && !oneFrame) || (!(maybeFlush || hasVPackets) && mustFetchNewPacket) || playC.waitForData)
		{
			if (playC.paused && !paused)
			{
				QMetaObject::invokeMethod(this, "pause");
				paused = true;
				frame_timer = -1.0;
			}
			playC.vPackets.unlock();

			tmp_br = tmp_time = frames = 0;
			skip = false;
			fast = 0;

			if (!playC.paused)
				waiting = playC.fillBufferB = true;

			emptyBufferMutex.lock();
			playC.emptyBufferCond.wait(&emptyBufferMutex, MUTEXWAIT_TIMEOUT);
			emptyBufferMutex.unlock();

			if (frame_timer != -1.0)
				frame_timer = gettime();

			continue;
		}
		paused = waiting = false;
		Packet packet;
		if (hasVPackets && mustFetchNewPacket)
			packet = playC.vPackets.fetch();
		else
			packet.ts.setInvalid();
		playC.vPackets.unlock();
		if (playC.nextFrameB)
		{
			skip = playC.nextFrameB = false;
			oneFrame = playC.paused = true;
			fast = 0;
		}
		playC.fillBufferB = true;

		/* Subtitles packet */
		Packet sPacket;
		playC.sPackets.lock();
		if (playC.sPackets.canFetch())
			sPacket = playC.sPackets.fetch();
		playC.sPackets.unlock();

		mutex.lock();
		if (br)
		{
			mutex.unlock();
			break;
		}

		/* Subtitles */
		const double subsPts = playC.frame_last_pts + playC.frame_last_delay  - playC.subtitlesSync;
		QList<const QMPlay2_OSD *> osdList, osdListToDelete;
		playC.subsMutex.lock();
		if (sDec) //Image subs (pgssub, dvdsub, ...)
		{
			if (!sDec->decodeSubtitle(sPacket, subsPts, subtitles, W, H))
			{
				osdListToDelete += subtitles;
				subtitles = NULL;
			}
		}
		else
		{
			if (!sPacket.isEmpty())
			{
				const QByteArray sPacketData = QByteArray::fromRawData((const char *)sPacket.data(), sPacket.size());
				if (playC.ass->isASS())
					playC.ass->addASSEvent(sPacketData);
				else
					playC.ass->addASSEvent(Functions::convertToASS(sPacketData), sPacket.ts, sPacket.duration);
			}
			if (!playC.ass->getASS(subtitles, subsPts))
			{
				osdListToDelete += subtitles;
				subtitles = NULL;
			}
		}
		if (subtitles)
		{
			const bool hasDuration = subtitles->duration() >= 0.0;
			if (deleteSubs || (subtitles->isStarted() && subsPts < subtitles->pts()) || (hasDuration && subsPts > subtitles->pts() + subtitles->duration()))
			{
				osdListToDelete += subtitles;
				subtitles = NULL;
			}
			else if (subsPts >= subtitles->pts())
			{
				subtitles->start();
				osdList += subtitles;
			}
		}
		playC.subsMutex.unlock();
		playC.osdMutex.lock();
		if (playC.osd)
		{
			if (deleteOSD || playC.osd->left_duration() < 0)
			{
				osdListToDelete += playC.osd;
				playC.osd = NULL;
			}
			else
				osdList += playC.osd;
		}
		playC.osdMutex.unlock();
		if ((!lastOSDListEmpty || !osdList.isEmpty()) && writer->readyWrite())
		{
			videoWriter()->writeOSD(osdList);
			lastOSDListEmpty = osdList.isEmpty();
		}
		while (!osdListToDelete.isEmpty())
			delete osdListToDelete.takeFirst();
		deleteSubs = deleteOSD = false;
		/**/

		filtersMutex.lock();
		if (playC.flushVideo)
		{
			filters.clearBuffers();
			frame_timer = -1.0;
		}

		if (!packet.isEmpty() || maybeFlush)
		{
			VideoFrame decoded;
			QByteArray newPixelFormat;
			const int bytes_consumed = dec->decodeVideo(packet, decoded, newPixelFormat, playC.flushVideo, skip ? ~0 : (fast >> 1));
			if (!newPixelFormat.isEmpty())
			{
				emit playC.pixelFormatUpdate(newPixelFormat);
			}
			if (playC.flushVideo)
			{
				useLastDelay = true; //if seeking
				playC.flushVideo = false;
			}
			if (!decoded.isEmpty())
			{
				if (!HWAccelWriter && (decoded.size.width != W || decoded.size.height != H))
				{
					//Frame size has been changed
					filtersMutex.unlock();
					frameSizeUpdateMutex.lock();
					mutex.unlock();
					emit playC.frameSizeUpdate(decoded.size.width, decoded.size.height);
					frameSizeUpdateMutex.lock(); //Wait for "frameSizeUpdate()" to be finished
					mutex.lock();
					frameSizeUpdateMutex.unlock();
					filtersMutex.lock();
				}
				filters.addFrame(decoded, packet.ts);
			}
			else if (skip)
				filters.removeLastFromInputBuffer();
			tmp_br += bytes_consumed;
		}

		const bool ptsIsValid = filters.getFrame(videoFrame, packet.ts);
		filtersMutex.unlock();

		if ((maybeFlush = packet.ts.isValid()))
		{
			if (packet.sampleAspectRatio && lastSampleAspectRatio != -1.0 && !qFuzzyCompare(lastSampleAspectRatio, packet.sampleAspectRatio)) //Aspect ratio has been changed
			{
				lastSampleAspectRatio = -1.0; //Needs to be updated later
				emit playC.aRatioUpdate(packet.sampleAspectRatio); //Sets "lastSampleAspectRatio", because it calls "setARatio()";
			}
			if (ptsIsValid || packet.ts > playC.pos)
				playC.chPos(packet.ts);

			double delay = packet.ts - playC.frame_last_pts;
			if (useLastDelay || delay <= 0.0 || (playC.frame_last_pts <= 0.0 && delay > playC.frame_last_delay))
			{
				delay = playC.frame_last_delay;
				useLastDelay = false;
			}

			tmp_time += delay * 1000.0;
			frames += 1000;
			if (tmp_time >= 1000.0)
			{
				emit playC.updateBitrate(-1, round((tmp_br << 3) / tmp_time), frames / tmp_time);
				frames = tmp_br = tmp_time = 0;
			}

			delay /= playC.speed;

			playC.frame_last_delay = delay;
			playC.frame_last_pts = packet.ts;

			if (playC.skipAudioFrame < 0.0)
				playC.skipAudioFrame = 0.0;

			const double true_delay = delay;

			const double audioCurrentPts = playC.audio_current_pts;
			bool canSkipFrames = syncVtoA && audioCurrentPts > 0.0;

			if (canSkipFrames && !oneFrame)
			{
				double sync_pts = audioCurrentPts;
				if (sync_last_pts == sync_pts)
					sync_pts += gettime() - sync_timer;
				else
				{
					sync_last_pts = sync_pts;
					sync_timer = gettime();
				}

				const double diff = packet.ts - (delay + sync_pts - playC.videoSync);
				const double sync_threshold = qMax(delay, playC.audio_last_delay);
				const double max_threshold = qMax(sync_threshold * 2.0, 0.15);
				const double fDiff = qAbs(diff);

				if (fast && !skip && diff > -sync_threshold / 2.0)
					fast = 0;
				skip = false;

//				qDebug() << "diff:" << diff << "sync_threshold:" << sync_threshold << "max_threshold:" << max_threshold;
				if (fDiff > sync_threshold && fDiff < max_threshold)
				{
					if (fDiff > sync_threshold * 1.75)
					{
						delay += diff / 4.0;
						lastAVDesync = true;
//						qDebug() << "Syncing 2" << diff << delay << sync_threshold;
					}
					else
					{
						if (lastAVDesync)
						{
							delay += diff / 8.0;
//							qDebug() << "Syncing 1" << diff << delay << sync_threshold;
						}
					}
				}
				else if (fDiff >= max_threshold)
				{
					if (diff < 0.0) //obraz się spóźnia
					{
						delay = 0.0;
						if (fast >= 7)
							skip = true;
					}
					else if (diff > 0.0) //obraz idzie za szybko
					{
						if (diff <= 0.5)
							delay *= 2.0;
						else if (!playC.skipAudioFrame)
							playC.skipAudioFrame = diff;
					}
					lastAVDesync = true;
//					qDebug() << "Skipping" << diff << skip << fast << delay;
				}
				else
				{
					lastAVDesync = false;
				}
			}
			else if (audioCurrentPts <= 0.0 || oneFrame)
			{
				skip = false;
				fast = 0;
			}

			const bool hasFrame = !videoFrame.isEmpty();
			const bool hasFrameTimer = (frame_timer != -1.0);
			const bool updateFrameTimer = (hasFrame != hasFrameTimer);
			if (hasFrame)
			{
				if (hasFrameTimer)
				{
					const double frame_timer_2 = gettime();
					const double delay_diff = frame_timer_2 - frame_timer;
					const double desired_delay = delay;
					if (delay > 0.0)
						delay -= delay_diff;
					if (canSkipFrames && true_delay > 0.0 && delay_diff > true_delay)
						++fast;
					else if (fast && delay > 0.0)
					{
						if (delay > true_delay / 2.0)
							delay /= 2.0;
						if (fast & 1)
							--fast;
					}
					const double toSleep = delay;
					while (delay > 0.0 && !playC.paused && !br && !br2)
					{
						const double sleepTime = qMin(delay, 0.1);
						Functions::s_wait(sleepTime);
						delay -= sleepTime;
					}
					frame_timer = gettime();
					frame_timer -= frame_timer - frame_timer_2 - qMax(toSleep, -desired_delay);
				}
				if (!skip && canWrite)
				{
					oneFrame = canWrite = false;
					QMetaObject::invokeMethod(this, "write", Q_ARG(VideoFrame, videoFrame));
				}
			}
			if (updateFrameTimer)
				frame_timer = gettime();
		}

		mutex.unlock();
	}
}

void VideoThr::write(VideoFrame videoFrame)
{
	canWrite = true;
	if (writer->readyWrite())
		videoWriter()->writeVideo(videoFrame);
#ifdef X11_RESET_SCREEN_SAVER
	if (x11ResetScreenSaver)
		x11ResetScreenSaver->reset();
#endif
}
void VideoThr::screenshot(VideoFrame videoFrame)
{
	ImgScaler imgScaler;
	const int aligned8W = Functions::aligned(W, 8);
	if (imgScaler.create(!HWAccelWriter ? videoFrame.size : VideoFrameSize(W, H, 1, 1), aligned8W, H))
	{
		QImage img(aligned8W, H, QImage::Format_RGB32);
		bool ok = true;
		if (!HWAccelWriter)
			imgScaler.scale(videoFrame, img.bits());
		else
			ok = videoWriter()->HWAccellGetImg(videoFrame, img.bits(), &imgScaler);
		if (!ok)
			QMPlay2Core.logError(tr("Cannot create screenshot"));
		else
		{
			const QString ext = QMPlay2Core.getSettings().getString("screenshotFormat");
			const QString dir = QMPlay2Core.getSettings().getString("screenshotPth");
			quint16 num = 0;
			foreach (const QString &f, QDir(dir).entryList(QStringList("QMPlay2_snap_?????" + ext), QDir::Files, QDir::Name))
			{
				const quint16 n = f.mid(13, 5).toUShort();
				if (n > num)
					num = n;
			}
			img.save(dir + "/QMPlay2_snap_" + QString("%1").arg(++num, 5, 10, QChar('0')) + ext);
		}
	}
}
void VideoThr::pause()
{
	writer->pause();
}
