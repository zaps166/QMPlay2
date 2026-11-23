// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <CuvidHWInterop.hpp>
#include <vulkan/VulkanHWInterop.hpp>

namespace QmVk {
class ImagePool;
}

class CuvidVulkan final : public CuvidHWInterop, public QmVk::HWInterop
{
public:
    CuvidVulkan(const std::shared_ptr<CUcontext> &cuCtx);
    ~CuvidVulkan();

    QString name() const override;

    void map(Frame &frame) override;
    void clear() override;

    SyncDataPtr sync(const std::vector<Frame> &frames, vk::SubmitInfo *submitInfo = nullptr) override;

private:
    void ensureSemaphore();
    void destroySemaphore();

private:
    const std::shared_ptr<QmVk::ImagePool> m_vkImagePool;
    CUstream m_cuStream = nullptr;

    vk::ExternalMemoryHandleTypeFlagBits m_vkMemType;
    CUexternalMemoryHandleType m_cuMemType;

    vk::ExternalSemaphoreHandleTypeFlagBits m_vkSemHandleType;
    CUexternalSemaphoreHandleType m_cuSemHandleType;

    std::shared_ptr<QmVk::Semaphore> m_semaphore;
    CUexternalSemaphore m_cuSemaphore = nullptr;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    HANDLE m_semaphoreHandle = nullptr;
#else
    int m_semaphoreHandle = -1;
#endif

    std::mutex m_picturesToSyncMutex;
    std::unordered_set<int> m_picturesToSync;

    bool m_linear = false;
};
