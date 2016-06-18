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

#include <PlayClass.hpp>

#include <QMPlay2_OSD.hpp>
#include <VideoThr.hpp>
#include <AudioThr.hpp>
#include <DemuxerThr.hpp>
#include <LibASS.hpp>
#include <Main.hpp>

#include <VideoFrame.hpp>
#include <Functions.hpp>
#include <Settings.hpp>
#include <SubsDec.hpp>
#include <Demuxer.hpp>
#include <Decoder.hpp>
#include <Reader.hpp>

#if QT_VERSION >= 0x040800
	#define USE_QRAWFONT
#endif

#include <QCoreApplication>
#ifdef USE_QRAWFONT
	#include <QRawFont>
#else
	#include <QFontDatabase>
#endif
#include <QInputDialog>
#include <QMessageBox>
#include <QTextCodec>
#include <QAction>
#include <QDir>

#include <math.h>

#if defined Q_OS_WIN && !defined Q_OS_WIN64
	#include <QProgressBar>
	#include <QVBoxLayout>
	#include <QLabel>

	class UpdateFC : public QThread
	{
	public:
		UpdateFC(LibASS *ass) :
			ass(ass)
		{
			start();
			if (!wait(500))
			{
				QDialog d(QMPlay2GUI.mainW);
				d.setWindowTitle(QCoreApplication::applicationName());
				QLabel l(QObject::tr("Font cache is updating, please wait"));
				QProgressBar p;
				p.setRange(0, 0);
				QVBoxLayout la(&d);
				la.addWidget(&l);
				la.addWidget(&p);
				d.open();
				d.setMinimumSize(d.size());
				d.setMaximumSize(d.size());
				do
					el.processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents);
				while (isRunning());
			}
		}
	private:
		void run()
		{
			ass->initOSD();
			SetFileAttributesW((WCHAR *)QString(QDir::homePath() + "/fontconfig").utf16(), FILE_ATTRIBUTE_HIDDEN);
			el.wakeUp();
		}
		LibASS *ass;
		QEventLoop el;
	};
#endif

PlayClass::PlayClass() :
	demuxThr(NULL), vThr(NULL), aThr(NULL),
#if defined Q_OS_WIN && !defined Q_OS_WIN64
	firsttimeUpdateCache(true),
#endif
	ass(NULL), osd(NULL)
{
	doSilenceBreak = doSilenceOnStart = false;

	maxThreshold = 60.0;
	vol[0] = vol[1] = 1.0;

	quitApp = muted = reload = false;

	aRatioName = "auto";
	speed = subtitlesScale = zoom = 1.0;
	flip = 0;
	rotate90 = spherical = false;

	restartSeekTo = SEEK_NOWHERE;
	seekA = seekB = -1;

	subtitlesStream = -1;
	videoSync = subtitlesSync = 0.0;
	videoEnabled = audioEnabled = subtitlesEnabled = true;

	doSuspend = false;

	Brightness = Saturation = Contrast = Hue = 0;

	connect(&timTerminate, SIGNAL(timeout()), this, SLOT(timTerminateFinished()));
	connect(this, SIGNAL(aRatioUpdate(double)), this, SLOT(aRatioUpdated(double)));
	connect(this, SIGNAL(frameSizeUpdate(int, int)), this, SLOT(frameSizeUpdated(int, int)));
	connect(this, SIGNAL(pixelFormatUpdate(const QByteArray &)), this, SLOT(pixelFormatUpdated(const QByteArray &)));
}

