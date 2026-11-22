// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <vulkan/VulkanHWInterop.hpp>

#include <unordered_map>
#include <memory>
#include <mutex>

namespace QmVk {
class Instance;
class Image;
}

class VkVideoVulkan final : public QmVk::HWInterop
{
public:
    VkVideoVulkan(const std::shared_ptr<QmVk::Instance> &device);
    ~VkVideoVulkan();

    QString name() const override;

    void map(Frame &frame) override;
    void clear() override;

    SyncDataPtr sync(const std::vector<Frame> &frames, vk::SubmitInfo *submitInfo = nullptr) override;

    void updateInfo(const std::vector<Frame> &frames) override;

public:
    void insertAvailableAvVkFrames(uintptr_t avVkFramePtr, int codedWidth, int codedHeight);

private:
    const std::shared_ptr<QmVk::Instance> m_vkInstance;

    std::mutex m_mutex;

    std::unordered_map<uintptr_t, bool> m_availableAvVkFrames;
    std::unordered_map<uintptr_t, std::shared_ptr<QmVk::Image>> m_images;

    int m_codedWidth = 0;
    int m_codedHeight = 0;
};
