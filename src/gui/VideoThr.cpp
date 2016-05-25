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

VideoThr::VideoThr(PlayClass &playC, Writer *HWAccelWriter, const QStringList &pluginsName) :
	AVThread(playC, "video:", HWAccelWriter, pluginsName),
	doScreenshot(false),
	deleteOSD(false), deleteFrame(false),
	W(0), H(0),
	sDec(NULL),
	HWAccelWriter(HWAccelWriter),
	subtitles(NULL)
{}
VideoThr::~VideoThr()
{
	delete playC.osd;
	playC.osd = NULL;
	delete subtitles;
	delete sDec;
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
	return writer ? writer->modParam("Spherical", playC.spherical) : false;
}
bool VideoThr::setFlip()
{
	return writer ? writer->modParam("Flip", playC.flip) : false;
}
bool VideoThr::setRotate90()
{
	return writer ? writer->modParam("Rotate90", playC.rotate90) : false;
}
void VideoThr::setVideoEqualizer()
{
	if (writer)
	{
		writer->modParam("Brightness", playC.Brightness);
		writer->modParam("Saturation", playC.Saturation);
		writer->modParam("Contrast", playC.Contrast);
		writer->modParam("Hue", playC.Hue);
	}
}
void VideoThr::setFrameSize(int w, int h)
{
	W = w;
	H = h;
	if (writer)
	{
		writer->modParam("W", W);
		writer->modParam("H", H);
	}
	deleteSubs = deleteOSD = deleteFrame = true;
}
void VideoThr::setARatio(double aRatio)
{
	updateSubs();
	if (writer)
		writer->modParam("AspectRatio", aRatio);
}
void VideoThr::setZoom()
{
	updateSubs();
	if (writer)
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
		if (writer && ((HWDeint = writer->modParam("Deinterlace", 1 | deintFlags << 1))))
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
				QMPlay2Core.logError(tr("Cannot initialize the deinterlacing filter") + " \"" + deintFilterName + '"');
			}
		}
	}
	else if (writer)
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
				if (!ok)
					QMPlay2Core.logError(tr("Error initializing filter") + " \"" + filterName + '"');
			}

	if (processParams)
	{
		filtersMutex.unlock();
		if (writer && writer->hasParam("Deinterlace"))
			writer->processParams();
	}

	filters.start();
}

bool VideoThr::processParams()
{
	if (writer)
	{
		lastSampleAspectRatio = writer->getParam("AspectRatio").toDouble() / (double)W * (double)H;
		return writer->processParams();
	}
	return false;
}

void VideoThr::updateSubs()
{
	if (writer && playC.ass)
	{
		playC.subsMutex.lock();
		if (subtitles)
			playC.ass->getASS(subtitles, playC.frame_last_pts + playC.frame_last_delay  - playC.subtitlesSync);
		playC.subsMutex.unlock();
	}
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
		if ((!lastOSDListEmpty || !osdList.isEmpty()) && writer && writer->readyWrite())
		{
			((VideoWriter *)writer)->writeOSD(osdList);
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
			const int bytes_consumed = dec->decodeVideo(packet, decoded, playC.flushVideo, skip ? ~0 : (fast >> 1));
			if (playC.flushVideo)
			{
				useLastDelay = true; //if seeking
				playC.flushVideo = false;
			}
			if (!decoded.isEmpty())
				filters.addFrame(decoded, packet.ts);
			else if (skip)
				filters.removeLastFromInputBuffer();
			tmp_br += bytes_consumed;
		}

		const bool ptsIsValid = filters.getFrame(videoFrame, packet.ts);
		filtersMutex.unlock();

		if ((maybeFlush = packet.ts.isValid()))
		{
			if (packet.sampleAspectRatio && lastSampleAspectRatio != -1.0 && fabs(lastSampleAspectRatio - packet.sampleAspectRatio) >= 0.000001) //zmiana współczynnika proporcji
			{
				lastSampleAspectRatio = -1.0; //Needs to be updated later
				emit playC.aRatioUpdate(packet.sampleAspectRatio * (double)W / (double)H); //Sets lastSampleAspectRatio because it calls processParams()
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

#if defined(Q_WS_X11) || defined(X11_EXTRAS)
	#ifdef X11_EXTRAS
		#include <QGuiApplication>
	#endif
	#include <QX11Info>
	#include <X11/Xlib.h>
#endif

void VideoThr::write(VideoFrame videoFrame)
{
#ifdef X11_EXTRAS
	static bool isXcb = (QGuiApplication::platformName() == "xcb");
	if (isXcb)
#endif
#if defined(Q_WS_X11) || defined(X11_EXTRAS)
		XResetScreenSaver(QX11Info::display());
#endif
	canWrite = true;
	if (writer && writer->readyWrite())
		((VideoWriter *)writer)->writeVideo(videoFrame);
}
void VideoThr::screenshot(VideoFrame videoFrame)
{
	ImgScaler imgScaler;
	const int aligned8W = Functions::aligned(W, 8);
	if (writer && imgScaler.create(W, H, aligned8W, H))
	{
		QImage img(aligned8W, H, QImage::Format_RGB32);
		bool ok = true;
		if (!HWAccelWriter)
			imgScaler.scale(videoFrame, img.bits());
		else
			ok = ((VideoWriter *)writer)->HWAccellGetImg(videoFrame, img.bits(), &imgScaler);
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
	if (writer)
		writer->pause();
}
