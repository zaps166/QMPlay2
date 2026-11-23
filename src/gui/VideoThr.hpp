// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <GUIHeaders.hpp>

#include <AVThread.hpp>
#include <VideoFilters.hpp>
#include <QMPlay2OSD.hpp>

class VideoWriter;
class HWDecContext;

class VideoThr final : public AVThread
{
    Q_OBJECT

public:
    VideoThr(PlayClass &playC, const QStringList &pluginsName = {});
    ~VideoThr();

    void setDec(Decoder *dec) override;

    std::shared_ptr<HWDecContext> getHWDecContext() const;

    bool videoWriterSet();

    bool lock() override;

    void stop(bool terminate = false) override;

    bool hasError() const override;
    bool hasDecoderError() const override;

    AVPixelFormats getSupportedPixelFormats() const;

    inline void setDoScreenshot()
    {
        doScreenshot = true;
    }
    inline void setDeleteOSD()
    {
        deleteOSD = true;
    }

    void setTsDiscontPossible(bool tsDiscontPossible);

    void destroySubtitlesDecoder();
    void setSubtitlesDecoder(Decoder *dec, bool decodeToAss);

    bool setSpherical();
    bool setFlip();
    bool setRotate90();
    void setVideoAdjustment();
    void setFrameSize(const QSize &size);
    void setARatio(double aRatio, const AVRational &sar);
    void setZoom(double zoom);
    void setSubtitlesScale(double subtitlesScale);
    void setColorInfo(AVColorPrimaries colorPrimaries, AVColorTransferCharacteristic colorTrc);
    void otherReset();

    void initFilters();

    bool processParams();

    void updateSubs();

private:
    inline VideoWriter *videoWriter() const;

    void run() override;

#ifdef Q_OS_WIN
    template<bool h>
    inline void setHighTimerResolution();
#endif

    void write(const Frame &videoFrame, QMPlay2OSDList &&osdList, quint32 lastSeq);
    void screenshot(Frame videoFrame);
    void pause();

private:
    bool deleteSubs, syncVtoA, doScreenshot, canWrite, deleteOSD, deleteFrame, gotFrameOrError, decoderError, m_error = false;
    bool m_tsDisontPossible = false;
    AVRational lastSAR;
    int W, H;
    quint32 seq;

    Decoder *sDec;
    bool m_decodeToAss = false;
    std::shared_ptr<QMPlay2OSD> m_subtitles, m_subtitlesBusy;
    std::mutex m_subsDisplayMutex;
    std::unique_lock<std::mutex> m_subsDisplayLocker;
    VideoFilters filters;
    QMutex filtersMutex;
    double m_subtitlesScale = 1.0;

#ifdef Q_OS_WIN
    bool m_timerPrecision = false;
#endif
};