void PlayClass::play(const QString &_url)
{
	if (!demuxThr)
	{
		if (audioEnabled || videoEnabled)
		{
			audioStream = videoStream = subtitlesStream = -1;

			url = _url;
			demuxThr = new DemuxerThr(*this);
			demuxThr->minBuffSizeLocal = QMPlay2Core.getSettings().getInt("AVBufferLocal");
			demuxThr->minBuffSizeNetwork = QMPlay2Core.getSettings().getInt("AVBufferNetwork");
			demuxThr->playIfBuffered = QMPlay2Core.getSettings().getDouble("PlayIfBuffered");
			demuxThr->updateBufferedSeconds = QMPlay2Core.getSettings().getBool("ShowBufferedTimeOnSlider");

			connect(demuxThr, SIGNAL(load(Demuxer *)), this, SLOT(load(Demuxer *)));
			connect(demuxThr, SIGNAL(finished()), this, SLOT(demuxThrFinished()));

			if (!QMPlay2Core.getSettings().getBool("KeepZoom"))
				zoomReset();
			if (!QMPlay2Core.getSettings().getBool("KeepARatio"))
				emit resetARatio();
			if (!QMPlay2Core.getSettings().getBool("KeepSubtitlesDelay"))
				subtitlesSync = 0.0;
			if (!QMPlay2Core.getSettings().getBool("KeepSubtitlesScale"))
				subtitlesScale = 1.0;
			if (!QMPlay2Core.getSettings().getBool("KeepVideoDelay"))
				videoSync = 0.0;
			if (!QMPlay2Core.getSettings().getBool("KeepSpeed"))
				speed = 1.0;

			replayGain = 1.0;

			canUpdatePos = true;
			waitForData = paused = flushVideo = flushAudio = endOfStream = false;
			lastSeekTo = seekTo = pos = SEEK_NOWHERE;
			skipAudioFrame = audio_current_pts = frame_last_pts = frame_last_delay = audio_last_delay = 0.0;

			ignorePlaybackError = QMPlay2Core.getSettings().getBool("IgnorePlaybackError");

			choosenAudioLang = QMPlay2Core.getLanguagesMap().key(QMPlay2Core.getSettings().getString("AudioLanguage"));
			choosenSubtitlesLang = QMPlay2Core.getLanguagesMap().key(QMPlay2Core.getSettings().getString("SubtitlesLanguage"));

			if (restartSeekTo >= 0.0) //jeżeli restart odtwarzania
			{
				seekTo = restartSeekTo;
				restartSeekTo = SEEK_NOWHERE;
			}
			else
				choosenAudioStream = choosenVideoStream = choosenSubtitlesStream = -1;

			demuxThr->start();
		}
	}
	else
	{
		newUrl = _url;
		stop();
	}
}
void PlayClass::stop(bool _quitApp)
{
	if (stopPauseMutex.tryLock())
	{
		quitApp = _quitApp;
		if (isPlaying())
		{
			if (aThr && newUrl.isEmpty())
				 aThr->silence();
			if (isPlaying())
			{
				timTerminate.start(TERMINATE_TIMEOUT * 5 / 3);
				demuxThr->stop();
				fillBufferB = true;
			}
		}
		else
		{
			stopAVThr();
			clearPlayInfo();
			if (quitApp)
				emit quit();
		}
		stopPauseMutex.unlock();
	}
}
void PlayClass::restart()
{
	if (!url.isEmpty())
	{
		restartSeekTo = pos;
		play(url);
	}
}

void PlayClass::chPos(double newPos, bool updateGUI)
{
	if (!canUpdatePos)
		return;
	if ((updateGUI || pos == -1) && ((int)newPos != (int)pos))
		emit updatePos(newPos);
	pos = newPos;
	lastSeekTo = SEEK_NOWHERE;
	if (seekA >= 0 && seekB > seekA && pos >= seekB) //A-B Repeat
		seek(seekA);
}

void PlayClass::togglePause()
{
	if (stopPauseMutex.tryLock())
	{
		if (aThr && !paused)
			aThr->silence();
		paused = !paused;
		fillBufferB = true;
		if (aThr && !paused)
			aThr->silence(true);
		stopPauseMutex.unlock();
	}
}
void PlayClass::seek(int pos)
{
	if (pos < 0)
		pos = 0;
	if (lastSeekTo == pos)
		return;
	if ((seekA > -1 && pos < seekA) || (seekB > -1 && pos > seekB))
	{
		const bool showDisabledInfo = (seekA > 0 || seekB > 0);
		seekA = seekB = -1;
		updateABRepeatInfo(showDisabledInfo);
	}
	bool locked = false;
	if (demuxThr)
		locked = demuxThr->seekMutex.tryLock();
	lastSeekTo = seekTo = pos;
	if (locked)
	{
		if (demuxThr->isDemuxerReady())
			demuxThr->seek(false);
		demuxThr->seekMutex.unlock();
	}
	emit QMPlay2Core.seeked(pos); //Signal for MPRIS2
	fillBufferB = true;
	if (aThr && paused)
		aThr->silence(true);
}
void PlayClass::chStream(const QString &s)
{
	if (s.left(5) == "audio")
		choosenAudioStream = s.right(s.length() - 5).toInt();
	else if (s.left(5) == "video")
		choosenVideoStream = s.right(s.length() - 5).toInt();
	else if (s.left(9) == "subtitles")
		choosenSubtitlesStream = s.right(s.length() - 9).toInt();
	else if (s.left(8) == "fileSubs")
	{
		int idx = s.right(s.length() - 8).toInt();
		if (fileSubsList.count() > idx)
			loadSubsFile(fileSubsList[idx]);
	}
}
void PlayClass::setSpeed(double spd)
{
	if (spd < 0.05 || spd > 100.0)
		speed = 1.0;
	else
		speed = spd;
	speedMessageAndOSD();
}

bool PlayClass::isPlaying() const
{
	return demuxThr && demuxThr->isRunning();
}
#ifdef Q_OS_WIN
bool PlayClass::isNowPlayingVideo() const
{
	return !paused && vThr && vThr->isRunning();
}
#endif

