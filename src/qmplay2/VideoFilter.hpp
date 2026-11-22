// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <ModuleCommon.hpp>
#include <ModuleParams.hpp>
#include <Frame.hpp>

#include <QQueue>

#ifdef USE_VULKAN
#include <functional>

namespace QmVk {
class Device;
class Queue;
class CommandBuffer;
class Image;
class ImagePool;
class HWInterop;
}
#endif

class QMPLAY2SHAREDLIB_EXPORT VideoFilter : public ModuleCommon, public ModuleParams
{
#ifdef USE_VULKAN
protected:
    using CopyImageLinearToOptimalFn = std::function<void(const std::shared_ptr<QmVk::CommandBuffer> &)>;
#endif

public:
    enum DeintFlags
    {
        AutoDeinterlace = 0x1,
        DoubleFramerate = 0x2,
        AutoParity = 0x4,
        TopFieldFirst = 0x8
    };

public:
    VideoFilter(bool fillDefaultSupportedPixelFormats);
    ~VideoFilter();

    virtual void clearBuffer();

    bool removeLastFromInternalBuffer();

    virtual bool filter(QQueue<Frame> &framesQueue) = 0;

protected:
    void processParamsDeint();

    void addFramesToInternalQueue(QQueue<Frame> &framesQueue);
    void addFramesToDeinterlace(QQueue<Frame> &framesQueue);

    void deinterlaceDoublerCommon(Frame &frame);

    bool isTopFieldFirst(const Frame &videoFrame) const;

    double getMidFrameTS(double ts1, double ts2) const;

    Frame getNewFrame(const Frame &other);

#ifdef USE_VULKAN
    std::shared_ptr<QmVk::Queue> getVulkanComputeQueue(
        const std::shared_ptr<QmVk::Device> &device
    );

    std::shared_ptr<QmVk::Image> vulkanImageFromFrame(
        Frame &frame,
        const std::shared_ptr<QmVk::Device> &device = nullptr,
        CopyImageLinearToOptimalFn *copyFn = nullptr
    );
#endif

protected:
    AVPixelFormats m_supportedPixelFormats;

    QQueue<Frame> m_internalQueue;

    quint8 m_deintFlags = 0;

    // For doubler
    bool m_secondFrame = false;
    double m_lastTS = qQNaN();

#ifdef USE_VULKAN
    std::shared_ptr<QmVk::HWInterop> m_vkHwInterop;
    std::shared_ptr<QmVk::ImagePool> m_vkImagePool;
#endif
};
