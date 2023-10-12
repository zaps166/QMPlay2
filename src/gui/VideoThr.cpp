/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2023  Błażej Szczygieł

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
#include <ScreenSaver.hpp>
#include <PlayClass.hpp>
#include <Main.hpp>

#include <GPUInstance.hpp>
#include <HWDecContext.hpp>
#include <VideoWriter.hpp>
#include <Frame.hpp>
#include <Settings.hpp>
#include <Decoder.hpp>
#include <LibASS.hpp>
#include <ImgScaler.hpp>
#include <Functions.hpp>

#ifdef USE_OPENGL
#   include <opengl/OpenGLHWInterop.hpp>
#endif
#ifdef USE_VULKAN
#   include <vulkan/VulkanInstance.hpp>
#   include <vulkan/VulkanHWInterop.hpp>
#   include <vulkan/VulkanYadifDeint.hpp>
#endif

extern "C" {
    #include <libavutil/cpu.h>
    #include <libavutil/mem.h>
}

using Functions::gettime;
using namespace std;

#include <QDebug>
#include <QImage>
#include <QDir>

#ifdef Q_OS_WIN
#   include <windows.h>
#endif

#include <cmath>

VideoThr::VideoThr(PlayClass &playC, const QStringList &pluginsName) :
    AVThread(playC),
    syncVtoA(QMPlay2Core.getSettings().getBool("SyncVtoA")),
    doScreenshot(false),
    deleteOSD(false), deleteFrame(false), gotFrameOrError(false), decoderError(false),
    W(0), H(0), seq(0),
    sDec(nullptr)
{
    if (QMPlay2Core.renderer() != QMPlay2CoreClass::Renderer::Legacy)
    {
        writer = QMPlay2Core.gpuInstance()->createOrGetVideoOutput();
        videoWriter()->open();
    }
    else
    {
        writer = Writer::create("video:", pluginsName);
    }

    maybeStartThread();
}
VideoThr::~VideoThr()
{
    QMPlay2GUI.videoAdjustment->enableControls();
    QMPlay2GUI.screenSaver->unInhibit(0);
#ifdef Q_OS_WIN
    setHighTimerResolution<false>();
#endif
    playC.osd.reset();
    delete sDec;
}

void VideoThr::setDec(Decoder *dec)
{
    AVThread::setDec(dec);
    if (!dec->hasHWDecContext() && videoWriter()->hwDecContext())
        videoWriter()->setHWDecContext(nullptr);
    decoderError = false;
}

shared_ptr<HWDecContext> VideoThr::getHWDecContext() const
{
    return videoWriter()->hwDecContext();
}

bool VideoThr::videoWriterSet()
{
    if (QMPlay2Core.renderer() == QMPlay2CoreClass::Renderer::Legacy)
        return false; // Force restart to use possible new order of video outputs
    return writer->set();
}

bool VideoThr::lock()
{
    if (QThread::currentThread() == thread())
    {
        m_subsDisplayLocker = {};
    }
    return AVThread::lock();
}

void VideoThr::stop(bool terminate)
{
    if (QMPlay2Core.renderer() != QMPlay2CoreClass::Renderer::Legacy)
        QMPlay2Core.gpuInstance()->clearVideoOutput();
    playC.videoSeekPos = -1;
    AVThread::stop(terminate);
}

bool VideoThr::hasDecoderError() const
{
    return decoderError;
}

