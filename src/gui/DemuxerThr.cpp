/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

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
#include <StreamMuxer.hpp>
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
    return QMPlay2Core.getSettingsDir() + "Covers/" + QCryptographicHash::hash(QByteArray((album.isEmpty() ? title.toUtf8() : album.toUtf8()) + artist.toUtf8()), QCryptographicHash::Md5).toHex();
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
        QImage img = demuxerImage.isEmpty()
            ? QImage()
            : QImage::fromData(demuxerImage)
        ;
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
            if (img.isNull() && (!artist.isEmpty() || title.contains(QStringLiteral(" - "))) && (!title.isEmpty() || !album.isEmpty())) //Ładowanie okładki z cache
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
                    playC.sPackets.seekTo(seekTo, backward); // Ignore error on subtitles stream (subtitles can be missing near to specified timestamp)

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

        if (doDemuxerSeek)
            stopRecording();

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
                changeStatusText();
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
    demuxer->selectStreams({playC.audioStream, playC.videoStream, playC.subtitlesStream});
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

void DemuxerThr::startRecording()
{
    if (isDemuxerReady())
        m_recording = true;
    else if (!m_recording)
        emit recording(false, true);
}
void DemuxerThr::stopRecording()
{
    m_recording = false;
}

void DemuxerThr::run()
{
    const auto origUrl = url;

    emit playC.chText(tr("Opening"));
    emit playC.setCurrentPlaying();

    emit QMPlay2Core.busyCursor();
    Functions::getDataIfHasPluginPrefix(url, &url, &name, nullptr, &ioCtrl);
    emit QMPlay2Core.restoreCursor();
    if (ioCtrl.isAborted())
        return end();

    if (Functions::isResourcePlaylist(url))
    {
        end();
        emit QMPlay2Core.processParam("remove", origUrl);
        emit QMPlay2Core.processParam("open", url);
        return;
    }

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
        {
            if (streamInfo->params->codec_type == AVMEDIA_TYPE_VIDEO) //napisów szukam tylko wtedy, jeżeli jest strumień wideo
            {
                const QString directory = Functions::filePath(url.mid(7));

                const QString fName = Functions::fileName(url, false).replace('_', ' ');
                auto maybeAppendFiles = [this, filter, fName](const QString  &directory) {
                    for (const QFileInfo &subsFileInfo : QDir(directory).entryInfoList(filter, QDir::Files))
                    {
                        const QString subsFName = Functions::fileName(subsFileInfo.fileName(), false).replace('_', ' ');
                        if (subsFName.contains(fName, Qt::CaseInsensitive) || fName.contains(subsFName, Qt::CaseInsensitive))
                        {
                            const QString fileSubsUrl = Functions::Url(subsFileInfo.filePath());
                            if (!playC.fileSubsList.contains(fileSubsUrl))
                                playC.fileSubsList += fileSubsUrl;
                        }
                    }
                };

                maybeAppendFiles(directory);

                const QStringList dirs {"ass", "srt", "sub", "subs", "subtitles"};
                for (const QFileInfo &dirInfo : QDir(directory).entryInfoList(dirs, QDir::Dirs | QDir::NoDotAndDotDot))
                    maybeAppendFiles(dirInfo.filePath());

                break;
            }
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
    if (!QMPlay2Core.getSettings().getBool("ReplayGain/Enabled"))
    {
        playC.replayGain = 1.0;
    }
    else if (!demuxer->getReplayGain(QMPlay2Core.getSettings().getBool("ReplayGain/Album"), gain_db, peak))
    {
        playC.replayGain = pow(10.0, QMPlay2Core.getSettings().getDouble("ReplayGain/PreampNoMetadata") / 20.0);
    }
    else
    {
        playC.replayGain = pow(10.0, gain_db / 20.0) * pow(10.0, QMPlay2Core.getSettings().getDouble("ReplayGain/Preamp") / 20.0);
        if (QMPlay2Core.getSettings().getBool("ReplayGain/PreventClipping") && peak * playC.replayGain > 1.0)
            playC.replayGain = 1.0 / peak;
    }

    if ((unknownLength = demuxer->length() < 0.0))
        updateBufferedSeconds = false;

    emit playC.updateLength(round(demuxer->length()));
    changeStatusText();
    emit playC.playStateChanged(true);

    localStream = demuxer->localStream();
    time = localStream ? 0.0 : Functions::gettime();

    const int minBuffered = demuxer->dontUseBuffer()
        ? 1
        : localStream
          ? qMax(1, minBuffSizeLocal)
          : 0 // Use "forwardTime"
    ;
    const double forwardTime = (minBuffered == 0)
        ? unknownLength
          ? qMax(0.1, m_minBuffTimeNetworkLive)
          : qMax(0.1, m_minBuffTimeNetwork)
        : 0.0
    ;
    double backwardTime = 0.0;
    int vS, aS;
    double vT, aT;

    demuxerReady = true;

    updateCoverAndPlaying(false);
    connect(&QMPlay2Core, SIGNAL(updateCover(QString, QString, QString, QByteArray)), this, SLOT(updateCover(QString, QString, QString, QByteArray)));

    QObject o;
    connect(&QMPlay2Core, &QMPlay2CoreClass::updateInformationPanel,
            &o, [this] {
        Q_ASSERT(QThread::currentThread() != thread());
        if (demuxer)
            emitInfo();
    });

    if (forwardTime > 0.0)
    {
        Q_ASSERT(!localStream);
        double percent = 50.0;
        switch (QMPlay2Core.getSettings().getUInt("BackwardBuffer"))
        {
            case 0:
                percent = 10;
                break;
            case 1:
                percent = 25;
                break;
            case 3:
                percent = 75;
                break;
            case 4:
                percent = 100;
                break;
        }
        backwardTime = qMax(0.1, m_minBuffTimeNetwork) * percent / 100.0;
    }
    PacketBuffer::setBackwardTime(backwardTime);

    if (!localStream)
        setPriority(QThread::LowPriority); //Network streams should have low priority, because slow single core CPUs have problems with smooth video playing during buffering

    DemuxerTimer demuxerTimer(*this);

    if (stillImage && playC.paused)
        playC.paused = false;

    const QString demuxerName = demuxer->name();

    const bool isChunkedLive = (!localStream && unknownLength && (demuxerName == "hls" || demuxerName == "dash"));
    QElapsedTimer waitForDataTimer;
    bool firstWaitForData = true;
    bool canWaitForData = true;

    QHash<int, int> recStreamsMap;

    if (unknownLength)
        emit allowRecording(true);

    auto handleRecording = [&](bool startAndStop) {
        if (startAndStop && m_recording && !m_recMuxer)
            startRecordingInternal(recStreamsMap);
        else if (!m_recording && m_recMuxer)
            stopRecordingInternal(recStreamsMap);
    };

    while (!demuxer.isAborted())
    {
        {
            QMutexLocker seekLocker(&seekMutex);
            seek(true);
            if (playC.seekTo == SEEK_REPEAT)
                break; //Repeat seek failed - break.
        }

        handleRecording(true);

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
        const double remainingDuration = getAVBuffersSize(vS, aS, vT, aT);
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

        if (minBuffered == 0 && !localStream && !playC.waitForData && !playC.endOfStream && playIfBuffered > 0.0 && emptyBuffers(vS, aS))
        {
            playC.waitForData = true;
            updateBufferedTime = 0.0;
        }
        else if
        (
            playC.waitForData &&
            (
                playC.endOfStream                                                  ||
                bufferedAllPackets(vT, aT, forwardTime)                            || //Buffer is full
                (remainingDuration >= playIfBuffered)                              ||
                (qFuzzyIsNull(remainingDuration) && bufferedAllPackets(vS, aS, 2))    //Buffer has at least 2 packets, but still without length (in seconds) information
            )
        )
        {
            if (isChunkedLive && !firstWaitForData && canWaitForData && !playC.endOfStream)
            {
                // Wait a bit longer for HLS streams to prevent stuttering on every HLS chunk. Do a short
                // sleep and continue, because reading from demuxer might block for HLS chunk length.
                if (!waitForDataTimer.isValid())
                    waitForDataTimer.start();
                if (waitForDataTimer.elapsed() / 1e3 <= playIfBuffered)
                {
                    msleep(50);
                    continue;
                }
                waitForDataTimer.invalidate();
                canWaitForData = false;
            }

            playC.waitForData = false;
            if (!paused)
                playC.emptyBufferCond.wakeAll();

            firstWaitForData = false;
        }
        else
        {
            canWaitForData = true;
        }

        if (playC.endOfStream || ((minBuffered > 0) ? bufferedAllPackets(vS, aS, minBuffered) : bufferedAllPackets(vT, aT, forwardTime)))
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
                {
                    msleep(15);
                    handleRecording(false);
                }
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

            if (int recStreamIdx = recStreamsMap.value(streamIdx, -1); recStreamIdx > -1)
            {
                Q_ASSERT(m_recMuxer);
                m_recMuxer->write(packet, recStreamIdx);
            }

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
            getAVBuffersSize(vS, aS, vT, aT);
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

    if (m_recMuxer)
        stopRecordingInternal(recStreamsMap);

    emit allowRecording(false);

    emit QMPlay2Core.updatePlaying(false, title, artist, album, round(demuxer->length()), false, updatePlayingName);

    playC.endOfStream = playC.canUpdatePos = false; //to musi tu być!
    end();
}

void DemuxerThr::startRecordingInternal(QHash<int, int> &recStreamsMap)
{
    QList<StreamInfo *> recStreamsInfo;
    Q_ASSERT(recStreamsMap.isEmpty());
    Q_ASSERT(m_recording);
    Q_ASSERT(!m_recMuxer);
    auto pushStream = [&](int streamIdx) {
        if (streamIdx > -1)
        {
            recStreamsMap[streamIdx] = recStreamsInfo.size();
            recStreamsInfo.push_back(demuxer->streamsInfo().at(streamIdx));
        }
    };
    pushStream(playC.videoStream);
    pushStream(playC.audioStream);
    pushStream(playC.subtitlesStream);

    const auto [ext, fmt] = Functions::determineExtFmt(recStreamsInfo);

    const auto dir = QMPlay2Core.getSettings().getString("OutputFilePath");
    const auto fileName = Functions::getSeqFile(dir, "." + ext, "rec");
    m_recMuxer = std::make_unique<StreamMuxer>(dir + "/" + fileName, recStreamsInfo, fmt, true);
    if (m_recMuxer->isOk())
    {
        emit recording(true, false, fileName);
        auto writePackets = [&](PacketBuffer &packets, int packetsStreamIdx) {
            if (int recStreamIdx = recStreamsMap.value(packetsStreamIdx, -1); recStreamIdx > -1)
            {
                packets.iterate([&](const Packet &packet) {
                    m_recMuxer->write(packet, recStreamIdx);
                });
            }
        };
        writePackets(playC.vPackets, playC.videoStream);
        writePackets(playC.aPackets, playC.audioStream);
        writePackets(playC.sPackets, playC.subtitlesStream);
    }
    else
    {
        m_recording = false;
        m_recMuxer.reset();
        recStreamsMap.clear();
        emit recording(false, true);
    }
    changeStatusText();
}
void DemuxerThr::stopRecordingInternal(QHash<int, int> &recStreamsMap)
{
    Q_ASSERT(m_recMuxer);
    m_recording = false;
    m_recMuxer.reset();
    recStreamsMap.clear();
    emit recording(false, false);
    changeStatusText();
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
            changeStatusText();
            emit playC.playStateChanged(false);
            playC.emptyBufferCond.wakeAll();
        }
    }
    else if (paused)
    {
        paused = demuxerPaused = false;
        changeStatusText();
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
    QString lyrics = QMPlay2Core.getDescriptionForUrl(url);
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
            case QMPLAY2_TAG_LYRICS:
                if (lyrics.isEmpty())
                    lyrics = tag.second;
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
        emit QMPlay2Core.updatePlaying(true, title, artist, album, round(demuxer->length()), showCovers && !hasCover, updatePlayingName, lyrics);
    }
}

