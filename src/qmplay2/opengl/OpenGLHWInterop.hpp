/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <HWDecContext.hpp>
#include <VideoAdjustment.hpp>

#include <functional>

class ImgScaler;

class OpenGLHWInterop : public HWDecContext
{
public:
    enum Format
    {
        NV12,
        RGB32
    };

    using SetTextureParamsFn = std::function<void(quint32 texture)>;

public:
    virtual Format getFormat() const = 0;
    virtual bool isTextureRectangle() const
    {
        return false;
    }
    virtual bool isCopy() const
    {
        return true;
    }

    virtual bool init(const int *widths, const int *heights, const SetTextureParamsFn &setTextureParamsFn) = 0;
    virtual void clear() = 0;

    virtual bool mapFrame(Frame &videoFrame) = 0;
    virtual quint32 getTexture(int plane) = 0;

    virtual QImage getImage(const Frame &frame) = 0;

    virtual void getVideAdjustmentCap(VideoAdjustment &videoAdjustmentCap)
    {
        videoAdjustmentCap.zero();
    }
    virtual void setVideoAdjustment(const VideoAdjustment &videoAdjustment)
    {
        Q_UNUSED(videoAdjustment)
    }
};