void PlayClass::loadSubsFile(const QString &fileName)
{
	bool subsLoaded = false;
	if (demuxThr && vThr && ass)
	{
		IOController<Reader> reader;
		if (Reader::create(fileName, reader) && reader->size() > 0)
		{
			QByteArray fileData = reader->read(reader->size());

			QTextCodec *codec = QTextCodec::codecForName(QMPlay2Core.getSettings().getByteArray("SubtitlesEncoding"));
			if (codec && codec->name() != "UTF-8")
			{
				codec = QTextCodec::codecForUtfText(fileData, codec);
				if (codec->name() != "UTF-8")
					fileData = codec->toUnicode(fileData).toUtf8();
			}

			sPackets.clear();
			subsMutex.lock();

			vThr->destroySubtitlesDecoder();
			if (!QMPlay2Core.getSettings().getBool("KeepSubtitlesDelay"))
				subtitlesSync = 0.0;
			ass->closeASS();
			ass->clearFonts();

			bool loaded = false;
			const QString fileExt = Functions::fileExt(fileName).toLower();
			if (fileExt == "ass" || fileExt == "ssa")
			{
				/* Wczytywanie katalogu z czcionkami dla wybranego pliku napisów */
				const QString fontPath = Functions::filePath(fileName.mid(7));
				foreach (const QString &fontFile, QDir(fontPath).entryList(QStringList() << "*.ttf" << "*.otf", QDir::Files))
				{
					QFile f(fontPath + fontFile);
					if (f.size() <= 0xA00000 /* 10MiB max */ && f.open(QFile::ReadOnly))
					{
						const QByteArray fontData = f.readAll();
						f.close();
#ifdef USE_QRAWFONT
						const QString fontName = QRawFont(fontData, 0.0).familyName();
						if (!fontName.isEmpty())
							ass->addFont(fontName.toUtf8(), fontData);
#else //For Qt older than 4.8
						const int fontID = QFontDatabase::addApplicationFontFromData(fontData);
						if (fontID != -1)
						{
							const QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontID);
							QFontDatabase::removeApplicationFont(fontID);
							if (!fontFamilies.isEmpty())
								ass->addFont(fontFamilies.first().toUtf8(), fontData);
						}
#endif
					}
				}

				ass->initASS(fileData);
				loaded = true;
			}
			else
			{
				SubsDec *subsdec = SubsDec::create(fileExt);
				if (subsdec)
				{
					loaded = subsdec->toASS(fileData, ass, fps);
					delete subsdec;
				}
			}
			if (loaded)
			{
				fileSubs = fileName;
				subtitlesStream = choosenSubtitlesStream = -2; //"subtitlesStream < -1" oznacza, że wybrano napisy z pliku
				if (!fileSubsList.contains(fileName))
				{
					subsLoaded = true;
					fileSubsList += fileName;
				}
			}
			else
			{
				fileSubs.clear();
				subtitlesStream = choosenSubtitlesStream = -1;
				ass->closeASS();
			}

			subsMutex.unlock();
		}
	}
	if (demuxThr)
		demuxThr->emitInfo();
	if (subsLoaded)
		messageAndOSD(tr("Loaded subtitles") + ": " + Functions::fileName(fileName), false);
}

void PlayClass::messageAndOSD(const QString &txt, bool onStatusBar, double duration)
{
	if (ass && QMPlay2Core.getSettings().getBool("OSD/Enabled"))
	{
		osdMutex.lock();
		ass->getOSD(osd, txt.toUtf8(), duration);
		osdMutex.unlock();
	}
	if (onStatusBar)
		emit QMPlay2Core.statusBarMessage(txt, duration * 1000);
}

inline bool PlayClass::hasVideoStream()
{
	return vThr && demuxThr && demuxThr->demuxer && videoStream > -1;
}

void PlayClass::speedMessageAndOSD()
{
	messageAndOSD(tr("Play speed") + QString(": %1x").arg(speed));
	QMPlay2Core.speedChanged(speed);
}

double PlayClass::getARatio()
{
	if (aRatioName == "auto")
		return demuxThr->demuxer->streamsInfo().at(videoStream)->getAspectRatio();
	if (aRatioName == "sizeDep")
		return (double)demuxThr->demuxer->streamsInfo().at(videoStream)->W / (double)demuxThr->demuxer->streamsInfo().at(videoStream)->H;
	return aRatioName.toDouble();
}
inline double PlayClass::getSAR()
{
	return demuxThr->demuxer->streamsInfo().at(videoStream)->sample_aspect_ratio;
}

void PlayClass::flushAssEvents()
{
	subsMutex.lock();
	if (ass && subtitlesStream > -1)
		ass->flushASSEvents();
	subsMutex.unlock();
}
void PlayClass::clearSubtitlesBuffer()
{
	sPackets.clear();
	flushAssEvents();
}

void PlayClass::stopVThr()
{
	if (vThr)
	{
		stopVDec();
		if (vThr)
		{
			vThr->stop();
			vThr = NULL;
		}
	}
}
void PlayClass::stopAThr()
{
	doSilenceOnStart = false;
	if (aThr)
	{
		stopADec();
		if (aThr)
		{
			aThr->stop();
			aThr = NULL;
		}
	}
}
inline void PlayClass::stopAVThr()
{
	stopVThr();
	stopAThr();
}

void PlayClass::stopVDec()
{
	if (vThr && vThr->dec)
	{
		if (vThr->lock())
		{
			vThr->destroyDec();
			vThr->destroySubtitlesDecoder();
		}
		else
		{
			vThr->stop(true);
			vThr = NULL;
		}
	}
	frame_last_pts = frame_last_delay = 0.0;
	subtitlesStream = -1;
	nextFrameB = false;
	delete ass; //wywołuje też closeASS(), sMutex nie potrzebny, bo vThr jest zablokowany (mutex przed sMutex)
	ass = NULL;
	fps = 0.0;
}
void PlayClass::stopADec()
{
	if (aThr && aThr->dec)
	{
		if (aThr->lock())
			aThr->destroyDec();
		else
		{
			aThr->stop(true);
			aThr = NULL;
		}
	}
	audio_current_pts = skipAudioFrame = audio_last_delay = 0.0;
}

