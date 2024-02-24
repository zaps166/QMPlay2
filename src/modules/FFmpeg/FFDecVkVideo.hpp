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
    FFDecVkVideo(Module &);
    ~FFDecVkVideo();

    bool set() override;

    QString name() const override;

    int decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurryUp) override;

    bool open(StreamInfo &streamInfo) override;

private:
    bool initHw();
    void destroyHw();

private:
    AVCodec *m_codec = nullptr;
    std::shared_ptr<QmVk::Instance> m_vkInstance;
    std::shared_ptr<QmVk::PhysicalDevice> m_physicalDevice;
    std::shared_ptr<QmVk::Device> m_device;
    AVBufferRef *m_hwDeviceBufferRef = nullptr;
    std::shared_ptr<VkVideoVulkan> m_vkVideoVulkan;
    std::unordered_map<uint64_t, std::unique_lock<std::mutex>> m_queueLocks;
    bool m_recovering = false;
};
