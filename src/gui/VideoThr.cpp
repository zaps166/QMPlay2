/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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

#include <VideoAdjustmentW.hpp>
#include <PlayClass.hpp>
#include <Main.hpp>

#include <VideoWriter.hpp>
#include <QMPlay2OSD.hpp>
#include <VideoFrame.hpp>
#include <Settings.hpp>
#include <Decoder.hpp>
#include <LibASS.hpp>

#include <ScreenSaver.hpp>
#include <ImgScaler.hpp>
#include <Functions.hpp>
using Functions::gettime;

#include <QDebug>
#include <QImage>
#include <QDir>

#include <cmath>

VideoThr::VideoThr(PlayClass &playC, VideoWriter *hwAccelWriter, const QStringList &pluginsName) :
	AVThread(playC, "video:", hwAccelWriter, pluginsName),
	doScreenshot(false),
	deleteOSD(false), deleteFrame(false), gotFrameOrError(false), decoderError(false),
	W(0), H(0), seq(0),
	sDec(nullptr),
	hwAccelWriter(hwAccelWriter),
	subtitles(nullptr)
{
	maybeStartThread();
}
VideoThr::~VideoThr()
{
	QMPlay2GUI.videoAdjustment->enableControls();
	QMPlay2GUI.screenSaver->unInhibit(0);
	delete playC.osd;
	playC.osd = nullptr;
	delete subtitles;
	delete sDec;
}

void VideoThr::stop(bool terminate)
{
	playC.videoSeekPos = -1;
	AVThread::stop(terminate);
}