void PlayClass::setFlip()
{
	if (vThr && vThr->setFlip() && !spherical)
	{
		vThr->processParams();
		flipRotMsg();
	}
}
void PlayClass::flipRotMsg()
{
	bool canAddRotate90 = rotate90;
	QString str;
	switch (flip)
	{
		case 0:
			if (!rotate90)
				str += tr("No image rotation");
			break;
		case Qt::Horizontal:
			str = tr("Horizontal flip");
			break;
		case Qt::Vertical:
			str = tr("Vertical flip");
			break;
		case Qt::Horizontal + Qt::Vertical:
			if (rotate90)
				str = tr("Rotation 270°");
			else
				str = tr("Rotation 180°");
			canAddRotate90 = false;
			break;
	}
	if (canAddRotate90)
	{
		const QString rotate90Str = tr("Rotation 90°");
		if (!str.isEmpty())
			str.prepend(rotate90Str + "\n");
		else
			str = rotate90Str;
	}
	messageAndOSD(str, true, 0.75);
}

void PlayClass::clearPlayInfo()
{
	emit chText(tr("Stopped"));
	emit playStateChanged(false);
	emit updateWindowTitle();
	if (QMPlay2Core.getSettings().getBool("ShowCovers"))
		emit updateImage();
	emit clearInfo();
}

void PlayClass::updateABRepeatInfo(bool showDisabledInfo)
{
	QString abMsg;
	if (seekA < 0 && seekB < 0)
	{
		if (showDisabledInfo)
			abMsg = tr("disabled");
	}
	else
	{
		abMsg = tr("from") + " " + Functions::timeToStr(seekA);
		if (seekB > -1)
			abMsg += " " + tr("to") + " " + Functions::timeToStr(seekB);
	}
	if (!abMsg.isEmpty())
		emit QMPlay2Core.statusBarMessage(tr("A-B Repeat") + " " + abMsg, 2500);
}

void PlayClass::suspendWhenFinished(bool b)
{
	doSuspend = b;
}

void PlayClass::saveCover()
{
	if (demuxThr)
		QMPlay2GUI.saveCover(demuxThr->getCoverFromStream());
}
void PlayClass::settingsChanged(int page, bool page3Restart)
{
	switch (page)
	{
		case 1:
			if (demuxThr)
			{
				if (!QMPlay2Core.getSettings().getBool("ShowCovers"))
					emit updateImage();
				else
					demuxThr->loadImage();
			}
			break;
		case 2:
			restart();
			break;
		case 3:
			if (page3Restart)
				restart();
			break;
		case 4: //napisy
			if (ass && subtitlesStream != -1)
			{
				ass->setASSStyle();
				if (vThr)
				{
					vThr->updateSubs();
					vThr->processParams();
				}
			}
			break;
		case 5: //OSD
			if (ass)
			{
				ass->setOSDStyle();
				osdMutex.lock();
				if (osd)
					ass->getOSD(osd, osd->text(), osd->left_duration());
				osdMutex.unlock();
			}
			break;
		case 6: //video filters
			if (vThr)
				vThr->initFilters();
			break;
	}
}
void PlayClass::videoResized(int w, int h)
{
	if (ass && (w != videoWinW || h != videoWinH))
	{
		ass->setWindowSize(w, h);
		if (QMPlay2Core.getSettings().getBool("OSD/Enabled"))
		{
			osdMutex.lock();
			if (osd)
				ass->getOSD(osd, osd->text(), osd->left_duration());
			osdMutex.unlock();
		}
		if (vThr)
			vThr->updateSubs();
	}
	videoWinW = w;
	videoWinH = h;
}

void PlayClass::setVideoEqualizer(int b, int s, int c, int h)
{
	Brightness = b;
	Saturation = s;
	Contrast = c;
	Hue = h;
	if (vThr)
	{
		vThr->setVideoEqualizer();
		vThr->processParams();
	}
}

