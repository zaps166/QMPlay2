// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <FFDecHWAccel.hpp>

#include <unordered_map>
#include <mutex>

class VkVideoVulkan;

namespace QmVk {

class Instance;
class PhysicalDevice;
class Device;

}

class FFMPEG_EXPORT FFDecVkVideo final : public FFDecHWAccel
{
public:
    FFDecVkVideo(Module &module, bool hwDownload);
    ~FFDecVkVideo();

    bool set() override;

    QString name() const override;

    int decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurryUp) override;

    bool open(StreamInfo &streamInfo) override;

private:
    bool initHw();
    void destroyHw();

private:
    const bool m_hwDownload;
    AVCodec *m_codec = nullptr;
    std::shared_ptr<QmVk::Instance> m_vkInstance;
    std::shared_ptr<QmVk::PhysicalDevice> m_physicalDevice;
    std::shared_ptr<QmVk::Device> m_device;
    AVBufferRef *m_hwDeviceBufferRef = nullptr;
    std::shared_ptr<VkVideoVulkan> m_vkVideoVulkan;
    std::unordered_map<uint64_t, std::unique_lock<std::mutex>> m_queueLocks;
    bool m_recovering = false;
};
