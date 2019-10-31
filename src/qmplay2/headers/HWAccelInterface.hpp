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

#include <VideoAdjustment.hpp>

#include <QString>

class VideoFrame;
class ImgScaler;

class HWAccelInterface
{
public:
    enum Format
    {
        NV12,
        RGB32
    };
    enum Field
    {
        FullFrame,
        TopField,
        BottomField
    };
    enum MapResult
    {
        MapNotReady = -1,
        MapOk,
        MapError,
    };

    virtual ~HWAccelInterface() = default;

    virtual QString name() const = 0;

    virtual Format getFormat() const = 0;
    virtual bool isTextureRectangle() const
    {
        return false;
    }
    virtual bool isCopy() const
    {
        return true;
    }

    virtual bool lock()
    {
        return true;
    }
    virtual void unlock()
    {}

    virtual bool canInitializeTextures() const
    {
        return true;
    }

    virtual bool init(quint32 *textures) = 0;
    virtual void clear(bool contextChange) = 0;

    virtual MapResult mapFrame(const VideoFrame &videoFrame, Field field) = 0;

    virtual bool getImage(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32 = nullptr) = 0;

    virtual void getVideAdjustmentCap(VideoAdjustment &videoAdjustmentCap)
    {
        videoAdjustmentCap.zero();
    }
    virtual void setVideoAdjustment(const VideoAdjustment &videoAdjustment)
    {
        Q_UNUSED(videoAdjustment)
    }
};
