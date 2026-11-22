// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <GPUInstance.hpp>
#include <QMPlay2Lib.hpp>
#include <Frame.hpp>

#include "../../qmvk/AbstractInstance.hpp"

class QVulkanInstance;
class QWindow;

namespace QmVk {

using namespace std;

class BufferPool;
class ImagePool;

class QMPLAY2SHAREDLIB_EXPORT Instance : public GPUInstance, public AbstractInstance
{
public: // Helpers
    static vector<uint32_t> readShader(const QString &fileName);

    static vk::Format fromFFmpegPixelFormat(int avPixFmt);

    static vector<shared_ptr<PhysicalDevice>> enumerateSupportedPhysicalDevices();

    static QByteArray getPhysicalDeviceID(const vk::PhysicalDeviceProperties &properties);

    static bool hasStorageImage(const shared_ptr<PhysicalDevice> &physicalDevice, vk::Format fmt);

    static bool checkFiltersSupported(const shared_ptr<PhysicalDevice> &physicalDevice);

public:
    static shared_ptr<Instance> create(bool doObtainPhysicalDevice);

public:
    Instance();
    ~Instance();

    void prepareDestroy() override;

private:
    void init(bool doObtainPhysicalDevice);

public:
    QString name() const override;
    QMPlay2CoreClass::Renderer renderer() const override;

    VideoWriter *createOrGetVideoOutput() override;

    bool checkFiltersSupported() const override;

public:
    inline QVulkanInstance *qVulkanInstance();
    inline PFN_vkGetInstanceProcAddr getVkGetInstanceProcAddr();

    void obtainPhysicalDevice();

    inline shared_ptr<PhysicalDevice> physicalDevice() const;

    bool isPhysicalDeviceGpu() const;

    inline AVPixelFormats supportedPixelFormats() const;
    bool hasStorage16bit() const;

    shared_ptr<Device> createDevice(const shared_ptr<PhysicalDevice> &physicalDevice);
    shared_ptr<Device> createOrGetDevice(const shared_ptr<PhysicalDevice> &physicalDevice);

    inline const vk::PhysicalDeviceFeatures2 &enabledDeviceFeatures() const;

    shared_ptr<BufferPool> createBufferPool();
    shared_ptr<ImagePool> createImagePool();

    shared_ptr<function<void()>> setFiltersOnOtherQueueFamiliy();
    inline bool hasFiltersOnOtherQueueFamiliy() const;

private:
    bool isCompatibleDevice(const shared_ptr<PhysicalDevice> &physicalDevice) const override final;
    void sortPhysicalDevices(vector<shared_ptr<PhysicalDevice>> &physicalDevices) const override final;

private:
    void fillSupportedFormats();

private:
    static vector<const char *> requiredPhysicalDeviceExtenstions();

private:
    QVulkanInstance *const m_qVulkanInstance;
    PFN_vkGetInstanceProcAddr m_vkGetInstanceProcAddr = nullptr;

    vk::UniqueDebugUtilsMessengerEXT m_debugUtilsMessanger;

    vk::PhysicalDeviceSamplerYcbcrConversionFeatures m_sycf;
    vk::PhysicalDeviceTimelineSemaphoreFeatures m_tsf;
    vk::PhysicalDeviceSynchronization2Features m_s2f;
    vk::PhysicalDeviceVideoDecodeVP9FeaturesKHR m_vp9;
    vk::PhysicalDeviceFeatures2 m_enabledDeviceFeatures;

    shared_ptr<PhysicalDevice> m_physicalDevice;
    AVPixelFormats m_supportedPixelFormats;

    std::mutex m_createOrGetDeviceMutex;

    function<void()> m_unsetFiltersOnOtherQueueFamiliyFn;
    std::atomic<uint32_t> m_filtersOnOtherQueueFamiliy = {0};

    QWindow *m_testWin = nullptr;
};

/* Inline implementation */

QVulkanInstance *Instance::qVulkanInstance()
{
    return m_qVulkanInstance;
}
PFN_vkGetInstanceProcAddr Instance::getVkGetInstanceProcAddr()
{
     return m_vkGetInstanceProcAddr;
}

shared_ptr<PhysicalDevice> Instance::physicalDevice() const
{
    return m_physicalDevice;
}

AVPixelFormats Instance::supportedPixelFormats() const
{
    return m_supportedPixelFormats;
}

const vk::PhysicalDeviceFeatures2 &Instance::enabledDeviceFeatures() const
{
    return m_enabledDeviceFeatures;
}

bool Instance::hasFiltersOnOtherQueueFamiliy() const
{
    return (m_filtersOnOtherQueueFamiliy > 0);
}

}
