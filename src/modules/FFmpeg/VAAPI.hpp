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
#include <QMutex>
#include <QSet>

#include <va/va.h>
#include <va/va_vpp.h>

class VideoFrame;
class ImgScaler;

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

    bool filterVideo(const VideoFrame &videoFrame, VASurfaceID &id, int &field);

    quint8 *getNV12Image(VAImage &image, VASurfaceID surfaceID) const;
    bool getImage(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) const;

    void clearSurfaces();
    void insertSurface(quintptr id);
    bool checkSurface(quintptr id);

private:
    bool hasProfile(const char *codecName) const;

public:
    bool ok = false;

    VADisplay VADisp = nullptr;

    VAImageFormat nv12ImageFmt = {};

    int outW = 0, outH = 0;

    // Postprocessing
    VAProcDeinterlacingType vpp_deint_type = VAProcDeinterlacingNone;
    bool use_vpp = false;

private:
    int version = 0;

    QSet<quintptr> m_surfaces;
    QMutex m_surfacesMutex;

    // Postprocessing
    bool m_allowFilters = false;
    VAContextID context_vpp;
    VAConfigID config_vpp;
    VABufferID vpp_buffers[VAProcFilterCount]; //TODO implement all filters
    VASurfaceID id_vpp, forward_reference;
    bool vpp_second;
};
