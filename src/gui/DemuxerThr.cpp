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

#include <DemuxerThr.hpp>

#include <PlayClass.hpp>
#include <AVThread.hpp>
#include <Writer.hpp>
#include <Main.hpp>

#include <Functions.hpp>
#include <SubsDec.hpp>
#include <Demuxer.hpp>
#include <Decoder.hpp>
#include <Reader.hpp>

#include <QCryptographicHash>
#include <QCoreApplication>
#include <QDir>

#include <cmath>

static inline bool getCurrentPlaying(int stream, const QList<StreamInfo *> &streamsInfo, const StreamInfo *streamInfo)
{
	return (stream > -1 && streamsInfo.count() > stream) && streamsInfo[stream] == streamInfo;
}
static QString getWriterName(AVThread *avThr)
{
	QString decName;
	if (avThr)
	{
		decName = avThr->dec->name();
		if (!decName.isEmpty())
			decName += ", ";
	}
	return (avThr && avThr->writer) ? " - <i>" + decName + avThr->writer->name() + "</i>" : QString();
}

static QString getCoverFile(const QString &title, const QString &artist, const QString &album)
{
	return QMPlay2Core.getSettingsDir() + "Covers/" + QCryptographicHash::hash((album.isEmpty() ? title.toUtf8() : album.toUtf8()) + artist.toUtf8(), QCryptographicHash::Md5).toHex();
}

class BufferInfo
{
public:
	inline BufferInfo() :
		remainingDuration(0.0), backwardDuration(0.0),
		firstPacketTime(-1), lastPacketTime(-1),
		remainingBytes(0), backwardBytes(0)
	{}

	double remainingDuration, backwardDuration;
	qint32 firstPacketTime, lastPacketTime;
	qint64 remainingBytes, backwardBytes;
};

/* DemuxerThr */

DemuxerThr::DemuxerThr(PlayClass &playC) :
	playC(playC),
	url(playC.url),
	err(false), demuxerReady(false), hasCover(false),
	skipBufferSeek(false), localStream(true), unknownLength(true), waitingForFillBufferB(false), paused(false), demuxerPaused(false),
	updateBufferedTime(0.0)
{}
DemuxerThr::~DemuxerThr()
{}

QByteArray DemuxerThr::getCoverFromStream() const
{
	return demuxer ? demuxer->image(true) : QByteArray();
}

void DemuxerThr::loadImage()
{
	if (isDemuxerReady())
	{
		const QByteArray demuxerImage = demuxer->image();
		QImage img = QImage::fromData(demuxerImage);
		if (!img.isNull())
			emit QMPlay2Core.coverDataFromMediaFile(demuxerImage);
		else
		{
			QString realUrl = url;
			Functions::splitPrefixAndUrlIfHasPluginPrefix(realUrl, nullptr, &realUrl);
			if (realUrl.startsWith("file://") && QMPlay2Core.getSettings().getBool("ShowDirCovers")) //Ładowanie okładki z katalogu
			{
				const QStringList nameFilters {
					"folder", "folder.*",
					"front", "front.*",
					"cover", "cover.*"
				};
				const QString directory = Functions::filePath(realUrl.mid(7));
				for (const QString &cover : QDir(directory).entryList(nameFilters, QDir::Files))
				{
					const QString coverPath = directory + cover;
					img = QImage(coverPath);
					if (!img.isNull())
					{
						emit QMPlay2Core.coverFile(coverPath);
						break;
					}
				}
			}
			if (img.isNull() && !artist.isEmpty() && (!title.isEmpty() || !album.isEmpty())) //Ładowanie okładki z cache
			{
				QString coverPath = getCoverFile(title, artist, album);
				if (!title.isEmpty() && !album.isEmpty() && !QFile::exists(coverPath)) //Try to load cover for title if album cover doesn't exist
					coverPath = getCoverFile(title, artist, QString());
				img = QImage(coverPath);
				if (!img.isNull())
					emit QMPlay2Core.coverFile(coverPath);
			}
		}
		hasCover = !img.isNull();
		emit playC.updateImage(img);
	}
}

