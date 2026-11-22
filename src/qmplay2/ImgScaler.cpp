// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <ImgScaler.hpp>

#include <Frame.hpp>

#ifdef USE_VULKAN
#   include "../qmvk/Image.hpp"
#endif

extern "C"
{
    #include <libswscale/swscale.h>
}

ImgScaler::ImgScaler() :
    m_swsCtx(nullptr),
    m_srcH(0), m_dstLinesize(0)
{}

bool ImgScaler::create(const Frame &videoFrame, int newWdst, int newHdst)
{
    if (videoFrame.isEmpty())
        return false;

    if (newWdst < 0)
        newWdst = videoFrame.width();
    if (newHdst < 0)
        newHdst = videoFrame.height();
    m_srcH = videoFrame.height();
    m_dstLinesize = newWdst << 2;
    m_swsCtx = sws_getCachedContext(
        m_swsCtx,
        videoFrame.width(),
        m_srcH,
        videoFrame.pixelFormat(),
        newWdst,
        newHdst,
        AV_PIX_FMT_RGB32,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr
    );
    return (bool)m_swsCtx;
}
bool ImgScaler::scale(const Frame &src, void *dst)
{
    const int numPlanes = src.numPlanes();
    const uint8_t *srcData[3] = {};

    auto swsScale = [&](int *srcLinesize) {
        sws_scale(m_swsCtx, srcData, srcLinesize, 0, m_srcH, (uint8_t **)&dst, &m_dstLinesize);
    };

    if (src.hasCPUAccess())
    {
        for (int i = 0; i < numPlanes; ++i)
            srcData[i] = src.constData(i);

        swsScale(src.linesize());
        return true;
    }
#ifdef USE_VULKAN
    else if (auto vkImage = src.vulkanImage()) try
    {
        auto hostVkImage = QmVk::Image::createLinear(
            vkImage->device(),
            vk::Extent2D(src.width(), src.height()),
            vkImage->format()
        );
        vkImage->copyTo(hostVkImage);
        for (int i = 0; i < numPlanes; ++i)
            srcData[i] = hostVkImage->map<uint8_t>(i);

        int srcLinesize[3] = {};
        for (int i = 0; i < numPlanes; ++i)
            srcLinesize[i] = hostVkImage->linesize(i);

        swsScale(srcLinesize);
        return true;
    }
    catch (const vk::SystemError &e)
    {
        Q_UNUSED(e)
    }
#endif
    return false;
}
void ImgScaler::scale(const void *src[], const int srcLinesize[], void *dst)
{
    sws_scale(m_swsCtx, (const uint8_t **)src, srcLinesize, 0, m_srcH, (uint8_t **)&dst, &m_dstLinesize);
}
void ImgScaler::destroy()
{
    if (m_swsCtx)
    {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
}