void PlayClass::setAB()
{
	if (demuxThr && demuxThr->isDemuxerReady() && demuxThr->canSeek())
	{
		const int intPos = pos;
		if (seekA < 0)
			seekA = intPos;
		else if (seekB < 0)
		{
			if (seekA != intPos)
				seekB = intPos;
		}
		else
			seekA = seekB = -1;
		updateABRepeatInfo(true);
	}
}
void PlayClass::speedUp()
{
	speed += 0.05;
	if (speed >= 0.951 && speed <= 1.049)
		speed = 1.0;
	speedMessageAndOSD();
}
void PlayClass::slowDown()
{
	speed -= 0.05;
	if (speed >= 0.951 && speed <= 1.049)
		speed = 1.0;
	else if (speed < 0.05)
		speed = 0.05;
	speedMessageAndOSD();
}
void PlayClass::setSpeed()
{
	bool ok;
	double s = QInputDialog::getDouble(NULL, tr("Play speed"), tr("Set playback speed (sec.)"), speed, 0.05, 100.0, 2, &ok);
	if (ok)
	{
		speed = s;
		speedMessageAndOSD();
	}
}
void PlayClass::zoomIn()
{
	if (vThr)
	{
		zoom += 0.05;
		if (ass)
			ass->setZoom(zoom);
		messageAndOSD("Zoom: " + QString::number(zoom));
		vThr->setZoom();
		vThr->processParams();
	}
}
void PlayClass::zoomOut()
{
	if (vThr && zoom - 0.05 > 0.0)
	{
		zoom -= 0.05;
		if (ass)
			ass->setZoom(zoom);
		messageAndOSD("Zoom: " + QString::number(zoom));
		vThr->setZoom();
		vThr->processParams();
	}
}
void PlayClass::zoomReset()
{
	if (zoom != 1.0)
	{
		zoom = 1.0;
		if (vThr)
		{
			if (ass)
				ass->setZoom(zoom);
			messageAndOSD("Zoom: " + QString::number(zoom));
			vThr->setZoom();
			vThr->processParams();
		}
	}
}
void PlayClass::aRatio()
{
	aRatioName = sender()->objectName();
	QString msg_txt = tr("Aspect ratio") + ": " + ((QAction *)sender())->text().remove('&');
	if (hasVideoStream())
	{
		const double aspect_ratio = getARatio();
		if (ass)
			ass->setARatio(aspect_ratio);
		messageAndOSD(msg_txt);
		vThr->setARatio(aspect_ratio, getSAR());
		vThr->processParams();
	}
	else
		messageAndOSD(msg_txt);
}
void PlayClass::volume(int l, int r)
{
	vol[0] = l / 100.0;
	vol[1] = r / 100.0;
	if (!muted)
	{
		emit QMPlay2Core.volumeChanged((vol[0] + vol[1]) / 2.0);
		messageAndOSD(tr("Volume") + ": " + QString::number((l + r) / 2) + "%");
	}
}
void PlayClass::toggleMute()
{
	muted = !muted;
	if (!muted)
		volume(vol[0] * 100, vol[1] * 100);
	else
	{
		messageAndOSD(tr("Muted sound"));
		emit QMPlay2Core.volumeChanged(0.0);
	}
}
void PlayClass::slowDownVideo()
{
	if (videoSync <= -maxThreshold)
		return;
	videoSync -= 0.1;
	if (videoSync < 0.1 && videoSync > -0.1)
		videoSync = 0.0;
	messageAndOSD(tr("Video delay") + ": " + QString::number(videoSync) + "s");
}
void PlayClass::speedUpVideo()
{
	if (videoSync >= maxThreshold)
		return;
	videoSync += 0.1;
	if (videoSync < 0.1 && videoSync > -0.1)
		videoSync = 0.0;
	messageAndOSD(tr("Video delay") + ": " + QString::number(videoSync) + "s");
}
void PlayClass::setVideoSync()
{
	bool ok;
	double vs = QInputDialog::getDouble(NULL, tr("Video delay"), tr("Set video delay (sec.)"), videoSync, -maxThreshold, maxThreshold, 1, &ok);
	if (!ok)
		return;
	videoSync = vs;
	messageAndOSD(tr("Video delay") + ": " + QString::number(videoSync) + "s");
}
void PlayClass::slowDownSubs()
{
	if (subtitlesStream == -1 || (subtitlesSync - 0.1 < 0 && fileSubs.isEmpty()))
		return;
	subtitlesSync -= 0.1;
	if (subtitlesSync < 0.1 && subtitlesSync > -0.1)
		subtitlesSync = 0.;
	messageAndOSD(tr("Subtitles delay") + ": " + QString::number(subtitlesSync) + "s");
}
void PlayClass::speedUpSubs()
{
	if (subtitlesStream == -1)
		return;
	subtitlesSync += 0.1;
	if (subtitlesSync < 0.1 && subtitlesSync > -0.1)
		subtitlesSync = 0.;
	messageAndOSD(tr("Subtitles delay") + ": " + QString::number(subtitlesSync) + "s");
}
void PlayClass::setSubtitlesSync()
{
	if (subtitlesStream == -1)
		return;
	bool ok;
	double ss = QInputDialog::getDouble(NULL, tr("Subtitles delay"), tr("Set subtitles delay (sec.)"), subtitlesSync, fileSubs.isEmpty() ? 0 : -2147483647, 2147483647, 2, &ok);
	if (!ok)
		return;
	subtitlesSync = ss;
	messageAndOSD(tr("Subtitles delay") + ": " + QString::number(subtitlesSync) + "s");
}
void PlayClass::biggerSubs()
{
	if (ass)
	{
		if (subtitlesScale > maxThreshold - 0.05)
			return;
		subtitlesScale += 0.05;
		ass->setFontScale(subtitlesScale);
		messageAndOSD(tr("Subtitles size") + ": " + QString::number(subtitlesScale));
		if (vThr)
		{
			vThr->updateSubs();
			vThr->processParams();
		}
	}
}
void PlayClass::smallerSubs()
{
	if (ass)
	{
		if (subtitlesScale <= 0.05)
			return;
		subtitlesScale -= 0.05;
		ass->setFontScale(subtitlesScale);
		messageAndOSD(tr("Subtitles size") + ": " + QString::number(subtitlesScale));
		if (vThr)
		{
			vThr->updateSubs();
			vThr->processParams();
		}
	}
}
void PlayClass::toggleAVS(bool b)
{
	if (sender()->objectName() == "toggleAudio")
	{
		audioEnabled = b;
		audioStream = -1;
		if (!audioEnabled)
			messageAndOSD(tr("Sound off"));
	}
	else if (sender()->objectName() == "toggleVideo")
	{
		videoEnabled = b;
		videoStream = -1;
	}
	else if (sender()->objectName() == "toggleSubtitles")
	{
		subtitlesEnabled = b;
		subtitlesStream = -1;
		if (!subtitlesEnabled)
			messageAndOSD(tr("Subtitles off"));
	}
	reload = true;
}
void PlayClass::setSpherical(bool b)
{
	spherical = b;
	if (vThr)
	{
		if (vThr->setSpherical())
			vThr->processParams();
		else if (spherical)
		{
			QAction *act = qobject_cast<QAction *>(sender());
			if (act)
				act->trigger();
			messageAndOSD(tr("Spherical view is not supported by this video output module"), true, 2.0);
		}
	}
}
void PlayClass::setHFlip(bool b)
{
	if (b)
		flip |= Qt::Horizontal;
	else
		flip &= ~Qt::Horizontal;
	setFlip();
}
void PlayClass::setVFlip(bool b)
{
	if (b)
		flip |= Qt::Vertical;
	else
		flip &= ~Qt::Vertical;
	setFlip();
}
void PlayClass::setRotate90(bool b)
{
	rotate90 = b;
	if (vThr)
	{
		if (vThr->setRotate90())
		{
			if (!spherical)
			{
				vThr->processParams();
				flipRotMsg();
			}
		}
		else if (rotate90)
		{
			QAction *act = qobject_cast<QAction *>(sender());
			if (act)
				act->trigger();
			messageAndOSD(tr("Rotation is not supported by this video output module"), true, 2.0);
		}
	}
}
void PlayClass::screenShot()
{
	if (vThr)
	{
		vThr->setDoScreenshot();
		emptyBufferCond.wakeAll();
	}
}
void PlayClass::nextFrame()
{
	if (videoStream > -1 && stopPauseMutex.tryLock())
	{
		paused = false;
		nextFrameB = fillBufferB = true;
		stopPauseMutex.unlock();
	}
}