void DemuxerThr::seek(bool doDemuxerSeek)
{
	if (!doDemuxerSeek && skipBufferSeek)
		return;
	if (playC.seekTo >= 0.0 || (playC.seekTo == SEEK_STREAM_RELOAD || playC.seekTo == SEEK_REPEAT))
	{
		AVThread *aThr = (AVThread *)playC.aThr, *vThr = (AVThread *)playC.vThr;

		emit playC.chText(tr("Seeking"));
		playC.canUpdatePos = false;

		bool repeat = false;

		bool seekInBuffer = !skipBufferSeek;
		if (playC.seekTo == SEEK_STREAM_RELOAD) //po zmianie strumienia audio, wideo lub napisów lub po ponownym uruchomieniu odtwarzania
		{
			playC.seekTo = qMax(0.0, playC.pos);
			seekInBuffer = false;
		}
		else if (playC.seekTo == SEEK_REPEAT)
		{
			playC.seekTo = 0.0;
			repeat = true;
		}

		const bool stepBackwards = (playC.allowAccurateSeek && playC.nextFrameB);

		const Qt::CheckState accurateSeek = (Qt::CheckState)QMPlay2Core.getSettings().getInt("AccurateSeek");
		const bool doAccurateSeek = (accurateSeek == Qt::Checked || (accurateSeek == Qt::PartiallyChecked && (!localStream || playC.allowAccurateSeek)) || stepBackwards);

		const bool backward = doAccurateSeek || repeat || (playC.seekTo < playC.pos);
		bool flush = false, aLocked = false, vLocked = false;

		skipBufferSeek = false;

		bool cantSeek = false;

		if (seekInBuffer && (!localStream || !backward))
		{
			playC.vPackets.lock();
			playC.aPackets.lock();
			playC.sPackets.lock();

			const auto doSeekInBuffer = [&] {
				double seekTo = playC.seekTo;
				bool ok = (!playC.vPackets.isEmpty() || !playC.aPackets.isEmpty());

				if (ok && !playC.vPackets.isEmpty())
				{
					ok &= playC.vPackets.seekTo(seekTo, backward);
					if (ok)
						seekTo = playC.vPackets.currentPacketTime(); // Use video position for audio seeking
				}

				if (ok && !playC.aPackets.isEmpty())
					ok &= playC.aPackets.seekTo(seekTo, backward);

				if (ok && !playC.sPackets.isEmpty())
					ok &= playC.sPackets.seekTo(seekTo, backward);

				return ok;
			};

			if (doSeekInBuffer())
			{
				doDemuxerSeek = false;
				flush = true;
				ensureTrueUpdateBuffered();
				if (aThr)
					aLocked = aThr->lock();
				if (vThr)
					vLocked = vThr->lock();
			}
			else
			{
				if (!accurateSeek && !backward && !localStream && playC.endOfStream)
				{
					// Don't seek in demuxer on network streams if we can't seek in buffer
					// and we have end of stream which means that everything is buffered.
					doDemuxerSeek = false;
					cantSeek = true;
				}
				else
				{
					emit playC.updateBufferedRange(-1, -1);
					updateBufferedTime = 0.0;
				}
			}

			playC.vPackets.unlock();
			playC.aPackets.unlock();
			playC.sPackets.unlock();
		}

		// Workaround: subtract 1 second for stepping backwards - sometimes FFmpeg doesn't seek to
		// key frame (why?) or PTS is higher than DTS. This also doesn't resolve all rare issues.
		if (doDemuxerSeek && demuxer->seek(playC.seekTo - (stepBackwards ? 1.0 : 0.0), backward))
			flush = true;
		else if (!doDemuxerSeek && !flush && !cantSeek)
		{
			skipBufferSeek = true;
			if (!localStream && !unknownLength)
				demuxer->abort(); //Abort only the Demuxer, not IOController
		}
		else if (repeat && doDemuxerSeek)
		{
			playC.seekTo = SEEK_REPEAT; //Notify that repeat seek failed
		}

		if (flush)
		{
			playC.endOfStream = false;
			if (doDemuxerSeek)
				clearBuffers();
			else
				playC.flushAssEvents();
			if (!aLocked && aThr)
				aLocked = aThr->lock();
			if (!vLocked && vThr)
				vLocked = vThr->lock();
			playC.skipAudioFrame = playC.audio_current_pts = 0.0;
			playC.flushVideo = playC.flushAudio = true;
			if (playC.pos < 0.0) //skok po rozpoczęciu odtwarzania po uruchomieniu programu
				emit playC.updatePos(playC.seekTo); //uaktualnia suwak na pasku do wskazanej pozycji
			if (repeat || playC.videoSeekPos > -1.0 || playC.audioSeekPos > -1.0)
				playC.emptyBufferCond.wakeAll(); //Wake AV threads
			const double seekPos = (doAccurateSeek && playC.seekTo > 0.0) ? playC.seekTo : -1.0;
			if (vThr)
				playC.videoSeekPos = seekPos;
			if (aThr)
				playC.audioSeekPos = seekPos;
			if (aLocked)
				aThr->unlock();
			if (vLocked)
				vThr->unlock();
		}

		if (!skipBufferSeek)
		{
			playC.canUpdatePos = true;
			if (playC.seekTo != SEEK_REPEAT) //Don't reset variable if repeat seek failed
				playC.seekTo = SEEK_NOWHERE;
			if (playC.paused)
			{
				if (!QMPlay2Core.getSettings().getBool("UnpauseWhenSeeking"))
					playC.pauseAfterFirstFrame = true;
				playC.paused = false;
			}
			else
			{
				emit playC.chText(tr("Playback"));
			}
		}
	}
}

void DemuxerThr::stop()
{
	ioCtrl.abort();
	demuxer.abort();
}
void DemuxerThr::end()
{
	bool endMutexLocked = false;

	stopVAMutex.lock();
	if (currentThread() != thread())
	{
		endMutex.lock(); //Zablokuje główny wątek do czasu zniszczenia demuxer'a
		endMutexLocked = true;
		QMetaObject::invokeMethod(this, "stopVADec"); //to wykonuje się w głównym wątku (Qt::QueuedConnection)
		stopVAMutex.lock(); //i czeka na koniec wykonywania
		stopVAMutex.unlock();
	}
	else //wywołane z głównego wątku
		stopVADec();

	demuxer.reset();

	if (endMutexLocked)
		endMutex.unlock(); //Jeżeli był zablokowany, odblokuje mutex

	playC.videoDecodersError.clear();
}

