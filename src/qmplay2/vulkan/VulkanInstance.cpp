/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2020  Błażej Szczygieł

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

#include "../../qmvk/PhysicalDevice.hpp"
#include "../../qmvk/Device.hpp"
#include "../../qmvk/MemoryPropertyFlags.hpp"

#include "VulkanInstance.hpp"
#include "VulkanBufferPool.hpp"
#include "VulkanImagePool.hpp"
#include "VulkanWriter.hpp"

#include <QVulkanInstance>
#include <QResource>

namespace QmVk {

constexpr auto s_queueFlags = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute;

vector<uint32_t> Instance::readShader(const QString &fileName)
{
    const QResource res(":/vulkan/" + fileName + ".spv");
    const auto resData = reinterpret_cast<const uint32_t *>(res.data());
    return vector<uint32_t>(resData, resData + res.size() / sizeof(uint32_t));
}

vk::Format Instance::fromFFmpegPixelFormat(int avPixFmt)
{
    switch (avPixFmt)
    {
        case AV_PIX_FMT_GRAY8:
            return vk::Format::eR8Unorm;
        case AV_PIX_FMT_GRAY9:
        case AV_PIX_FMT_GRAY10:
        case AV_PIX_FMT_GRAY12:
        case AV_PIX_FMT_GRAY14:
        case AV_PIX_FMT_GRAY16:
            return vk::Format::eR16Unorm;

        case AV_PIX_FMT_NV12:
            return vk::Format::eG8B8R82Plane420Unorm;
        case AV_PIX_FMT_P010:
        case AV_PIX_FMT_P016:
            return vk::Format::eG16B16R162Plane420Unorm;
        case AV_PIX_FMT_NV16:
            return vk::Format::eG8B8R82Plane422Unorm;
        case AV_PIX_FMT_NV20:
            return vk::Format::eG16B16R162Plane422Unorm;

        case AV_PIX_FMT_YUV420P:
        case AV_PIX_FMT_YUVJ420P:
            return vk::Format::eG8B8R83Plane420Unorm;
        case AV_PIX_FMT_YUV420P9:
        case AV_PIX_FMT_YUV420P10:
        case AV_PIX_FMT_YUV420P12:
        case AV_PIX_FMT_YUV420P14:
        case AV_PIX_FMT_YUV420P16:
            return vk::Format::eG16B16R163Plane420Unorm;
        case AV_PIX_FMT_YUV422P:
        case AV_PIX_FMT_YUVJ422P:
            return vk::Format::eG8B8R83Plane422Unorm;
        case AV_PIX_FMT_YUV422P9:
        case AV_PIX_FMT_YUV422P10:
        case AV_PIX_FMT_YUV422P12:
        case AV_PIX_FMT_YUV422P14:
        case AV_PIX_FMT_YUV422P16:
            return vk::Format::eG16B16R163Plane422Unorm;
        case AV_PIX_FMT_YUV444P:
        case AV_PIX_FMT_YUVJ444P:
        case AV_PIX_FMT_GBRP:
        case AV_PIX_FMT_GBRP9:
        case AV_PIX_FMT_GBRP10:
        case AV_PIX_FMT_GBRP12:
        case AV_PIX_FMT_GBRP14:
        case AV_PIX_FMT_GBRP16:
            return vk::Format::eG8B8R83Plane444Unorm;
        case AV_PIX_FMT_YUV444P9:
        case AV_PIX_FMT_YUV444P10:
        case AV_PIX_FMT_YUV444P12:
        case AV_PIX_FMT_YUV444P14:
        case AV_PIX_FMT_YUV444P16:
            return vk::Format::eG16B16R163Plane444Unorm;

        case AV_PIX_FMT_RGBA:
            return vk::Format::eR8G8B8A8Unorm;
        case AV_PIX_FMT_BGRA:
            return vk::Format::eB8G8R8A8Unorm;
    }
    return vk::Format::eUndefined;
}

vector<shared_ptr<PhysicalDevice>> Instance::enumerateSupportedPhysicalDevices()
{
    try
    {
        return (QMPlay2Core.isVulkanRenderer()
            ? std::static_pointer_cast<Instance>(QMPlay2Core.gpuInstance())
            : Instance::create()
        )->enumeratePhysicalDevices(true);
    }
    catch (const vk::SystemError &e)
    {
        Q_UNUSED(e)
        return {};
    }
}

QByteArray Instance::getPhysicalDeviceID(const vk::PhysicalDeviceProperties &properties)
{
    return QString("%1:%2").arg(properties.vendorID).arg(properties.deviceID).toLatin1().toBase64();
}

shared_ptr<Instance> Instance::create()
{
    auto instance = make_shared<Instance>(Priv());
    instance->init();
    return instance;
}

Instance::Instance(Priv)
    : m_qVulkanInstance(new QVulkanInstance)
{}
Instance::~Instance()
{
    delete m_qVulkanInstance;
}

void Instance::init()
{
#ifdef QT_DEBUG
    if (m_qVulkanInstance->supportedLayers().contains("VK_LAYER_LUNARG_standard_validation"))
        m_qVulkanInstance->setLayers({"VK_LAYER_LUNARG_standard_validation"});
#endif

    m_qVulkanInstance->setExtensions({
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
    });

    if (!m_qVulkanInstance->create())
        throw vk::InitializationFailedError(QString("Can't create Vulkan instance (0x%1)").arg(m_qVulkanInstance->errorCode(), 0, 16).toStdString());

    for (auto &&extension : m_qVulkanInstance->extensions())
        m_extensions.insert(extension.constData());

    static_cast<vk::Instance &>(*this) = m_qVulkanInstance->vkInstance();

    AbstractInstance::init(
        reinterpret_cast<PFN_vkGetInstanceProcAddr>(m_qVulkanInstance->getInstanceProcAddr("vkGetInstanceProcAddr"))
    );

    obtainPhysicalDevice();
}

QString Instance::name() const
{
    return "vulkan";
}
QMPlay2CoreClass::Renderer Instance::renderer() const
{
    return QMPlay2CoreClass::Renderer::Vulkan;
}

VideoWriter *Instance::createOrGetVideoOutput()
{
    if (!m_videoWriter)
        m_videoWriter = new QmVk::Writer;
    return m_videoWriter;
}

void Instance::obtainPhysicalDevice()
{
    const auto supportedPhysicalDevices = enumeratePhysicalDevices(true);

    const auto id = QMPlay2Core.getSettings().getByteArray("Vulkan/Device");
    if (!id.isEmpty())
    {
        auto it = find_if(supportedPhysicalDevices.begin(), supportedPhysicalDevices.end(), [&](const shared_ptr<PhysicalDevice> &physicalDevice) {
            return (getPhysicalDeviceID(physicalDevice->properties()) == id);
        });
        if (it != supportedPhysicalDevices.end())
        {
            m_physicalDevice = *it;
            return;
        }
    }

    m_physicalDevice = supportedPhysicalDevices[0];
}

shared_ptr<Device> Instance::createDevice(const shared_ptr<PhysicalDevice> &physicalDevice)
{
    auto physicalDeviceExtensions = requiredPhysicalDeviceExtenstions();
    physicalDeviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
#ifdef VK_USE_PLATFORM_WIN32_KHR
    physicalDeviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
#else
    physicalDeviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
#endif

    return AbstractInstance::createDevice(
        physicalDevice,
        s_queueFlags,
        requiredPhysicalDeviceFeatures(),
        physicalDeviceExtensions,
        2
    );
}

shared_ptr<BufferPool> Instance::createBufferPool()
{
    return make_shared<BufferPool>(static_pointer_cast<Instance>(shared_from_this()));
}
shared_ptr<ImagePool> Instance::createImagePool()
{
    return make_shared<ImagePool>(static_pointer_cast<Instance>(shared_from_this()));
}

bool Instance::isCompatibleDevice(const shared_ptr<PhysicalDevice> &physicalDevice) const try
{
    const auto &limits = physicalDevice->limits();

    if (limits.maxPushConstantsSize < 128)
        return false;

    constexpr auto featuresLen = sizeof(vk::PhysicalDeviceFeatures) / sizeof(vk::Bool32);
    const auto availableFeatures = physicalDevice->getFeatures();
    const auto requiredFeatures = requiredPhysicalDeviceFeatures();
    const auto availableFeaturesArr = reinterpret_cast<const vk::Bool32 *>(&availableFeatures);
    const auto requiredFeaturesArr = reinterpret_cast<const vk::Bool32 *>(&requiredFeatures);
    for (size_t i = 0; i < featuresLen; ++i)
    {
        if (requiredFeaturesArr[i] && !availableFeaturesArr[i])
            return false;
    }

    if (!physicalDevice->checkExtensions(requiredPhysicalDeviceExtenstions()))
        return false;

    physicalDevice->getQueueFamilyIndex(s_queueFlags);

    const auto requiredHostMemoryFlags =
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent |
        vk::MemoryPropertyFlagBits::eHostCached
    ;
    physicalDevice->findMemoryType(requiredHostMemoryFlags);

    auto checkFormat = [&](vk::Format format, bool img, bool buff) {
        auto fmtProps = physicalDevice->getFormatProperties(format);
        if (img)
        {
            if (!(fmtProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage))
                return false;
            if (!(fmtProps.optimalTilingFeatures & (vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eStorageImage)))
                return false;
        }
        if (buff)
        {
            if (!(fmtProps.bufferFeatures & vk::FormatFeatureFlagBits::eUniformTexelBuffer))
                return false;
        }
        return true;
    };
    if (!checkFormat(vk::Format::eR8Unorm, true, true))
        return false;
    if (!checkFormat(vk::Format::eR8G8Unorm, true, false))
        return false;
    if (!checkFormat(vk::Format::eB8G8R8A8Unorm, false, true))
        return false;

    return true;
} catch (const vk::SystemError &e) {
    Q_UNUSED(e)
    return false;
}

vk::PhysicalDeviceFeatures Instance::requiredPhysicalDeviceFeatures()
{
    vk::PhysicalDeviceFeatures requiredFeatures;
    requiredFeatures.robustBufferAccess = true;
    requiredFeatures.shaderStorageImageWriteWithoutFormat = true;
    return requiredFeatures;
}
vector<const char *> Instance::requiredPhysicalDeviceExtenstions()
{
    return {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
}

}