void PlayClass::frameSizeUpdated(int w, int h) //jeżeli rozmiar obrazu zmieni się podczas odtwarzania
{
	if (hasVideoStream())
	{
		StreamInfo *streamInfo = demuxThr->demuxer->streamsInfo().at(videoStream);
		streamInfo->W = w;
		streamInfo->H = h;
		const double aspect_ratio = getARatio();
		if (ass)
			ass->setARatio(aspect_ratio);
		vThr->setARatio(aspect_ratio, getSAR());
		demuxThr->emitInfo();
	}
	if (vThr) //"demuxThr->demuxer" can be closed, but "vThr" can be in waiting state
	{
		vThr->setFrameSize(w, h);
		vThr->initFilters();
		vThr->processParams();
		vThr->frameSizeUpdateUnlock();
	}
}
void PlayClass::aRatioUpdated(double sar) //jeżeli współczynnik proporcji zmieni się podczas odtwarzania
{
	if (hasVideoStream())
	{
		demuxThr->demuxer->streamsInfo().at(videoStream)->sample_aspect_ratio = sar;
		const double aspect_ratio = getARatio();
		if (ass)
			ass->setARatio(aspect_ratio);
		vThr->lock();
		vThr->setDeleteOSD();
		vThr->setARatio(aspect_ratio, getSAR());
		vThr->processParams();
		vThr->unlock();
		demuxThr->emitInfo();
	}
}
void PlayClass::pixelFormatUpdated(const QByteArray &pixFmt)
{
	if (hasVideoStream())
	{
		demuxThr->demuxer->streamsInfo().at(videoStream)->format = pixFmt;
		demuxThr->emitInfo();
	}
}