bool DemuxerThr::load(bool canEmitInfo)
{
	playC.loadMutex.lock();
	emit load(demuxer.rawPtr()); //to wykonuje się w głównym wątku (wywołuje load() z "playC")
	playC.loadMutex.lock(); //i czeka na koniec wykonywania
	playC.loadMutex.unlock();
	if (canEmitInfo)
		emitInfo();
	return playC.audioStream >= 0 || playC.videoStream >= 0;
}

void DemuxerThr::checkReadyWrite(AVThread *avThr)
{
	if (avThr && avThr->writer && avThr->updateTryLock())
	{
		err |= !avThr->writer->readyWrite();
		avThr->updateUnlock();
	}
}

void DemuxerThr::run()
{
	emit playC.chText(tr("Opening"));
	emit playC.setCurrentPlaying();

	emit QMPlay2Core.busyCursor();
	Functions::getDataIfHasPluginPrefix(url, &url, &name, nullptr, &ioCtrl);
	emit QMPlay2Core.restoreCursor();
	if (ioCtrl.isAborted())
		return end();

	if (!Demuxer::create(url, demuxer) || demuxer.isAborted())
	{
		if (!demuxer.isAborted() && !demuxer)
		{
			QMPlay2Core.logError(tr("Cannot open") + ": " + url.remove("file://"));
			emit playC.updateCurrentEntry(QString(), -1.0);
			err = true;
		}
		return end();
	}

	if (url.startsWith("file://"))
	{
		QStringList filter;
		filter << "*.ass" << "*.ssa";
		for (const QString &ext : SubsDec::extensions())
			filter << "*." + ext;
		for (StreamInfo *streamInfo : demuxer->streamsInfo())
			if (streamInfo->type == QMPLAY2_TYPE_VIDEO) //napisów szukam tylko wtedy, jeżeli jest strumień wideo
			{
				const QString directory = Functions::filePath(url.mid(7));
				const QString fName = Functions::fileName(url, false).replace('_', ' ');
				for (const QString &subsFile : QDir(directory).entryList(filter, QDir::Files))
				{
					const QString subsFName = Functions::fileName(subsFile, false).replace('_', ' ');
					if (subsFName.contains(fName, Qt::CaseInsensitive) || fName.contains(subsFName, Qt::CaseInsensitive))
					{
						const QString fileSubsUrl = Functions::Url(directory + subsFile);
						if (!playC.fileSubsList.contains(fileSubsUrl))
							playC.fileSubsList += fileSubsUrl;
					}
				}
				break;
			}
	}

	bool stillImage = playC.stillImage = demuxer->isStillImage();

	err = !load(false);
	if (err || demuxer.isAborted())
		return end();

	updatePlayingName = name.isEmpty() ? Functions::fileName(url, false) : name;

	if (playC.videoStream > -1)
		playC.frame_last_delay = getFrameDelay();

	/* ReplayGain */
	float gain_db = 0.0f, peak = 1.0f;
	if (!QMPlay2Core.getSettings().getBool("ReplayGain/Enabled") || !demuxer->getReplayGain(QMPlay2Core.getSettings().getBool("ReplayGain/Album"), gain_db, peak))
		playC.replayGain = 1.0;
	else
	{
		playC.replayGain = pow(10.0, gain_db / 20.0) * pow(10.0, QMPlay2Core.getSettings().getDouble("ReplayGain/Preamp") / 20.0);
		if (QMPlay2Core.getSettings().getBool("ReplayGain/PreventClipping") && peak * playC.replayGain > 1.0)
			playC.replayGain = 1.0 / peak;
	}

	if ((unknownLength = demuxer->length() < 0.0))
		updateBufferedSeconds = false;

	emit playC.updateLength(round(demuxer->length()));
	emit playC.chText(tr("Playback"));
	emit playC.playStateChanged(true);

	localStream = demuxer->localStream();
	time = localStream ? 0.0 : Functions::gettime();

	int forwardPackets = demuxer->dontUseBuffer() ? 1 : (localStream ? minBuffSizeLocal : minBuffSizeNetwork), backwardPackets;
	int vS, aS;

	demuxerReady = true;

	updateCoverAndPlaying(false);
	connect(&QMPlay2Core, SIGNAL(updateCover(QString, QString, QString, QByteArray)), this, SLOT(updateCover(QString, QString, QString, QByteArray)));

	if (forwardPackets == 1 || localStream || unknownLength)
		PacketBuffer::setBackwardPackets((backwardPackets = 0));
	else
	{
		int percent = 25;
		switch (QMPlay2Core.getSettings().getUInt("BackwardBuffer"))
		{
			case 0:
				percent = 10;
				break;
			case 2:
				percent = 50;
				break;
		}
		PacketBuffer::setBackwardPackets((backwardPackets = forwardPackets * percent / 100));
		forwardPackets -= backwardPackets;
	}

	if (!localStream)
		setPriority(QThread::LowPriority); //Network streams should have low priority, because slow single core CPUs have problems with smooth video playing during buffering

	DemuxerTimer demuxerTimer(*this);

	if (stillImage && playC.paused)
		playC.paused = false;

	while (!demuxer.isAborted())
	{
		{
			QMutexLocker seekLocker(&seekMutex);
			seek(true);
			if (playC.seekTo == SEEK_REPEAT)
				break; //Repeat seek failed - break.
		}

		AVThread *aThr = (AVThread *)playC.aThr, *vThr = (AVThread *)playC.vThr;

		checkReadyWrite(aThr);
		checkReadyWrite(vThr);

		if (demuxer.isAborted() || err)
			break;

		if (vThr && vThr->hasDecoderError())
		{
			playC.videoDecodersError.insert(playC.videoDecoderModuleName);
			playC.reload = playC.videoDecErrorLoad = true;
			if (!load() || playC.videoStream < 0)
			{
				err = true;
				break;
			}
		}

		handlePause();

		if (playC.pauseAfterFirstFrame)
		{
			playC.nextFrame();
			playC.pauseAfterFirstFrame = false;
		}

		const bool updateBuffered = localStream ? false : canUpdateBuffered();
		const double remainingDuration = getAVBuffersSize(vS, aS);
		if (playC.endOfStream && !vS && !aS && canBreak(aThr, vThr))
		{
			if (!stillImage)
			{
				if (!unknownLength && playC.doRepeat)
				{
					playC.seekTo = SEEK_REPEAT;
					continue;
				}
				break;
			}
			else
			{
				if (playC.paused)
					stillImage = false;
				else
					playC.paused = true;
				handlePause();
			}
		}
		if (updateBuffered)
		{
			emitBufferInfo(false);
			if (demuxer->metadataChanged())
				updateCoverAndPlaying(true);
		}
		else if (localStream && demuxer->metadataChanged())
			updateCoverAndPlaying(true);

		if (!localStream && !playC.waitForData && !playC.endOfStream && playIfBuffered > 0.0 && emptyBuffers(vS, aS))
		{
			playC.waitForData = true;
			updateBufferedTime = 0.0;
		}
		else if
		(
			playC.waitForData &&
			(
				playC.endOfStream                                                  ||
				bufferedAllPackets(vS, aS, forwardPackets)                         || //Buffer is full
				(remainingDuration >= playIfBuffered)                              ||
				(qFuzzyIsNull(remainingDuration) && bufferedAllPackets(vS, aS, 2))    //Buffer has at least 2 packets, but still without length (in seconds) information
			)
		)
		{
			playC.waitForData = false;
			if (!paused)
				playC.emptyBufferCond.wakeAll();
		}

		if (playC.endOfStream || bufferedAllPackets(vS, aS, forwardPackets))
		{
			if (paused && !demuxerPaused)
			{
				demuxerPaused = true;
				demuxer->pause();
			}

			//po zakończeniu buforowania należy odświeżyć informacje o buforowaniu
			if (paused && !waitingForFillBufferB && !playC.fillBufferB)
			{
				ensureTrueUpdateBuffered();
				waitingForFillBufferB = true;
				continue;
			}

			bool loadError = false, first = true;
			while (!playC.fillBufferB)
			{
				if (mustReloadStreams() && !load())
				{
					loadError = true;
					break;
				}
				if (playC.seekTo == SEEK_STREAM_RELOAD)
					break;
				if (!first)
					msleep(15);
				else
				{
					msleep(1);
					first = false;
				}
			}
			if (loadError)
				break;
			playC.fillBufferB = false;
			continue;
		}
		waitingForFillBufferB = false;

		Packet packet;
		int streamIdx = -1;
		if (!localStream)
			demuxerTimer.start(); //Start the timer which will update buffer and pause information while demuxer is busy for long time (demuxer must call "processEvents()" from time to time)
		const bool demuxerOk = demuxer->read(packet, streamIdx);
		if (!localStream)
			demuxerTimer.stop(); //Stop the timer, because the demuxer loop updates the data automatically
		if (demuxerOk)
		{
			if (mustReloadStreams() && !load())
				break;

			if (streamIdx < 0 || playC.seekTo == SEEK_STREAM_RELOAD)
				continue;

			if (streamIdx == playC.audioStream)
				playC.aPackets.put(packet);
			else if (streamIdx == playC.videoStream)
				playC.vPackets.put(packet);
			else if (streamIdx == playC.subtitlesStream)
				playC.sPackets.put(packet);

			if (!paused && !playC.waitForData)
				playC.emptyBufferCond.wakeAll();
		}
		else if (!skipBufferSeek)
		{
			getAVBuffersSize(vS, aS);
			playC.endOfStream = true;
			if (vS || aS || !canBreak(aThr, vThr))
			{
				if (!localStream)
					ensureTrueUpdateBuffered();
				if (!vS || !aS)
				{
					if (!vS)
						playC.videoSeekPos = -1.0;
					if (!aS)
						playC.audioSeekPos = -1.0;
					playC.emptyBufferCond.wakeAll();
				}
			}
			else if (!stillImage && !playC.doRepeat)
			{
				break;
			}
		}
	}

	emit QMPlay2Core.updatePlaying(false, title, artist, album, round(demuxer->length()), false, updatePlayingName);

	playC.endOfStream = playC.canUpdatePos = false; //to musi tu być!
	end();
}

