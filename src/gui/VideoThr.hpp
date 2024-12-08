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

#pragma once

#include <AVThread.hpp>
#include <VideoFilters.hpp>
#include <QMPlay2OSD.hpp>

extern "C" {
    #include <libavutil/rational.h>
}

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

    void destroySubtitlesDecoder();
    void setSubtitlesDecoder(Decoder *dec, bool decodeToAss);

    bool setSpherical();
    bool setFlip();
    bool setRotate90();
    void setVideoAdjustment();
    void setFrameSize(int w, int h);
    void setARatio(double aRatio, const AVRational &sar);
    void setZoom();
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
    bool deleteSubs, syncVtoA, doScreenshot, canWrite, deleteOSD, deleteFrame, gotFrameOrError, decoderError;
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
