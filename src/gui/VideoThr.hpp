/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

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
#include <PixelFormats.hpp>

class QMPlay2OSD;
class VideoWriter;

class VideoThr final : public AVThread
{
    Q_OBJECT
public:
    VideoThr(PlayClass &, VideoWriter *, const QStringList &pluginsName = {});
    ~VideoThr();

    void stop(bool terminate = false) override;

    bool hasDecoderError() const override;

    inline VideoWriter *getHWAccelWriter() const
    {
        return hwAccelWriter;
    }

    QMPlay2PixelFormats getSupportedPixelFormats() const;

    inline void setDoScreenshot()
    {
        doScreenshot = true;
    }
    inline void setSyncVtoA(bool b)
    {
        syncVtoA = b;
    }
    inline void setDeleteOSD()
    {
        deleteOSD = true;
    }

    void destroySubtitlesDecoder();
    inline void setSubtitlesDecoder(Decoder *dec)
    {
        sDec = dec;
    }

    bool setSpherical();
    bool setFlip();
    bool setRotate90();
    void setVideoAdjustment();
    void setFrameSize(int w, int h);
    void setARatio(double aRatio, double sar);
    void setZoom();
    void otherReset();

    void initFilters(bool processParams = true);

    bool processParams();

    void updateSubs();

private:
    inline VideoWriter *videoWriter() const;

    void run() override;

    bool deleteSubs, syncVtoA, doScreenshot, canWrite, deleteOSD, deleteFrame, gotFrameOrError, decoderError;
    double lastSampleAspectRatio;
    int W, H;
    quint32 seq;

    Decoder *sDec;
    VideoWriter *hwAccelWriter;
    QMPlay2OSD *subtitles;
    VideoFilters filters;
    QMutex filtersMutex;
private slots:
    void write(VideoFrame videoFrame, quint32 lastSeq);
    void screenshot(VideoFrame videoFrame);
    void pause();
};