inline void DemuxerThr::ensureTrueUpdateBuffered()
{
	time = Functions::gettime() - updateBufferedTime; //zapewni, że updateBuffered będzie na "true";
}
inline bool DemuxerThr::canUpdateBuffered() const
{
	return Functions::gettime() - time >= updateBufferedTime;
}
void DemuxerThr::handlePause()
{
	if (playC.paused)
	{
		if (!paused)
		{
			paused = true;
			emit playC.chText(tr("Paused"));
			emit playC.playStateChanged(false);
			playC.emptyBufferCond.wakeAll();
		}
	}
	else if (paused)
	{
		paused = demuxerPaused = false;
		emit playC.chText(tr("Playback"));
		emit playC.playStateChanged(true);
		playC.emptyBufferCond.wakeAll();
	}
}
void DemuxerThr::emitBufferInfo(bool clearBackwards)
{
	const BufferInfo bufferInfo = getBufferInfo(clearBackwards);
	if (updateBufferedSeconds)
		emit playC.updateBufferedRange(bufferInfo.firstPacketTime * 10, bufferInfo.lastPacketTime * 10);
	emit playC.updateBuffered(bufferInfo.backwardBytes, bufferInfo.remainingBytes, bufferInfo.backwardDuration, bufferInfo.remainingDuration);
	waitingForFillBufferB = true;
	if (updateBufferedTime < 1.0)
		updateBufferedTime += 0.25;
	time = Functions::gettime();
}

