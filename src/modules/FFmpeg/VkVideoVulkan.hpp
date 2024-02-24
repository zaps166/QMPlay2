/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

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
    void insertAvailableAvVkFrames(uintptr_t avVkFramePtr, int codedHeight);

private:
    const std::shared_ptr<QmVk::Instance> m_vkInstance;

    std::mutex m_mutex;

    std::unordered_map<uintptr_t, bool> m_availableAvVkFrames;
    std::unordered_map<uintptr_t, std::shared_ptr<QmVk::Image>> m_images;

    int m_codedHeight = 0;
};