void PlayClass::demuxThrFinished()
{
	timTerminate.stop();

	if (!stopPauseMutex.tryLock())
		doSilenceBreak = true; //jeżeli ta funkcja jest wywołana spod "processEvents()" z "silence()", po tym wywołaniu ma się natychmiast zakończyć

	if (demuxThr->demuxer) //Jeżeli wątek się zakończył po upływie czasu timera (nieprawidłowo zakończony), to demuxer nadal istnieje
		demuxThr->end();

	bool br  = demuxThr->demuxer.isAborted(), err = demuxThr->err;
	delete demuxThr;
	demuxThr = NULL;

	stopPauseMutex.unlock();

	if (restartSeekTo < 0.0) //jeżeli nie restart odtwarzania
		fileSubsList.clear(); //czyści listę napisów z pliku
	fileSubs.clear();

	const bool showDisabledInfo = (seekA > 0 || seekB > 0);
	seekA = seekB = -1;
	updateABRepeatInfo(showDisabledInfo);

	url.clear();
	pos = -1.0;

	emit updateBufferedRange(-1, -1);
	emit updateLength(0);
	emit updatePos(0);

	bool clr = true, canDoSuspend = !br;

	if (!newUrl.isEmpty() && !quitApp)
	{
		if (restartSeekTo >= 0.0) //jeżeli restart odtwarzania
			stopAVThr();
		emit clearCurrentPlaying();
		canDoSuspend = false;
		play(newUrl);
		newUrl.clear();
		clr = false;
	}
	else
	{
		if (br || quitApp)
			emit clearCurrentPlaying();

		if (!br && !quitApp)
		{
			if ((err && !ignorePlaybackError) || doSuspend) //Jeżeli wystąpił błąd i nie jest on ignorowany lub jeżeli komputer ma być uśpiony
				stopAVThr();
			else
			{
				emit playNext(err);
				clr = false;
			}
			emit clearCurrentPlaying();
		}
		else
			stopAVThr();
	}

	if (clr)
		clearPlayInfo();
	else
	{
		if (aThr)
			aThr->clearVisualizations();
		emit updateBuffered(-1, -1, 0.0, 0.0);
	}

	if (quitApp)
	{
		QCoreApplication::processEvents();
		emit quit();
	}
	else if (doSuspend)
	{
		doSuspend = false;
		emit uncheckSuspend();
		if (canDoSuspend)
			QMPlay2CoreClass::suspend();
	}
}

void PlayClass::timTerminateFinished()
{
	timTerminate.stop();
	if (demuxThr && demuxThr->isRunning())
	{
		demuxThr->terminate();
		demuxThr->wait(1000);
		emit QMPlay2Core.statusBarMessage(tr("Playback has been incorrectly terminated!"), 2000);
	}
	emit QMPlay2Core.restoreCursor();
}