bool VideoThr::hasDecoderError() const
{
	return decoderError;
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
		sDec = nullptr;
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
void VideoThr::setVideoAdjustment()
{
	QMPlay2GUI.videoAdjustment->setModuleParam(writer);
}
void VideoThr::setFrameSize(int w, int h)
{
	W = w;
	H = h;
	writer->modParam("W", W);
	writer->modParam("H", H);
	deleteSubs = deleteOSD = deleteFrame = true;
	gotFrameOrError = false;
	++seq;
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
void VideoThr::otherReset()
{
	if (writer->hasParam("ResetOther"))
		writer->modParam("ResetOther", true);
}

void VideoThr::initFilters(bool processParams)
{
	Settings &QMPSettings = QMPlay2Core.getSettings();

	if (processParams)
		filtersMutex.lock();

	filters.clear();

	if (!playC.stillImage && QMPSettings.getBool("Deinterlace/ON")) //Deinterlacing filters as first
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
			if (!deintFilterName.isEmpty())
			{
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
						QMPlay2Core.logError(tr("Cannot initialize the deinterlacing filter") + " \"" + deintFilterName + '"', true, true);
				}
			}
		}
	}
	else
	{
		writer->modParam("Deinterlace", 0);
	}

	if (!playC.stillImage && !hwAccelWriter)
	{
		for (QString filterName : QMPSettings.getStringList("VideoFilters"))
		{
			if (filterName.leftRef(1).toInt()) //if filter is enabled
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
		}
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
	bool skip = false, paused = false, oneFrame = false, useLastDelay = false, lastOSDListEmpty = true, maybeFlush = false, lastAVDesync = false, interlaced = false, err = false;
	double tmp_time = 0.0, sync_last_pts = 0.0, frame_timer = -1.0, sync_timer = 0.0, framesDisplayedTime = 0.0;
	QMutex emptyBufferMutex;
	VideoFrame videoFrame;
	unsigned fast = 0;
	int tmp_br = 0, frames = 0, framesDisplayed = 0;
	canWrite = true;

	const auto resetVariables = [&] {
		tmp_br = tmp_time = frames = 0;
		skip = false;
		fast = 0;

		if (frame_timer != -1.0)
			frame_timer = gettime();
	};

	const auto processOneFrame = [&] {
		if (playC.nextFrameB && playC.seekTo < 0.0 && playC.videoSeekPos <= 0.0)
		{
			skip = playC.nextFrameB = false;
			oneFrame = playC.paused = true;
			fast = 0;
			return true;
		}
		return false;
	};

	const auto finishAccurateSeek = [&] {
		resetVariables();
		playC.videoSeekPos = -1.0;
		playC.emptyBufferCond.wakeAll();
	};

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
		if (maybeFlush || (!gotFrameOrError && !err && mustFetchNewPacket))
			maybeFlush = playC.endOfStream && !hasVPackets;
		err = false;
		if ((playC.paused && !oneFrame) || (!(maybeFlush || hasVPackets) && mustFetchNewPacket) || playC.waitForData || (playC.videoSeekPos <= 0.0 && playC.audioSeekPos > 0.0) || decoderError)
		{
			if (playC.paused && !paused)
			{
				QMetaObject::invokeMethod(this, "pause");
				paused = true;
				frame_timer = -1.0;
				emit playC.updateBitrateAndFPS(-1, -1, -1.0, 0.0, interlaced); //Set real FPS to 0 on pause
			}
			playC.vPackets.unlock();

			if (!playC.paused)
				waiting = playC.fillBufferB = true;

			emptyBufferMutex.lock();
			playC.emptyBufferCond.wait(&emptyBufferMutex, MUTEXWAIT_TIMEOUT);
			emptyBufferMutex.unlock();

			resetVariables();

			continue;
		}
		paused = waiting = false;
		Packet packet;
		if (hasVPackets && mustFetchNewPacket)
			packet = playC.vPackets.fetch();
		else
			packet.ts.setInvalid();
		playC.vPackets.unlock();
		processOneFrame();
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
		QList<const QMPlay2OSD *> osdList, osdListToDelete;
		playC.subsMutex.lock();
		if (sDec) //Image subs (pgssub, dvdsub, ...)
		{
			if (!sDec->decodeSubtitle(sPacket, subsPts, subtitles, W, H))
			{
				osdListToDelete += subtitles;
				subtitles = nullptr;
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
				subtitles = nullptr;
			}
		}
		if (subtitles)
		{
			const bool hasDuration = subtitles->duration() >= 0.0;
			if (deleteSubs || (subtitles->isStarted() && subsPts < subtitles->pts()) || (hasDuration && subsPts > subtitles->pts() + subtitles->duration()))
			{
				osdListToDelete += subtitles;
				subtitles = nullptr;
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
			if (deleteOSD || playC.osd->leftDuration() < 0)
			{
				osdListToDelete += playC.osd;
				playC.osd = nullptr;
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
				emit playC.pixelFormatUpdate(newPixelFormat);
			if (playC.flushVideo)
			{
				useLastDelay = true; //if seeking
				playC.flushVideo = false;
			}
			if (playC.videoSeekPos > 0.0 && bytes_consumed <= 0 && !packet.ts.isValid() && decoded.isEmpty())
				finishAccurateSeek();
			if (!decoded.isEmpty())
			{
				if (decoded.size.width != W || decoded.size.height != H)
				{
					//Frame size has been changed
					filtersMutex.unlock();
					updateMutex.lock();
					mutex.unlock();
					emit playC.frameSizeUpdate(decoded.size.width, decoded.size.height);
					updateMutex.lock(); //Wait for "frameSizeUpdate()" to be finished
					mutex.lock();
					updateMutex.unlock();
					filtersMutex.lock();
				}
				interlaced = decoded.interlaced;
				filters.addFrame(decoded, packet.ts);
				gotFrameOrError = true;
			}
			else if (skip)
				filters.removeLastFromInputBuffer();
			if (bytes_consumed < 0)
			{
				gotFrameOrError = true;
				err = true;
			}
			else
			{
				tmp_br += bytes_consumed;
			}
		}

		// This thread will wait for "DemuxerThr" which'll detect this error and restart with new decoder.
		decoderError = (dec->hasCriticalError() || videoWriter()->hwAccelError());

		const bool ptsIsValid = filters.getFrame(videoFrame, packet.ts);
		filtersMutex.unlock();

		if ((maybeFlush = packet.ts.isValid()))
		{
			if (packet.sampleAspectRatio && lastSampleAspectRatio != -1.0 && !qFuzzyCompare(lastSampleAspectRatio, packet.sampleAspectRatio)) //Aspect ratio has been changed
			{
				lastSampleAspectRatio = -1.0; //Needs to be updated later
				emit playC.aRatioUpdate(packet.sampleAspectRatio); //Sets "lastSampleAspectRatio", because it calls "setARatio()";
			}

			if (playC.videoSeekPos > 0.0)
			{
				bool cont = true;
				if (packet.ts >= playC.videoSeekPos)
				{
					finishAccurateSeek();
					if (processOneFrame())
						playC.fillBufferB = true;
					if (playC.audioSeekPos <= 0.0 || oneFrame)
						cont = false; // Play only if audio is ready or if still frame should be displayed
				}
				if (cont)
				{
					mutex.unlock();
					continue;
				}
			}

			if (ptsIsValid || packet.ts > playC.pos)
				playC.chPos(packet.ts);

			double delay = packet.ts - playC.frame_last_pts;
			if (useLastDelay || delay <= 0.0 || (playC.frame_last_pts <= 0.0 && delay > playC.frame_last_delay))
			{
				delay = playC.frame_last_delay;
				useLastDelay = false;
			}

			tmp_time += delay;
			++frames;

			delay /= playC.speed;

			playC.frame_last_delay = delay;
			playC.frame_last_pts = packet.ts;

			if (playC.skipAudioFrame < 0.0)
				playC.skipAudioFrame = 0.0;

			const double true_delay = delay;

			const double audioCurrentPts = playC.audio_current_pts;
			const bool canSkipFrames = syncVtoA && audioCurrentPts > 0.0;

			if (tmp_time >= 1.0)
			{
				emit playC.updateBitrateAndFPS(-1, round((tmp_br << 3) / (tmp_time * 1000.0)), frames / tmp_time, canSkipFrames ? framesDisplayed / framesDisplayedTime : qQNaN(), interlaced);
				frames = tmp_br = framesDisplayed = 0;
				tmp_time = framesDisplayedTime = 0.0;
			}

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
					QMetaObject::invokeMethod(this, "write", Q_ARG(VideoFrame, videoFrame), Q_ARG(quint32, seq));
					if (canSkipFrames)
						++framesDisplayed;
				}
				if (canSkipFrames)
					framesDisplayedTime += true_delay;
			}
			if (updateFrameTimer)
				frame_timer = gettime();
		}

		mutex.unlock();
	}
}