void DemuxerThr::updateCoverAndPlaying(bool doCompare)
{
	const QString prevTitle  = title;
	const QString prevArtist = artist;
	const QString prevAlbum  = album;
	title.clear();
	artist.clear();
	album.clear();
	for (const QMPlay2Tag &tag : demuxer->tags()) //wczytywanie tytułu, artysty i albumu
	{
		const QMPlay2Tags tagID = StreamInfo::getTag(tag.first);
		switch (tagID)
		{
			case QMPLAY2_TAG_TITLE:
				title = tag.second;
				break;
			case QMPLAY2_TAG_ARTIST:
				artist = tag.second;
				break;
			case QMPLAY2_TAG_ALBUM:
				album = tag.second;
				break;
			default:
				break;
		}
	}
	if (!doCompare || prevTitle != title || prevArtist != artist || prevAlbum != album)
	{
		const bool showCovers = QMPlay2Core.getSettings().getBool("ShowCovers");
		if (showCovers)
			loadImage();
		emitInfo();
		emit QMPlay2Core.updatePlaying(true, title, artist, album, round(demuxer->length()), showCovers && !hasCover, updatePlayingName);
	}
}

static void printOtherInfo(const QVector<QMPlay2Tag> &other_info, QString &str)
{
	for (const QMPlay2Tag &tag : other_info)
		if (!tag.second.isEmpty())
		{
			QString value = tag.second;
			if (tag.first.toInt() == QMPLAY2_TAG_LANGUAGE)
				value = QMPlay2Core.getLanguagesMap().value(value, tag.second).toLower();
			str += "<li><b>" + StreamInfo::getTagName(tag.first).toLower() + ":</b> " + value + "</li>";
		}
}
void DemuxerThr::addSubtitleStream(bool currentPlaying, QString &subtitlesStreams, int i, int subtitlesStreamCount, const QString &streamName, const QString &codecName, const QString &title, const QVector<QMPlay2Tag> &other_info)
{
	subtitlesStreams += "<ul style='margin-top: 0px; margin-bottom: 0px;'>";
	if (currentPlaying)
		subtitlesStreams += "<u>";
	else
		subtitlesStreams += "<a style='text-decoration: none; color: black;' href='stream:" + streamName + QString::number(i) + "'>";
	subtitlesStreams += "<li><b>" + tr("Stream") + " " + QString::number(subtitlesStreamCount) + "</b></li>";
	if (currentPlaying)
		subtitlesStreams += "</u>";
	else
		subtitlesStreams += "</a>";
	subtitlesStreams += "<ul>";
	if (!title.isEmpty())
		subtitlesStreams += "<li><b>" + tr("title") + ":</b> " + title + "</li>";
	if (streamName == "fileSubs")
		subtitlesStreams += "<li><b>" + tr("loaded from file") + "</b></li>";
	if (!codecName.isEmpty())
		subtitlesStreams += "<li><b>" + tr("format") + ":</b> " + codecName + "</li>";
	printOtherInfo(other_info, subtitlesStreams);
	subtitlesStreams += "</ul></ul>";
}
void DemuxerThr::emitInfo()
{
	QString info;

	QString formatTitle = demuxer->title();
	if (formatTitle.isEmpty())
	{
		if (!name.isEmpty())
			info += "<b>" + tr("Title") + ":</b> " + name;
		formatTitle = name;
	}

	bool nameOrDescr = false;
	for (const QMPlay2Tag &tag : demuxer->tags())
		if (!tag.first.isEmpty())
		{
			QString txt = tag.second;
			txt.replace("<", "&#60;"); //Don't recognize as HTML tag
			if (tag.first == "0" || tag.first == "1") //Name and description
			{
				info += "<b>" + txt + "</b><br/>";
				nameOrDescr = true;
			}
			else
			{
				if (nameOrDescr)
				{
					info += "<br/>";
					nameOrDescr = false;
				}
				info += "<b>" + StreamInfo::getTagName(tag.first) + ":</b> " + txt + "<br/>";
			}
		}

	if (!info.isEmpty())
		info += "<br/>";

	QString realUrl;
	if (!Functions::splitPrefixAndUrlIfHasPluginPrefix(url, nullptr, &realUrl, nullptr))
		realUrl = url;
	if (!realUrl.startsWith("file://"))
		info += "<b>" + tr("Address") + ":</b> " + realUrl + "<br>";
	else
	{
		const QString pth = realUrl.right(realUrl.length() - 7);
		info += "<b>" + tr("File path") + ": </b> " + Functions::filePath(pth) + "<br/>";
		info += "<b>" + tr("File name") + ": </b> " + Functions::fileName(pth) + "<br/>";
	}

	if (demuxer->bitrate() > 0)
		info += "<b>" + tr("Bitrate") + ":</b> " + QString::number(demuxer->bitrate()) + "kbps<br/>";
	info += "<b>" + tr("Format") + ":</b> " + demuxer->name();

	if (!demuxer->image().isNull())
		info += "<br/><br/><a href='save_cover'>" + tr("Save cover picture") + "</a>";

	bool once = false;
	for (const ProgramInfo &program : demuxer->getPrograms())
	{
		if (!once)
		{
			info += "<p style='margin-bottom: 0px;'><b><big>" + tr("Programs") + ":</big></b></p>";
			once = true;
		}

		const int currentPlayingWanted = (playC.videoStream != -1) + (playC.audioStream != -1) + (playC.subtitlesStream != -1);
		int currentPlaying = 0;
		QString streams;

		for (int i = 0; i < program.streams.count(); ++i)
		{
			const int stream = program.streams.at(i).first;
			const QMPlay2MediaType type = program.streams.at(i).second;
			streams += QString::number(type) + ":" + QString::number(stream) + ",";
			if (stream == playC.videoStream || stream == playC.audioStream || stream == playC.subtitlesStream)
				++currentPlaying;
		}
		streams.chop(1);

		info += "<ul style='margin-top: 0px; margin-bottom: 0px;'>";
		info += "<li>";
		if (!streams.isEmpty())
			info += "<a href='stream:" + streams + "'>";
		if (currentPlaying == currentPlayingWanted)
			info += "<b>";
		info += tr("Program") + " " + QString::number(program.number);
		if (currentPlaying == currentPlayingWanted)
			info += "</b>";
		if (!streams.isEmpty())
			info += "</a>";
		info += "</li></ul>";
	}

	int chapterCount = 0;
	once = false;
	for (const ChapterInfo &chapter : demuxer->getChapters())
	{
		if (!once)
		{
			info += "<p style='margin-bottom: 0px;'><b><big>" + tr("Chapters") + ":</big></b></p>";
			once = true;
		}
		info += "<ul style='margin-top: 0px; margin-bottom: 0px;'>";
		info += "<li><a href='seek:" + QString::number(chapter.start) + "'>" + (chapter.title.isEmpty() ? tr("Chapter") + " " + QString::number(++chapterCount) : chapter.title) + "</a></li>";
		info += "</ul>";
	}

	bool videoPlaying = false, audioPlaying = false;

	const QList<StreamInfo *> streamsInfo = demuxer->streamsInfo();
	QString videoStreams, audioStreams, subtitlesStreams, attachmentStreams;
	int videoStreamCount = 0, audioStreamCount = 0, subtitlesStreamCount = 0, i = 0;
	for (StreamInfo *streamInfo : streamsInfo)
	{
		switch (streamInfo->type)
		{
			case QMPLAY2_TYPE_VIDEO:
			{
				const bool currentPlaying = getCurrentPlaying(playC.videoStream, streamsInfo, streamInfo);
				videoStreams += "<ul style='margin-top: 0px; margin-bottom: 0px;'><li>";
				if (currentPlaying)
				{
					videoPlaying = true;
					videoStreams += "<u>";
				}
				else
					videoStreams += "<a style='text-decoration: none; color: black;' href='stream:video" + QString::number(i) + "'>";
				videoStreams += "<b>" + tr("Stream") + " " + QString::number(++videoStreamCount) + "</b>";
				if (currentPlaying)
					videoStreams += "</u>" + getWriterName((AVThread *)playC.vThr);
				else
					videoStreams += "</a>";
				videoStreams += "</li><ul>";
				if (!streamInfo->title.isEmpty())
					videoStreams += "<li><b>" + tr("title") + ":</b> " + streamInfo->title + "</li>";
				if (!streamInfo->codec_name.isEmpty())
					videoStreams += "<li><b>" + tr("codec") + ":</b> " + streamInfo->codec_name + "</li>";
				videoStreams += "<li><b>" + tr("size") + ":</b> " + QString::number(streamInfo->W) + "x" + QString::number(streamInfo->H) + "</li>";
				videoStreams += "<li><b>" + tr("aspect ratio") + ":</b> " + QString::number(streamInfo->getAspectRatio()) + "</li>";
				if (streamInfo->FPS)
					videoStreams += "<li><b>" + tr("FPS") + ":</b> " + QString::number(streamInfo->FPS) + "</li>";
				if (streamInfo->bitrate)
					videoStreams += "<li><b>" + tr("bitrate") + ":</b> " + QString::number(streamInfo->bitrate / 1000) + "kbps</li>";
				if (!streamInfo->format.isEmpty())
					videoStreams += "<li><b>" + tr("format") + ":</b> " + streamInfo->format + "</li>";
				printOtherInfo(streamInfo->other_info, videoStreams);
				videoStreams += "</ul></ul>";
			} break;
			case QMPLAY2_TYPE_AUDIO:
			{
				const bool currentPlaying = getCurrentPlaying(playC.audioStream, streamsInfo, streamInfo);
				audioStreams += "<ul style='margin-top: 0px; margin-bottom: 0px;'><li>";
				if (currentPlaying)
				{
					audioPlaying = true;
					audioStreams += "<u>";
				}
				else
					audioStreams += "<a style='text-decoration: none; color: black;' href='stream:audio" + QString::number(i) + "'>";
				audioStreams += "<b>" + tr("Stream") + " " + QString::number(++audioStreamCount) + "</b>";
				if (currentPlaying)
					audioStreams += "</u>" + getWriterName((AVThread *)playC.aThr);
				else
					audioStreams += "</a>";
				audioStreams += "</li><ul>";
				if (!streamInfo->title.isEmpty())
					audioStreams += "<li><b>" + tr("title") + ":</b> " + streamInfo->title + "</li>";
				if (!streamInfo->artist.isEmpty())
					audioStreams += "<li><b>" + tr("artist") + ":</b> " + streamInfo->artist + "</li>";
				if (!streamInfo->codec_name.isEmpty())
					audioStreams += "<li><b>" + tr("codec") + ":</b> " + streamInfo->codec_name + "</li>";
				audioStreams += "<li><b>" + tr("sample rate") + ":</b> " + QString::number(streamInfo->sample_rate) + "Hz</li>";

				QString channels;
				if (streamInfo->channels == 1)
					channels = tr("mono");
				else if (streamInfo->channels == 2)
					channels = tr("stereo");
				else
					channels = QString::number(streamInfo->channels);
				audioStreams += "<li><b>" + tr("channels") + ":</b> " + channels + "</li>";

				if (streamInfo->bitrate)
					audioStreams += "<li><b>" + tr("bitrate") + ":</b> " + QString::number(streamInfo->bitrate / 1000) + "kbps</li>";
				if (!streamInfo->format.isEmpty())
					audioStreams += "<li><b>" + tr("format") + ":</b> " + streamInfo->format + "</li>";
				printOtherInfo(streamInfo->other_info, audioStreams);
				audioStreams += "</ul></ul>";
			} break;
			case QMPLAY2_TYPE_SUBTITLE:
				addSubtitleStream(getCurrentPlaying(playC.subtitlesStream, streamsInfo, streamInfo), subtitlesStreams, i, ++subtitlesStreamCount, "subtitles", streamInfo->codec_name, streamInfo->title, streamInfo->other_info);
				break;
			case QMPLAY2_TYPE_ATTACHMENT:
			{
				attachmentStreams += "<ul style='margin-top: 0px; margin-bottom: 0px;'>";
				attachmentStreams += "<li><b>" + streamInfo->title + "</b> - " + Functions::sizeString(streamInfo->data.size()) + "</li>";
				attachmentStreams += "</ul>";
			} break;
			default:
				break;
		}
		++i;
	}
	i = 0;
	for (const QString &fName : asConst(playC.fileSubsList))
		addSubtitleStream(fName == playC.fileSubs, subtitlesStreams, i++, ++subtitlesStreamCount, "fileSubs", QString(), Functions::fileName(fName));

	if (!videoStreams.isEmpty())
		info += "<p style='margin-bottom: 0px;'><b><big>" + tr("Video streams") + ":</big></b></p>" + videoStreams;
	if (!audioStreams.isEmpty())
		info += "<p style='margin-bottom: 0px;'><b><big>" + tr("Audio streams") + ":</big></b></p>" + audioStreams;
	if (!subtitlesStreams.isEmpty())
		info += "<p style='margin-bottom: 0px;'><b><big>" + tr("Subtitles streams") + ":</big></b></p>" + subtitlesStreams;
	if (!attachmentStreams.isEmpty())
		info += "<p style='margin-bottom: 0px;'><b><big>" + tr("Attached files") + ":</big></b></p>" + attachmentStreams;

	emit playC.setInfo(info, videoPlaying, audioPlaying);
	emit playC.updateCurrentEntry(formatTitle, demuxer->length());
}