static Decoder *loadStream(const QList<StreamInfo *> &streams, const int choosenStream, int &stream, const QMPlay2MediaType type, const QString &lang, Writer *writer = NULL)
{
	Decoder *dec = NULL;
	const bool subtitles = type == QMPLAY2_TYPE_SUBTITLE;
	if (choosenStream >= 0 && choosenStream < streams.count() && streams[choosenStream]->type == type)
	{
		if (streams[choosenStream]->must_decode || !subtitles)
			dec = Decoder::create(*streams[choosenStream], writer, QMPlay2GUI.getModules("decoders", 7));
		if (dec || subtitles)
			stream = choosenStream;
	}
	else
	{
		int defaultStream = -1, choosenLangStream = -1;
		for (int i = 0; i < streams.count(); ++i)
		{
			if (streams[i]->type == type)
			{
				if (defaultStream < 0 && streams[i]->is_default)
					defaultStream = i;
				if (!lang.isEmpty() && choosenLangStream < 0)
				{
					foreach (const QMPlay2Tag &tag, streams[i]->other_info)
					{
						if (tag.first.toInt() == QMPLAY2_TAG_LANGUAGE)
						{
							if (tag.second == lang || QMPlay2Core.getLanguagesMap().key(tag.second) == lang)
								choosenLangStream = i;
							break;
						}
					}
				}
			}
		}
		if (choosenLangStream > -1)
			defaultStream = choosenLangStream;
		for (int i = 0; i < streams.count(); ++i)
		{
			StreamInfo &streamInfo = *streams[i];
			if (streamInfo.type == type && (defaultStream == -1 || i == defaultStream))
			{
				if (streamInfo.must_decode || !subtitles)
					dec = Decoder::create(streamInfo, writer, QMPlay2GUI.getModules("decoders", 7));
				if (dec || subtitles)
				{
					stream = i;
					break;
				}
			}
		}
	}
	return dec;
}
void PlayClass::load(Demuxer *demuxer)
{
	const QList<StreamInfo *> streams = demuxer->streamsInfo();
	Decoder *dec = NULL;

	if (videoStream < 0 || (choosenVideoStream > -1 && choosenVideoStream != videoStream)) //load video
	{
		vPackets.clear();
		stopVDec(); //lock
		if (videoEnabled)
			dec = loadStream(streams, choosenVideoStream, videoStream, QMPLAY2_TYPE_VIDEO, QString(), vThr ? vThr->getHWAccelWriter() : NULL);
		else
			dec = NULL;
		if (dec)
		{
			const bool canEmitVideoStarted = !vThr;
			if (vThr && (vThr->getHWAccelWriter() != dec->HWAccel()))
				stopVThr();
			if (!vThr)
			{
				vThr = new VideoThr(*this, dec->HWAccel(), QMPlay2GUI.getModules("videoWriters", 5));
				vThr->setSyncVtoA(QMPlay2Core.getSettings().getBool("SyncVtoA"));
			}
			if (vThr->isRunning())
			{
				const double aspect_ratio = getARatio();

				vThr->setFrameSize(streams[videoStream]->W, streams[videoStream]->H);
				vThr->setARatio(aspect_ratio, getSAR());
				vThr->setVideoEqualizer();
				vThr->setDec(dec);
				vThr->setZoom();
				vThr->setSpherical(); //TODO: Disable when not supported
				vThr->setRotate90(); //TODO: Disable when not supported
				vThr->setFlip();
				vThr->initFilters(false);

				if (!vThr->processParams())
					dec = NULL;
				else
				{
					if (!vThr->getHWAccelWriter())
						dec->setSupportedPixelFormats(vThr->getSupportedPixelFormats());

					fps = streams[videoStream]->FPS;
					ass = new LibASS(QMPlay2Core.getSettings());
					ass->setWindowSize(videoWinW, videoWinH);
					ass->setFontScale(subtitlesScale);
					ass->setARatio(aspect_ratio);
					ass->setZoom(zoom);

#if defined Q_OS_WIN && !defined Q_OS_WIN64
					if (LibASS::slowFontCacheUpdate() && firsttimeUpdateCache)
					{
						UpdateFC updateFC(ass);
						firsttimeUpdateCache = false;
					}
					else
#endif
						ass->initOSD();

					if (canEmitVideoStarted)
						emit videoStarted();

					if (reload)
						seekTo = SEEK_STREAM_RELOAD;
				}
				vThr->unlock();
			}
			else
			{
				delete dec;
				dec = NULL;
			}
		}
		if (!dec)
		{
			choosenVideoStream = videoStream = -1;
			stopVThr();
		}
	}

	if (audioStream < 0 || (choosenAudioStream > -1 && choosenAudioStream != audioStream)) //load audio
	{
		aPackets.clear();
		stopADec(); //lock
		if (audioEnabled)
			dec = loadStream(streams, choosenAudioStream, audioStream, QMPLAY2_TYPE_AUDIO, choosenAudioLang);
		else
			dec = NULL;
		if (dec)
		{
			if (!aThr)
				aThr = new AudioThr(*this, QMPlay2GUI.getModules("audioWriters", 5));
			if (aThr->isRunning())
			{
				aThr->setDec(dec);
				quint32 srate = 0;
				quint8 chn = 0;
				if (QMPlay2Core.getSettings().getBool("ForceSamplerate"))
					srate = QMPlay2Core.getSettings().getUInt("Samplerate");
				if (QMPlay2Core.getSettings().getBool("ForceChannels"))
					chn = QMPlay2Core.getSettings().getUInt("Channels");

				if (!aThr->setParams(streams[audioStream]->channels, streams[audioStream]->sample_rate, chn, srate))
					dec = NULL;
				else if (reload)
					seekTo = SEEK_STREAM_RELOAD;
				if (doSilenceOnStart)
				{
					aThr->silence(true);
					doSilenceOnStart = false;
				}
				aThr->unlock();
			}
			else
			{
				delete dec;
				dec = NULL;
			}
		}
		if (!dec)
		{
			choosenAudioStream = audioStream = -1;
			stopAThr();
		}
	}

	//load subtitles
	if (subtitlesStream == -1 || (choosenSubtitlesStream > -1 && choosenSubtitlesStream != subtitlesStream))
	{
		if (!QMPlay2Core.getSettings().getBool("KeepSubtitlesDelay"))
			subtitlesSync = 0.0;
		fileSubs.clear();
		clearSubtitlesBuffer();
		if (videoStream >= 0 && vThr)
		{
			subsMutex.lock();
			vThr->destroySubtitlesDecoder();
			ass->closeASS();
			ass->clearFonts();
			subsMutex.unlock();

			if (subtitlesEnabled && fileSubsList.count() && choosenSubtitlesStream < 0)
				loadSubsFile(fileSubsList[fileSubsList.count() - 1]);
			else
			{
				if (subtitlesEnabled)
					dec = loadStream(streams, choosenSubtitlesStream, subtitlesStream, QMPLAY2_TYPE_SUBTITLE, choosenSubtitlesLang);
				else
				{
					subtitlesStream = -1;
					dec = NULL;
				}
				if (vThr && subtitlesStream > -1)
				{
					if (subtitlesSync < 0.0)
						subtitlesSync = 0.0;
					subsMutex.lock();
					if (dec)
						vThr->setSubtitlesDecoder(dec);
					QByteArray assHeader = streams[subtitlesStream]->data;
					if (!assHeader.isEmpty() && (streams[subtitlesStream]->codec_name == "ssa" || streams[subtitlesStream]->codec_name == "ass"))
					{
						for (int i = 0; i < streams.count(); ++i)
							if (streams[i]->type == QMPLAY2_TYPE_ATTACHMENT && (streams[i]->codec_name == "TTF" || streams[i]->codec_name == "OTF") && streams[i]->data.size())
								ass->addFont(streams[i]->title, streams[i]->data);
					}
					else
						assHeader.clear();
					ass->initASS(assHeader);
					if (reload)
						seekTo = SEEK_STREAM_RELOAD;
					subsMutex.unlock();
				}
				else
				{
					subtitlesStream = choosenSubtitlesStream = -1;
					if (dec)
					{
						delete dec;
						dec = NULL;
					}
				}
			}
		}
	}

	reload = false;
	loadMutex.unlock();
}
