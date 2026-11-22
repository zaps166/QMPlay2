// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <HWDecContext.hpp>
#include <VideoAdjustment.hpp>

#include <functional>

class ImgScaler;

class QMPLAY2SHAREDLIB_EXPORT OpenGLHWInterop : public HWDecContext
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

    virtual Frame getCpuFrame(const Frame &frame) = 0;

    virtual void getVideAdjustmentCap(VideoAdjustment &videoAdjustmentCap)
    {
        videoAdjustmentCap.zero();
    }
    virtual void setVideoAdjustment(const VideoAdjustment &videoAdjustment)
    {
        Q_UNUSED(videoAdjustment)
    }
};