bool DemuxerThr::mustReloadStreams()
{
	if
	(
		playC.reload ||
		(playC.choosenAudioStream     > -1 && playC.choosenAudioStream     != playC.audioStream    ) ||
		(playC.choosenVideoStream     > -1 && playC.choosenVideoStream     != playC.videoStream    ) ||
		(playC.choosenSubtitlesStream > -1 && playC.choosenSubtitlesStream != playC.subtitlesStream)
	)
	{
		if (playC.frame_last_delay <= 0.0 && playC.videoStream > -1)
			playC.frame_last_delay = getFrameDelay();
		playC.reload = true;
		return true;
	}
	return false;
}
bool DemuxerThr::bufferedAllPackets(int vS, int aS, int p)
{
	return
	(
		(playC.vThr && vS >= p && playC.aThr && aS >= p) ||
		(!playC.vThr && playC.aThr && aS >= p)           ||
		(playC.vThr && !playC.aThr && vS >= p)
	);
}
bool DemuxerThr::emptyBuffers(int vS, int aS)
{
	return (playC.vThr && playC.aThr && (vS <= 1 || aS <= 1)) || (!playC.vThr && playC.aThr && aS <= 1) || (playC.vThr && !playC.aThr && vS <= 1);
}
bool DemuxerThr::canBreak(const AVThread *avThr1, const AVThread *avThr2)
{
	return (!avThr1 || avThr1->isWaiting()) && (!avThr2 || avThr2->isWaiting());
}
double DemuxerThr::getAVBuffersSize(int &vS, int &aS)
{
	double remainingDuration = 0.0;

	playC.vPackets.lock();
	playC.aPackets.lock();

	if (playC.vPackets.isEmpty())
		vS = 0;
	else
	{
		vS = playC.vPackets.remainingPacketsCount();
		remainingDuration = playC.vPackets.remainingDuration();
	}

	if (playC.aPackets.isEmpty())
		aS = 0;
	else
	{
		aS = playC.aPackets.remainingPacketsCount();
		remainingDuration = remainingDuration > 0.0 ? qMin(remainingDuration, playC.aPackets.remainingDuration()) : playC.aPackets.remainingDuration();
	}

	playC.aPackets.unlock();
	playC.vPackets.unlock();

	//niedokładność double (?)
	if (remainingDuration < 0.0)
		remainingDuration = 0.0;

	return remainingDuration;
}
BufferInfo DemuxerThr::getBufferInfo(bool clearBackwards)
{
	BufferInfo bufferInfo;

	playC.vPackets.lock();
	playC.aPackets.lock();

	if (clearBackwards)
		playC.vPackets.clearBackwards();
	if (!playC.vPackets.isEmpty())
	{
		bufferInfo.backwardBytes  += playC.vPackets.backwardBytes();
		bufferInfo.remainingBytes += playC.vPackets.remainingBytes();

		bufferInfo.backwardDuration  = playC.vPackets.backwardDuration();
		bufferInfo.remainingDuration = playC.vPackets.remainingDuration();

		bufferInfo.firstPacketTime = floor(playC.vPackets.firstPacketTime());
		bufferInfo.lastPacketTime  = ceil (playC.vPackets.lastPacketTime() );
	}

	if (clearBackwards)
		playC.aPackets.clearBackwards();
	if (!playC.aPackets.isEmpty())
	{
		const qint32 firstAPacketTime = floor(playC.aPackets.firstPacketTime()), lastAPacketTime = ceil(playC.aPackets.lastPacketTime());

		bufferInfo.backwardBytes  += playC.aPackets.backwardBytes();
		bufferInfo.remainingBytes += playC.aPackets.remainingBytes();

		bufferInfo.backwardDuration  = bufferInfo.backwardDuration  > 0.0 ? qMax(bufferInfo.backwardDuration,  playC.aPackets.backwardDuration() ) : playC.aPackets.backwardDuration();
		bufferInfo.remainingDuration = bufferInfo.remainingDuration > 0.0 ? qMin(bufferInfo.remainingDuration, playC.aPackets.remainingDuration()) : playC.aPackets.remainingDuration();

		bufferInfo.firstPacketTime = bufferInfo.firstPacketTime >= 0 ? qMax(bufferInfo.firstPacketTime, firstAPacketTime) : firstAPacketTime;
		bufferInfo.lastPacketTime  = bufferInfo.lastPacketTime  >= 0 ? qMin(bufferInfo.lastPacketTime,  lastAPacketTime ) : lastAPacketTime;
	}

	playC.aPackets.unlock();
	playC.vPackets.unlock();

	//niedokładność double (?)
	if (bufferInfo.backwardDuration < 0.0)
		bufferInfo.backwardDuration = 0.0;
	if (bufferInfo.remainingDuration < 0.0)
		bufferInfo.remainingDuration = 0.0;

	return bufferInfo;
}
void DemuxerThr::clearBuffers()
{
	playC.vPackets.clear();
	playC.aPackets.clear();
	playC.clearSubtitlesBuffer();
}