AVPixelFormats VideoThr::getSupportedPixelFormats() const
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
    m_decodeToAss = false;
}
void VideoThr::setSubtitlesDecoder(Decoder *dec, bool decodeToAss)
{
    sDec = dec;
    m_decodeToAss = decodeToAss;
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
void VideoThr::setARatio(double aRatio, const AVRational &sar)
{
    updateSubs();
    writer->modParam("AspectRatio", aRatio);
    lastSAR = sar;
}
void VideoThr::setZoom()
{
    updateSubs();
    writer->modParam("Zoom", playC.zoom);
}

void VideoThr::setColorInfo(AVColorPrimaries colorPrimaries, AVColorTransferCharacteristic colorTrc)
{
    if (writer->hasParam("ColorPrimaries"))
        writer->modParam("ColorPrimaries", colorPrimaries);
    if (writer->hasParam("ColorTrc"))
        writer->modParam("ColorTrc", colorTrc);
}
void VideoThr::otherReset()
{
    if (writer->hasParam("ResetOther"))
        writer->modParam("ResetOther", true);
}

void VideoThr::initFilters()
{
    Settings &QMPSettings = QMPlay2Core.getSettings();

    filtersMutex.lock();
    filters.clear();

    if (!playC.stillImage)
    {
        const bool deint = QMPSettings.getBool("Deinterlace/ON");
        const bool autoDeint = QMPSettings.getBool("Deinterlace/Auto");
        const bool doubleFramerate = QMPSettings.getBool("Deinterlace/Doubler");
        const bool autoParity = QMPSettings.getBool("Deinterlace/AutoParity");
        const bool topFieldFirst = QMPSettings.getBool("Deinterlace/TFF");
        const quint8 deintFlags = autoDeint | doubleFramerate << 1 | autoParity << 2 | topFieldFirst << 3;

#ifdef USE_VULKAN
        auto enableVulkanDeint = [&] {
            if (!QMPlay2Core.isVulkanRenderer())
                return false;

            if (!static_pointer_cast<QmVk::Instance>(QMPlay2Core.gpuInstance())->checkFiltersSupported())
                return false;

            shared_ptr<VideoFilter> deintFilter = make_shared<QmVk::YadifDeint>(
                static_pointer_cast<QmVk::HWInterop>(getHWDecContext())
            );
            if (deintFilter->modParam("DeinterlaceFlags", deintFlags))
            {
                deintFilter->modParam("W", W);
                deintFilter->modParam("H", H);
                if (deintFilter->processParams())
                {
                    filters.on(deintFilter);
                }
            }

            return true;
        };
#endif

        auto iterateVideoFilters = [this, &QMPSettings](bool isHw) {
            for (QString filterName : QMPSettings.getStringList("VideoFilters"))
            {
                if (filterName.left(1).toInt()) //if filter is enabled
                {
                    bool ok = false;
                    filterName = filterName.mid(1);
                    if (auto filter = filters.on(filterName, isHw))
                    {
                        filter->modParam("W", W);
                        filter->modParam("H", H);
                        if (!(ok = filter->processParams()))
                            filters.off(filter);
                    }
                    if (!ok && W > 0 && H > 0)
                        QMPlay2Core.logError("Error initializing filter" + " \"" + filterName + '"');
                }
            }
        };

        if (getHWDecContext())
        {
            if (auto hwFilter = dec->hwAccelFilter())
            {
                hwFilter->modParam("Deinterlace", deint);
                hwFilter->modParam("DeinterlaceFlags", deintFlags);
                hwFilter->modParam("W", W);
                hwFilter->modParam("H", H);
                if (hwFilter->processParams())
                    filters.on(hwFilter);
            }
#ifdef USE_VULKAN
            else if (deint)
            {
                enableVulkanDeint();
            }

            iterateVideoFilters(true);
#endif
        }
        else
        {
            // Deinterlacing filter as first
            bool enableSoftwareDeint = deint;
#ifdef USE_VULKAN
            if (deint && QMPSettings.getBool("Vulkan/AlwaysGPUDeint"))
            {
                if (enableVulkanDeint())
                    enableSoftwareDeint = false;
            }
#endif
            if (enableSoftwareDeint)
            {
                const QString deintFilterName = QMPSettings.getString("Deinterlace/SoftwareMethod");
                if (auto deintFilter = filters.on(deintFilterName, false))
                {
                    bool ok = false;
                    if (deintFilter->modParam("DeinterlaceFlags", deintFlags))
                    {
                        deintFilter->modParam("W", W);
                        deintFilter->modParam("H", H);
                        ok = deintFilter->processParams();
                    }
                    if (!ok)
                    {
                        filters.off(deintFilter);
                        if (W > 0 && H > 0)
                            QMPlay2Core.logError("Cannot initialize the deinterlacing filter" + " \"" + deintFilterName + '"', true, true);
                    }
                }
            }

            iterateVideoFilters(false);
        }
    }

    filtersMutex.unlock();
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
        if (m_subtitlesBusy)
            playC.ass->getASS(m_subtitlesBusy);
        if (m_subtitles)
            playC.ass->getASS(m_subtitles);
        playC.subsMutex.unlock();
    }
}

inline VideoWriter *VideoThr::videoWriter() const
{
    return static_cast<VideoWriter *>(writer);
}

