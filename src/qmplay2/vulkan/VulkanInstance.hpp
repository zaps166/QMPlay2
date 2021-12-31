/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2021  Błażej Szczygieł

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

#include <GPUInstance.hpp>
#include <QMPlay2Lib.hpp>
#include <Frame.hpp>

#include "../../qmvk/AbstractInstance.hpp"

#include <mutex>

class QVulkanInstance;
class QWindow;

namespace QmVk {

using namespace std;

class BufferPool;
class ImagePool;

class QMPLAY2SHAREDLIB_EXPORT Instance : public GPUInstance, public AbstractInstance
{
    struct Priv {};

public: // Helpers
    static vector<uint32_t> readShader(const QString &fileName);

    static vk::Format fromFFmpegPixelFormat(int avPixFmt);

    static vector<shared_ptr<PhysicalDevice>> enumerateSupportedPhysicalDevices();

    static QByteArray getPhysicalDeviceID(const vk::PhysicalDeviceProperties &properties);

public:
    static shared_ptr<Instance> create();

public:
    Instance(Priv);
    ~Instance();

    void prepareDestroy() override;

private:
    void init();

public:
    QString name() const override;
    QMPlay2CoreClass::Renderer renderer() const override;

    VideoWriter *createOrGetVideoOutput() override;

public:
    inline QVulkanInstance *qVulkanInstance();

    void obtainPhysicalDevice();

    inline shared_ptr<PhysicalDevice> physicalDevice() const;

    AVPixelFormats supportedPixelFormats() const;

    shared_ptr<Device> createDevice(const shared_ptr<PhysicalDevice> &physicalDevice);

    shared_ptr<BufferPool> createBufferPool();
    shared_ptr<ImagePool> createImagePool();

    bool checkLinearTilingSampledImageSupported(const shared_ptr<Image> &image) const;

private:
    bool isCompatibleDevice(const shared_ptr<PhysicalDevice> &physicalDevice) const override final;
    void sortPhysicalDevices(vector<shared_ptr<PhysicalDevice>> &physicalDevices) const override final;

private:
    void fillSupportedFormats();

private:
    static vk::PhysicalDeviceFeatures requiredPhysicalDeviceFeatures();
    static vector<const char *> requiredPhysicalDeviceExtenstions();

private:
    QVulkanInstance *const m_qVulkanInstance;

    shared_ptr<PhysicalDevice> m_physicalDevice;
    set<vk::Format> m_formatsLinearTilingSampledImage;
    AVPixelFormats m_supportedPixelFormats;

    weak_ptr<Device> m_deviceWeak;
    mutable mutex m_deviceMutex;

    QWindow *m_testWin = nullptr;
};

/* Inline implementation */

QVulkanInstance *Instance::qVulkanInstance()
{
    return m_qVulkanInstance;
}

shared_ptr<PhysicalDevice> Instance::physicalDevice() const
{
    return m_physicalDevice;
}

}