void VideoThr::write(VideoFrame videoFrame, quint32 lastSeq)
{
	canWrite = true;
	if (lastSeq == seq && writer->readyWrite() && !videoWriter()->hwAccelError())
	{
		QMPlay2GUI.screenSaver->inhibit(0);
		videoWriter()->writeVideo(videoFrame);
	}
}
void VideoThr::screenshot(VideoFrame videoFrame)
{
	ImgScaler imgScaler;
	const int aligned8W = Functions::aligned(W, 8);
	if (imgScaler.create(videoFrame.size, aligned8W, H, (bool)hwAccelWriter))
	{
		QImage img(aligned8W, H, QImage::Format_RGB32);
		bool ok = true;
		if (!hwAccelWriter)
			imgScaler.scale(videoFrame, img.bits());
		else
			ok = videoWriter()->hwAccelGetImg(videoFrame, img.bits(), &imgScaler);
		if (!ok)
			QMPlay2Core.logError(tr("Cannot create screenshot"));
		else
		{
			const QString ext = QMPlay2Core.getSettings().getString("screenshotFormat");
			const QString dir = QMPlay2Core.getSettings().getString("screenshotPth");
			quint16 num = 0;
			for (const QString &f : QDir(dir).entryList({"QMPlay2_snap_?????" + ext}, QDir::Files, QDir::Name))
			{
				const quint16 n = f.midRef(13, 5).toUShort();
				if (n > num)
					num = n;
			}
			img.save(dir + "/QMPlay2_snap_" + QString("%1").arg(++num, 5, 10, QChar('0')) + ext);
		}
	}
}
void VideoThr::pause()
{
	QMPlay2GUI.screenSaver->unInhibit(0);
	writer->pause();
}