void VideoThr::run()
{
    bool skip = false, paused = false, oneFrame = false, useLastDelay = false, maybeFlush = false, lastAVDesync = false, interlaced = false, err = false, skipNonKey = false;
    double tmp_time = 0.0, sync_last_pts = 0.0, frame_timer = -1.0, sync_timer = 0.0, framesDisplayedTime = 0.0;
    QMutex emptyBufferMutex;
    Frame videoFrame;
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
            QTimer::singleShot(0, this, [=] {
                screenshot(videoFrame);
            });
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
                QTimer::singleShot(0, this, [this] {
                    pause();
                });
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
        double ts = qQNaN();
        if (hasVPackets && mustFetchNewPacket)
        {
            packet = playC.vPackets.fetch();
            if (packet.isTsValid())
                ts = packet.ts();
        }
        playC.vPackets.unlock();
        processOneFrame();
        playC.fillBufferB = true;

        /* Subtitles packet */
        QVector<Packet> sPackets;
        playC.sPackets.lock();
        while (playC.sPackets.canFetch())
        {
            auto packet = playC.sPackets.fetch();
            if (!packet.isEmpty())
                sPackets.push_back(move(packet));
        }
        playC.sPackets.unlock();

        mutex.lock();
        if (br)
        {
            mutex.unlock();
            break;
        }

        const bool flushVideo = playC.flushVideo;

        filtersMutex.lock();
        if (flushVideo || skipNonKey)
        {
            filters.clearBuffers();
            if (flushVideo)
                frame_timer = -1.0;
        }

        if ((!packet.isEmpty() || maybeFlush) && (!skipNonKey || packet.hasKeyFrame()))
        {
            Frame decoded;
            AVPixelFormat newPixelFormat = AV_PIX_FMT_NONE;
            const int bytes_consumed = dec->decodeVideo(packet, decoded, newPixelFormat, flushVideo || skipNonKey, (skip && !skipNonKey) ? ~0 : (fast >> 1));
            ts = decoded.isTsValid() ? decoded.ts() : qQNaN();
            if (newPixelFormat != AV_PIX_FMT_NONE)
                emit playC.pixelFormatUpdate(newPixelFormat);
            if (flushVideo)
            {
                useLastDelay = true; //if seeking
                playC.flushVideo = false;
            }
            if (playC.videoSeekPos > 0.0 && bytes_consumed <= 0 && !decoded.isTsValid() && decoded.isEmpty())
                finishAccurateSeek();
            if (!decoded.isEmpty())
            {
                if (decoded.width() != W || decoded.height() != H)
                {
                    //Frame size has been changed
                    filtersMutex.unlock();
                    updateMutex.lock();
                    mutex.unlock();
                    emit playC.frameSizeUpdate(decoded.width(), decoded.height());
                    updateMutex.lock(); //Wait for "frameSizeUpdate()" to be finished
                    mutex.lock();
                    updateMutex.unlock();
                    filtersMutex.lock();
                }
                interlaced = decoded.isInterlaced();
                filters.addFrame(decoded);
                gotFrameOrError = true;
            }
            else if (skip)
            {
                filters.removeLastFromInputBuffer();
            }
            if (bytes_consumed < 0)
            {
                gotFrameOrError = true;
                err = true;
            }
            else
            {
                tmp_br += bytes_consumed;
            }
            skipNonKey = false;
        }

        // This thread will wait for "DemuxerThr" which'll detect this error and restart with new decoder.
        if (dec->hasCriticalError())
        {
            decoderError = true;
        }
        else if (auto hwDecContext = getHWDecContext())
        {
            decoderError = hwDecContext->hasError();
        }

        const bool ptsIsValid = filters.getFrame(videoFrame);
        if (ptsIsValid)
            ts = videoFrame.ts();
        filtersMutex.unlock();

        const bool tsIsNotNan = !qIsNaN(ts);

        /* Subtitles */
        QMPlay2OSDList osdList;
        m_subsDisplayMutex.lock(); // Must be locked before "playC.subsMutex"!
        playC.subsMutex.lock();
        const double subsPts = tsIsNotNan
            ? ts - playC.subtitlesSync
            : qQNaN()
        ;
        const bool canDeleteSubs = (deleteSubs && m_subtitles);
        auto resetSubs = [this] {
            m_subtitles.reset();
            m_subtitlesBusy.reset();
        };
        if (sDec && !m_decodeToAss) //Image subs (pgssub, dvdsub, ...)
        {
            if (!sDec->decodeSubtitle(sPackets, subsPts, m_subtitles, QSize(W, H), flushVideo))
            {
                resetSubs();
            }
        }
        else if (playC.ass)
        {
            for (auto &&sPacket : qAsConst(sPackets))
            {
                if (sDec && m_decodeToAss)
                {
                    const auto events = sDec->decodeSubtitle(sPacket);
                    playC.ass->addASSEvents(events, sPacket.ts(), sPacket.duration());
                }
                else
                {
                    const auto sPacketData = QByteArray::fromRawData(reinterpret_cast<const char *>(sPacket.data()), sPacket.size());
                    if (playC.ass->isASS())
                        playC.ass->addASSEvent(sPacketData);
                    else
                        playC.ass->addASSEvent(Functions::convertToASS(sPacketData), sPacket.ts(), sPacket.duration());
                }
            }
            if (tsIsNotNan && !playC.ass->getASS(m_subtitles, subsPts))
            {
                resetSubs();
            }
        }
        if (m_subtitles && tsIsNotNan)
        {
            const bool hasDuration = m_subtitles->duration() >= 0.0;
            if (canDeleteSubs || (m_subtitles->isStarted() && subsPts < m_subtitles->pts()) || (hasDuration && subsPts > m_subtitles->pts() + m_subtitles->duration()))
            {
                resetSubs();
            }
            else if (subsPts >= m_subtitles->pts())
            {
                m_subtitles->start();
                osdList += m_subtitles;
            }
        }
        playC.subsMutex.unlock();
        m_subsDisplayMutex.unlock();
        playC.osdMutex.lock();
        if (playC.osd)
        {
            if (deleteOSD || playC.osd->leftDuration() < 0)
            {
                playC.osd.reset();
            }
            else
            {
                osdList += playC.osd;
            }
        }
        playC.osdMutex.unlock();
        deleteSubs = deleteOSD = false;
        /**/

        if ((maybeFlush = tsIsNotNan))
        {
            const AVRational currSAR = videoFrame.sampleAspectRatio();
            if (currSAR.num != 0 && currSAR.den != 0 && lastSAR.num != 0 && lastSAR.den != 0 && av_cmp_q(lastSAR, currSAR) != 0) //Aspect ratio has been changed
            {
                lastSAR = {0, 0}; //Needs to be updated later
                emit playC.aRatioUpdate(currSAR); //Sets "lastSAR", because it calls "setARatio()";
            }

            if (playC.videoSeekPos > 0.0)
            {
                bool cont = true;
                if (ts >= playC.videoSeekPos)
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

            if (ptsIsValid || ts > playC.pos)
                playC.chPos(ts);

            double delay = ts - playC.frame_last_pts;
            if (useLastDelay || delay <= 0.0 || (playC.frame_last_pts <= 0.0 && delay > playC.frame_last_delay) || (delay > 1.0 && delay / playC.frame_last_delay > 10.0))
            {
                delay = playC.frame_last_delay;
                useLastDelay = false;
            }

            tmp_time += delay;
            ++frames;

            playC.frame_last_delay = delay;
            playC.frame_last_pts = ts;

            delay /= playC.speed;

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

                const double diff = ts - (delay + sync_pts - playC.videoSync);
                const double sync_threshold = qMax(delay, playC.audio_last_delay);
                const double max_threshold = qMax(sync_threshold * 2.0, 0.15);
                const double fDiff = qAbs(diff);

                if (fast && !skip && diff > -sync_threshold / 2.0)
                    fast = 0;
                skip = false;

//                qDebug() << "diff:" << diff << "sync_threshold:" << sync_threshold << "max_threshold:" << max_threshold;
                if (fDiff > sync_threshold && fDiff < max_threshold)
                {
                    if (fDiff > sync_threshold * 1.75)
                    {
                        delay += diff / 4.0;
                        lastAVDesync = true;
//                        qDebug() << "Syncing 2" << diff << delay << sync_threshold;
                    }
                    else
                    {
                        if (lastAVDesync)
                        {
                            delay += diff / 8.0;
//                            qDebug() << "Syncing 1" << diff << delay << sync_threshold;
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
                        if (fast >= 56 || (fast >= 28 && fDiff >= max_threshold * 4.0))
                            skipNonKey = true;
                    }
                    else if (diff > 0.0) //obraz idzie za szybko
                    {
                        if (diff <= 0.5)
                            delay *= 2.0;
                        else if (!playC.skipAudioFrame)
                            playC.skipAudioFrame = diff;
                    }
                    lastAVDesync = true;
//                    qDebug() << "Skipping" << diff << skip << fast << delay;
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
                    if (!osdList.isEmpty())
                        m_subsDisplayLocker = unique_lock<std::mutex>(m_subsDisplayMutex);
                    QTimer::singleShot(0, this, [=, osdList = move(osdList)]() mutable {
                        write(videoFrame, move(osdList), seq);
                        m_subsDisplayLocker = {};
                    });
                    if (canSkipFrames && !skipNonKey)
                        ++framesDisplayed;
                }
                if (canSkipFrames)
                    framesDisplayedTime += true_delay;
            }
            if (updateFrameTimer)
                frame_timer = gettime();
        }

        mutex.unlock();

        if (br2)
        {
            // Take time for another thread to lock the mutex
            Functions::s_wait(0.001);
        }
    }
}