void DemuxerThr::addStreamsMenuString(QStringList &streamsMenu, const QString &idxStr, const QString &link, bool current, const QString &additional)
{
    QString streamMenu = idxStr;
    if (!additional.isEmpty())
        streamMenu.push_back(additional);
    streamMenu.push_back("\n");
    streamMenu.push_back(link);
    if (current)
        streamMenu.push_back("\n");
    streamsMenu.push_back(std::move(streamMenu));
}

static void printOtherInfo(const QVector<QMPlay2Tag> &other_info, QString &str, QString *lang = nullptr)
{
    for (const QMPlay2Tag &tag : other_info)
    {
        if (!tag.second.isEmpty())
        {
            QString value = tag.second;
            if (tag.first.toInt() == QMPLAY2_TAG_LANGUAGE)
            {
                value = QMPlay2Core.getLanguagesMap().value(value, tag.second).toLower();
                if (lang)
                    *lang = value;
            }
            str += "<li><b>" + StreamInfo::getTagName(tag.first).toLower() + ":</b> " + value + "</li>";
        }
    }
}
void DemuxerThr::addSubtitleStream(bool currentPlaying, QString &subtitlesStreams, int i, int subtitlesStreamCount, const QString &streamName, const QString &codecName, const QString &title, QStringList &streamsMenu, const QVector<QMPlay2Tag> &other_info)
{
    const QString streamLink = streamName + QString::number(i);
    const QString streamCountStr = QString::number(subtitlesStreamCount);
    QString lang;
    subtitlesStreams += "<ul style='margin-top: 0px; margin-bottom: 0px;'>";
    if (currentPlaying)
        subtitlesStreams += "<u>";
    else
        subtitlesStreams += "<a href='stream:" + streamLink + "'>";
    subtitlesStreams += "<li><b>" + tr("Stream") + " " + streamCountStr + "</b></li>";
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
    printOtherInfo(other_info, subtitlesStreams, &lang);
    subtitlesStreams += "</ul></ul>";

    QString streamMenuText;
    if (!lang.isEmpty())
        streamMenuText.push_back(" (" + lang + ")");
    if (!title.isEmpty())
        streamMenuText.push_back(" - " + title);
    addStreamsMenuString(streamsMenu, streamCountStr, streamLink, currentPlaying, streamMenuText);
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
            QString txt;
            if (StreamInfo::getTag(tag.first) != QMPLAY2_TAG_LYRICS)
            {
                txt = tag.second;
                txt.replace("<", "&#60;"); //Don't recognize as HTML tag
            }
            else
            {
                txt.clear();
            }
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
                info += "<b>" + StreamInfo::getTagName(tag.first);
                if (!txt.isEmpty())
                    info += ":</b> " + txt;
                else
                    info += "</b>";
                info += "<br/>";
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
        info += "<b>" + tr("File path") + ": </b> " + QDir::toNativeSeparators(Functions::filePath(pth)) + "<br/>";
        info += "<b>" + tr("File name") + ": </b> " + Functions::fileName(pth) + "<br/>";
    }

    if (demuxer->bitrate() > 0)
        info += "<b>" + tr("Bitrate") + ":</b> " + QString::number(demuxer->bitrate()) + "kbps<br/>";
    info += "<b>" + tr("Format") + ":</b> " + demuxer->name();

    if (!demuxer->image().isNull())
        info += "<br/><br/><a href='save_cover'>" + tr("Save cover picture") + "</a>";

    QStringList chaptersMenu, programsMenu;

    bool once = false;
    for (const ProgramInfo &program : demuxer->getPrograms())
    {
        const QString bitrateStr = Functions::getBitrateStr(program.bitrate);

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
            const AVMediaType type = program.streams.at(i).second;
            streams += QString::number(type) + ":" + QString::number(stream) + ",";
            if (stream == playC.videoStream || stream == playC.audioStream || stream == playC.subtitlesStream)
                ++currentPlaying;
        }
        streams.chop(1);

        const bool isCurrent = (currentPlaying == currentPlayingWanted);
        const auto programNumberStr = QString::number(program.number);

        info += "<ul style='margin-top: 0px; margin-bottom: 0px;'>";
        info += "<li>";
        if (!streams.isEmpty())
            info += "<a style='text-decoration: underline;' href='stream:" + streams + "'>";
        if (isCurrent)
            info += "<b>";
        info += tr("Program") + " " + programNumberStr;
        if (isCurrent)
            info += "</b>";
        if (!streams.isEmpty())
            info += "</a>";
        if (program.bitrate > 0)
            info += " (" + bitrateStr + ")";
        info += "</li></ul>";

        addStreamsMenuString(programsMenu, programNumberStr, streams, isCurrent, QString());
    }

    int chapterCount = 0;
    once = false;
    for (const ChapterInfo &chapter : demuxer->getChapters())
    {
        const QString title(chapter.title.isEmpty() ? tr("Chapter") + " " + QString::number(++chapterCount) : chapter.title);
        const QString seekTo = QString::number(chapter.start);
        if (!once)
        {
            info += "<p style='margin-bottom: 0px;'><b><big>" + tr("Chapters") + ":</big></b></p>";
            once = true;
        }
        info += "<ul style='margin-top: 0px; margin-bottom: 0px;'>";
        info += "<li><a style='text-decoration: underline;' href='seek:" + seekTo + "'>" + title + "</a></li>";
        info += "</ul>";
        chaptersMenu.push_back(title + "\nseek" + seekTo);
    }

    bool videoPlaying = false, audioPlaying = false;

    const QList<StreamInfo *> streamsInfo = demuxer->streamsInfo();
    QString videoStreams, audioStreams, subtitlesStreams, attachmentStreams;
    QStringList videoStreamsMenu, audioStreamsMenu, subtitlesStreamsMenu;
    int videoStreamCount = 0, audioStreamCount = 0, subtitlesStreamCount = 0, i = 0;
    for (StreamInfo *streamInfo : streamsInfo)
    {
        auto appendCodecBitRate = [](QString &txt, const QByteArray &codec, const QString &bitRate) {
            if (!codec.isEmpty() || !bitRate.isEmpty())
            {
                txt.push_back(" (");
                if (!codec.isEmpty())
                {
                    txt.push_back(codec);
                    if (!bitRate.isEmpty())
                        txt.push_back(", ");
                }
                if (!bitRate.isEmpty())
                {
                    txt.push_back(bitRate);
                }
                txt.push_back(")");
            }
        };
        switch (streamInfo->params->codec_type)
        {
            case AVMEDIA_TYPE_VIDEO:
            {
                const QString streamLink = "video" + QString::number(i);
                const QString streamCountStr = QString::number(++videoStreamCount);
                const QString sizeStr = QString::number(streamInfo->params->width) + "x" + QString::number(streamInfo->params->height);
                const QString bitrateStr = Functions::getBitrateStr(streamInfo->params->bit_rate);
                const bool currentPlaying = getCurrentPlaying(playC.videoStream, streamsInfo, streamInfo);
                videoStreams += "<ul style='margin-top: 0px; margin-bottom: 0px;'><li>";
                if (currentPlaying)
                {
                    videoPlaying = true;
                    videoStreams += "<u>";
                }
                else
                    videoStreams += "<a href='stream:" + streamLink + "'>";
                videoStreams += "<b>" + tr("Stream") + " " + streamCountStr + "</b>";
                if (currentPlaying)
                    videoStreams += "</u>" + getWriterName((AVThread *)playC.vThr);
                else
                    videoStreams += "</a>";
                videoStreams += "</li><ul>";
                if (!streamInfo->title.isEmpty())
                    videoStreams += "<li><b>" + tr("title") + ":</b> " + streamInfo->title + "</li>";
                if (!streamInfo->codec_name.isEmpty())
                    videoStreams += "<li><b>" + tr("codec") + ":</b> " + streamInfo->codec_name + "</li>";
                videoStreams += "<li><b>" + tr("size") + ":</b> " + sizeStr + "</li>";
                videoStreams += "<li><b>" + tr("aspect ratio") + ":</b> " + QString::number(streamInfo->getAspectRatio()) + "</li>";
                const double FPS = streamInfo->getFPS();
                if (FPS)
                    videoStreams += "<li><b>" + tr("FPS") + ":</b> " + QString::number(FPS) + "</li>";
                if (!bitrateStr.isEmpty())
                    videoStreams += "<li><b>" + tr("bitrate") + ":</b> " + bitrateStr + "</li>";
                const auto formatName = streamInfo->getFormatName();
                if (!formatName.isEmpty())
                {
                    const QString colorRangeName = streamInfo->getColorRangeName();
                    QString colorSpaceName = streamInfo->getColorSpaceName();
                    QString colorPrimariesName = streamInfo->getColorPrimariesName();
                    QString colorTrcName = streamInfo->getColorTrcName();
                    QString additionalFormatInfo;
                    if (!colorSpaceName.isEmpty() || !colorPrimariesName.isEmpty() || !colorTrcName.isEmpty())
                    {
                        if (colorSpaceName.isEmpty())
                            colorSpaceName = tr("unknown");
                        if (colorPrimariesName.isEmpty() && !colorTrcName.isEmpty())
                            colorPrimariesName = tr("unknown");
                        if (colorTrcName.isEmpty() && !colorPrimariesName.isEmpty())
                            colorTrcName = tr("unknown");
                        additionalFormatInfo = " (";
                        if (!colorRangeName.isEmpty())
                            additionalFormatInfo += colorRangeName + ", ";
                        additionalFormatInfo += colorSpaceName;
                        if (!colorPrimariesName.isEmpty())
                        {
                            additionalFormatInfo += "/" + colorPrimariesName;
                            if (!colorTrcName.isEmpty())
                            {
                                additionalFormatInfo += "/" + colorTrcName;
                            }
                        }
                        additionalFormatInfo += ")";
                    }
                    else if (!colorRangeName.isEmpty())
                    {
                        additionalFormatInfo = " (" + colorRangeName + ")";
                    }

                    videoStreams += "<li><b>" + tr("format") + ":</b> " + formatName + additionalFormatInfo + "</li>";
                }
                printOtherInfo(streamInfo->other_info, videoStreams);
                videoStreams += "</ul></ul>";

                QString streamMenuText = " - " + sizeStr;
                appendCodecBitRate(streamMenuText, streamInfo->codec_name, bitrateStr);
                addStreamsMenuString(videoStreamsMenu, streamCountStr, streamLink, currentPlaying, streamMenuText);
            } break;
            case AVMEDIA_TYPE_AUDIO:
            {
                const QString streamLink = "audio" + QString::number(i);
                const QString streamCountStr = QString::number(++audioStreamCount);
                const QString bitrateStr = Functions::getBitrateStr(streamInfo->params->bit_rate);
                QString lang;
                const bool currentPlaying = getCurrentPlaying(playC.audioStream, streamsInfo, streamInfo);
                audioStreams += "<ul style='margin-top: 0px; margin-bottom: 0px;'><li>";
                if (currentPlaying)
                {
                    audioPlaying = true;
                    audioStreams += "<u>";
                }
                else
                    audioStreams += "<a href='stream:" + streamLink + "'>";
                audioStreams += "<b>" + tr("Stream") + " " + streamCountStr + "</b>";
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
                audioStreams += "<li><b>" + tr("sample rate") + ":</b> " + QString::number(streamInfo->params->sample_rate) + " Hz</li>";

                QString channels;
                if (streamInfo->params->CODECPAR_NB_CHANNELS == 1)
                    channels = tr("mono");
                else if (streamInfo->params->CODECPAR_NB_CHANNELS == 2)
                    channels = tr("stereo");
                else
                    channels = QString::number(streamInfo->params->CODECPAR_NB_CHANNELS);
                audioStreams += "<li><b>" + tr("channels") + ":</b> " + channels + "</li>";

                if (!bitrateStr.isEmpty())
                    audioStreams += "<li><b>" + tr("bitrate") + ":</b> " + bitrateStr + "</li>";
                const auto formatName = streamInfo->getFormatName();
                if (!formatName.isEmpty())
                    audioStreams += "<li><b>" + tr("format") + ":</b> " + formatName + "</li>";
                printOtherInfo(streamInfo->other_info, audioStreams, &lang);
                audioStreams += "</ul></ul>";

                QString streamMenuText;
                if (!lang.isEmpty())
                    streamMenuText.push_back(" - " + lang);
                appendCodecBitRate(streamMenuText, streamInfo->codec_name, bitrateStr);
                addStreamsMenuString(audioStreamsMenu, streamCountStr, streamLink, currentPlaying, streamMenuText);
            } break;
            case AVMEDIA_TYPE_SUBTITLE:
                addSubtitleStream(getCurrentPlaying(playC.subtitlesStream, streamsInfo, streamInfo), subtitlesStreams, i, ++subtitlesStreamCount, "subtitles", streamInfo->codec_name, streamInfo->title, subtitlesStreamsMenu, streamInfo->other_info);
                break;
            case AVMEDIA_TYPE_ATTACHMENT:
            {
                attachmentStreams += "<ul style='margin-top: 0px; margin-bottom: 0px;'>";
                attachmentStreams += "<li><b>" + streamInfo->title + "</b> - " + Functions::sizeString(streamInfo->params->extradata_size) + "</li>";
                attachmentStreams += "</ul>";
            } break;
            default:
                break;
        }
        ++i;
    }
    i = 0;
    for (const QString &fName : std::as_const(playC.fileSubsList))
        addSubtitleStream(fName == playC.fileSubs, subtitlesStreams, i++, ++subtitlesStreamCount, "fileSubs", QString(), Functions::fileName(fName), subtitlesStreamsMenu);

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
    emit playC.setStreamsMenu(videoStreamsMenu, audioStreamsMenu, subtitlesStreamsMenu, chaptersMenu, programsMenu);
}