double DemuxerThr::getFrameDelay() const
{
	return 1.0 / demuxer->streamsInfo().at(playC.videoStream)->FPS;
}

void DemuxerThr::stopVADec()
{
	clearBuffers();

	playC.stopVDec();
	playC.stopADec();

	stopVAMutex.unlock();

	endMutex.lock(); //Czeka do czasu zniszczenia demuxer'a - jeżeli wcześniej mutex był zablokowany (wykonał się z wątku)
	endMutex.unlock(); //odblokowywuje mutex
}
void DemuxerThr::updateCover(const QString &title, const QString &artist, const QString &album, const QByteArray &cover)
{
	const QImage coverImg = QImage::fromData(cover);
	if (!coverImg.isNull())
	{
		if (this->title == title && this->artist == artist && (this->album == album || (album.isEmpty() && !title.isEmpty())))
			emit playC.updateImage(coverImg);
		QDir dir(QMPlay2Core.getSettingsDir());
		dir.mkdir("Covers");
		QFile f(getCoverFile(title, artist, album));
		if (f.open(QFile::WriteOnly))
		{
			f.write(cover);
			f.close();
			emit QMPlay2Core.coverFile(f.fileName());
		}
	}
}

/* BufferInfoTimer */

DemuxerTimer::DemuxerTimer(DemuxerThr &demuxerThr) :
	demuxerThr(demuxerThr)
{
	connect(&t, SIGNAL(timeout()), this, SLOT(timeout()));
}

inline void DemuxerTimer::start()
{
	t.start(500);
}
inline void DemuxerTimer::stop()
{
	t.stop();
}

void DemuxerTimer::timeout()
{
	demuxerThr.handlePause();
	if (demuxerThr.canUpdateBuffered())
		demuxerThr.emitBufferInfo(true);
}