#ifdef Q_OS_WIN
template<bool h>
inline void VideoThr::setHighTimerResolution()
{
    if (h)
    {
        if (!m_timerPrecision)
        {
            timeBeginPeriod(1);
            m_timerPrecision = true;
        }
    }
    else
    {
        if (m_timerPrecision)
        {
            timeEndPeriod(1);
            m_timerPrecision = false;
        }
    }
}
#endif

void VideoThr::write(const Frame &videoFrame, QMPlay2OSDList &&osdList, quint32 lastSeq)
{
    canWrite = true;

    if (lastSeq != seq || !writer->readyWrite())
        return;

    auto hwDecContext = getHWDecContext();
    if (hwDecContext && hwDecContext->hasError())
        return;

    QMPlay2GUI.screenSaver->inhibit(0);
#ifdef Q_OS_WIN
    setHighTimerResolution<true>();
#endif

    videoWriter()->writeVideo(videoFrame, move(osdList));

    if (m_subsDisplayLocker.owns_lock())
        swap(m_subtitles, m_subtitlesBusy);
}
void VideoThr::screenshot(Frame videoFrame)
{
    ImgScaler imgScaler;
    QImage img;

#ifdef USE_OPENGL
    if (auto hwGLInterop = dynamic_pointer_cast<OpenGLHWInterop>(getHWDecContext()))
    {
        videoFrame = hwGLInterop->getCpuFrame(videoFrame);
    }
#endif

    if (imgScaler.create(videoFrame, W, H))
    {
#ifdef USE_VULKAN
        if (auto vkHwInterop = dynamic_pointer_cast<QmVk::HWInterop>(getHWDecContext()))
        {
            vkHwInterop->map(videoFrame);
            vkHwInterop->sync({videoFrame});
            if (vkHwInterop->hasError())
                videoFrame.clear();
        }
#endif
        auto imgData = reinterpret_cast<uint8_t *>(av_malloc(W * H * 4 + av_cpu_max_align()));
        img = QImage(imgData, W, H, QImage::Format_RGB32, [](void *ptr) {
            av_free(ptr);
        }, imgData);
        if (!imgScaler.scale(videoFrame, imgData))
            img = QImage();
    }

    if (img.isNull())
    {
        QMPlay2Core.logError("Cannot create screenshot");
        return;
    }

    const QString ext = QMPlay2Core.getSettings().getString("screenshotFormat");
    const QString dir = QMPlay2Core.getSettings().getString("screenshotPth");
    QString realUrl = playC.getUrl();
    Functions::splitPrefixAndUrlIfHasPluginPrefix(realUrl, nullptr, &realUrl);
    QString screenshotName;
    if (realUrl.startsWith("file://"))
    {
        screenshotName = Functions::fileName(realUrl) + "_QMPlay2_snapshot_" + Functions::timeToStr(videoFrame.ts(), false, true).replace(":", ".");
    }
    else
    {
        quint16 num = 0;
        for (const QString &f : QDir(dir).entryList({"QMPlay2_snapshot_?????" + ext}, QDir::Files, QDir::Name))
        {
            const quint16 n = QStringView(f).mid(13, 5)MAYBE_TO_STRING.toUShort();
            if (n > num)
                num = n;
        }
        screenshotName = "QMPlay2_snap_" + QString("%1").arg(++num, 5, 10, QChar('0'));
    }
    screenshotName += ext;
    if (img.save(dir + "/" + screenshotName))
        playC.messageAndOSD(tr("Screenshot saved as: %1").arg(screenshotName));
}
void VideoThr::pause()
{
    QMPlay2GUI.screenSaver->unInhibit(0);
#ifdef Q_OS_WIN
    setHighTimerResolution<false>();
#endif
    writer->pause();
}
