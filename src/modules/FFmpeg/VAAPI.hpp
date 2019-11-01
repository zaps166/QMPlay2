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

#include <QCoreApplication>
#include <QVector>
#include <QHash>

#include <va/va.h>
#include <va/va_vpp.h>

class VideoFrame;
class ImgScaler;
struct AVFrame;

class VAAPI
{
    Q_DECLARE_TR_FUNCTIONS(VAAPI)

public:
    VAAPI();
    ~VAAPI();

    bool open(const char *codecName, bool &openGL);

    void init(int width, int height, bool allowFilters);

    bool vaapiCreateSurface(VASurfaceID &surface, int w, int h);

    void maybeInitVPP(int surfaceW, int surfaceH);
    void clearVPP(bool resetAllowFilters = true);

    void applyVideoAdjustment(int brightness, int contrast, int saturation, int hue);

    bool filterVideo(const VideoFrame &frame, VASurfaceID &id, int &field);

    quint8 *getNV12Image(VAImage &image, VASurfaceID surfaceID) const;
    bool getImage(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) const;

    void insertFrame(VASurfaceID id, AVFrame *frame);

private:
    bool hasProfile(const char *codecName) const;

    void clearVPPFrames();

public:
    bool ok = false;

    int m_fd = -1;
    VADisplay VADisp = nullptr;

    VAImageFormat nv12ImageFmt = {};

    int outW = 0, outH = 0;

    // Postprocessing
    VAProcDeinterlacingType vpp_deint_type = VAProcDeinterlacingNone;
    bool use_vpp = false;

private:
    int m_version = 0;

    // Postprocessing
    bool m_allowFilters = false;
    VAContextID context_vpp;
    VAConfigID config_vpp;
    VABufferID m_vppDeintBuff;
    VASurfaceID id_vpp;
    QVector<VASurfaceID> m_refs;
    VASurfaceID m_lastVppSurface;
    int m_nBackwardRefs, m_nForwardRefs;
    bool m_hasVppFrame = false;

    QHash<VASurfaceID, VideoFrame> m_vppFrames;
};