bool DemuxerThr::mustReloadStreams()
{
    if
    (
        playC.reload ||
        (playC.chosenAudioStream     > -1 && playC.chosenAudioStream     != playC.audioStream    ) ||
        (playC.chosenVideoStream     > -1 && playC.chosenVideoStream     != playC.videoStream    ) ||
        (playC.chosenSubtitlesStream > -1 && playC.chosenSubtitlesStream != playC.subtitlesStream)
    )
    {
        if (playC.frame_last_delay <= 0.0 && playC.videoStream > -1)
            playC.frame_last_delay = getFrameDelay();
        playC.reload = true;
        return true;
    }
    return false;
}
template<typename T> bool DemuxerThr::bufferedAllPackets(T vS, T aS, T p)
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
double DemuxerThr::getAVBuffersSize(int &vS, int &aS, double &vT, double &aT)
{
    double remainingDuration = 0.0;

    playC.vPackets.lock();
    playC.aPackets.lock();

    if (playC.vPackets.isEmpty())
    {
        vS = 0;
        vT = 0.0;
    }
    else
    {
        vS = playC.vPackets.remainingPacketsCount();
        vT = playC.vPackets.remainingDuration();
        remainingDuration = vT;
    }

    if (playC.aPackets.isEmpty())
    {
        aS = 0;
        aT = 0.0;
    }
    else
    {
        aS = playC.aPackets.remainingPacketsCount();
        aT = playC.aPackets.remainingDuration();
        remainingDuration = remainingDuration > 0.0 ? qMin(remainingDuration, aT) : aT;
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
    return 1.0 / demuxer->streamsInfo().at(playC.videoStream)->getFPS();
}

void DemuxerThr::changeStatusText()
{
    if (paused)
    {
        emit playC.chText(tr("Paused"));
    }
    else
    {
        emit playC.chText(m_recording ? tr("Playback and recording") : tr("Playback"));
    }
}

void DemuxerThr::stopVADec()
{
    clearBuffers();

    playC.stopVDec();
    playC.stopADec();

    const QString key = playC.getUrl().toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    const double pos = playC.getPos();
    bool store = !err && !unknownLength && demuxer && QMPlay2Core.getSettings().getBool("StoreUrlPos");
    if (store)
    {
        const double length = demuxer->length();
        const double percent = pos * 100.0 / length;
        if (length < 480.0 || percent < 1.0 || percent > 99.0)
        {
            if (!playC.dontResetContinuePlayback)
                QMPlay2Core.getUrlPosSets().remove(key);
        }
        else
        {
            QMPlay2Core.getUrlPosSets().set(key, pos);
        }
    }
    else
    {
        QMPlay2Core.getUrlPosSets().remove(key);
    }

    stopVAMutex.unlock();

    endMutex.lock(); //Czeka do czasu zniszczenia demuxer'a - jeżeli wcześniej mutex był zablokowany (wykonał się z wątku)
    endMutex.unlock(); //odblokowywuje mutex
}
void DemuxerThr::updateCover(const QString &title, const QString &artist, const QString &album, const QByteArray &cover)
{
    const QImage coverImg = QImage::fromData(cover);
    if (!coverImg.isNull())
    {
        const bool bothInTitle =
               !title.isEmpty()
            && !artist.isEmpty()
            && this->artist.isEmpty()
            && this->title.contains(QStringLiteral(" - "))
            && this->title.contains(title)
            && this->title.contains(artist)
        ;
        if ((bothInTitle || (this->title == title && this->artist == artist)) && (this->album == album || (album.isEmpty() && !title.isEmpty())))
            emit playC.updateImage(coverImg);

        static bool useCoversCache = !QMPlay2Core.getSettings().getBool("NoCoversCache");
        if (!useCoversCache)
            return;

        QDir dir(QMPlay2Core.getSettingsDir());
        dir.mkdir("Covers");
        QFile f(bothInTitle ? getCoverFile(this->title, QString(), album) : getCoverFile(title, artist, album));
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
