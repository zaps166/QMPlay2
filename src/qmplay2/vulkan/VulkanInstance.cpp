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

#include "../../qmvk/PhysicalDevice.hpp"
#include "../../qmvk/Device.hpp"
#include "../../qmvk/MemoryPropertyFlags.hpp"

#include "VulkanInstance.hpp"
#include "VulkanBufferPool.hpp"
#include "VulkanImagePool.hpp"
#include "VulkanWriter.hpp"

#include <QVulkanInstance>
#include <QResource>
#include <QLibrary>

#if defined(Q_OS_WIN)
#   include <QRegularExpression>
#elif defined(Q_OS_LINUX)
#   include <QFile>
#   include <QDir>
#endif

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
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(56, 22, 100)
        case AV_PIX_FMT_GRAY14:
#endif
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

void Instance::prepareDestroy()
{
    m_physicalDevice.reset();
}

void Instance::init()
{
#ifdef QT_DEBUG
    m_qVulkanInstance->setLayers({"VK_LAYER_KHRONOS_validation"});
#endif

    m_qVulkanInstance->setExtensions({
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
    });

    if (!m_qVulkanInstance->create())
        throw vk::InitializationFailedError("Can't create Vulkan instance");

#ifdef QT_DEBUG
    if (!m_qVulkanInstance->layers().contains("VK_LAYER_KHRONOS_validation"))
        qWarning() << "Vulkan validation layer not found!";
#endif

    for (auto &&extension : m_qVulkanInstance->extensions())
        m_extensions.insert(extension.constData());

    static_cast<vk::Instance &>(*this) = m_qVulkanInstance->vkInstance();

    auto vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(m_qVulkanInstance->getInstanceProcAddr("vkGetInstanceProcAddr"));
    if (!vkGetInstanceProcAddr && qEnvironmentVariableIsSet("QT_VULKAN_LIB"))
    {
        QLibrary vulkanLib(QString::fromUtf8(qgetenv("QT_VULKAN_LIB")));
        if (vulkanLib.load())
            vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(vulkanLib.resolve("vkGetInstanceProcAddr"));
        if (Q_UNLIKELY(!vkGetInstanceProcAddr))
            throw vk::InitializationFailedError(("Unable to get \"vkGetInstanceProcAddr\" from " + vulkanLib.fileName()).toUtf8().toStdString());
    }

    AbstractInstance::init(vkGetInstanceProcAddr);

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

AVPixelFormats Instance::supportedPixelFormats() const
{
    auto checkImageFormat = [this](vk::Format format) {
        auto fmtProps = m_physicalDevice->getFormatProperties(format);
        if (!(fmtProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage))
            return false;
        if (!(fmtProps.optimalTilingFeatures & (vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eStorageImage)))
            return false;
        return true;
    };

    AVPixelFormats pixelFormats {
        AV_PIX_FMT_GRAY8,

        AV_PIX_FMT_NV12,
        AV_PIX_FMT_NV16,

        AV_PIX_FMT_YUV420P,
        AV_PIX_FMT_YUVJ420P,

        AV_PIX_FMT_YUV422P,
        AV_PIX_FMT_YUVJ422P,

        AV_PIX_FMT_YUV444P,
        AV_PIX_FMT_YUVJ444P,

        AV_PIX_FMT_GBRP,
    };

    if (checkImageFormat(vk::Format::eR16Unorm) && checkImageFormat(vk::Format::eR16G16Unorm))
    {
        pixelFormats += {
            AV_PIX_FMT_GRAY9,
            AV_PIX_FMT_GRAY10,
            AV_PIX_FMT_GRAY12,
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(56, 22, 100)
            AV_PIX_FMT_GRAY14,
#endif
            AV_PIX_FMT_GRAY16,

            AV_PIX_FMT_P010,
            AV_PIX_FMT_P016,
            AV_PIX_FMT_NV20,

            AV_PIX_FMT_YUV420P9,
            AV_PIX_FMT_YUV420P10,
            AV_PIX_FMT_YUV420P12,
            AV_PIX_FMT_YUV420P14,
            AV_PIX_FMT_YUV420P16,

            AV_PIX_FMT_YUV422P9,
            AV_PIX_FMT_YUV422P10,
            AV_PIX_FMT_YUV422P12,
            AV_PIX_FMT_YUV422P14,
            AV_PIX_FMT_YUV422P16,

            AV_PIX_FMT_YUV444P9,
            AV_PIX_FMT_YUV444P10,
            AV_PIX_FMT_YUV444P12,
            AV_PIX_FMT_YUV444P14,
            AV_PIX_FMT_YUV444P16,
        };
    }

    if (checkImageFormat(vk::Format::eR8G8B8A8Unorm))
        pixelFormats += AV_PIX_FMT_RGBA;

    if (checkImageFormat(vk::Format::eB8G8R8A8Unorm))
        pixelFormats += AV_PIX_FMT_BGRA;

    return pixelFormats;
}

shared_ptr<Device> Instance::createDevice(const shared_ptr<PhysicalDevice> &physicalDevice)
{
    auto physicalDeviceExtensions = requiredPhysicalDeviceExtenstions();
    physicalDeviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
#ifdef VK_USE_PLATFORM_WIN32_KHR
    physicalDeviceExtensions.push_back(VK_KHR_WIN32_KEYED_MUTEX_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
#else
    physicalDeviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
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
void Instance::sortPhysicalDevices(vector<shared_ptr<PhysicalDevice>> &physicalDevices) const
{
    auto setAsFirst = [&](auto &&it) {
        auto primaryPhysicalDevice = move(*it);
        physicalDevices.erase(it);
        physicalDevices.insert(physicalDevices.begin(), move(primaryPhysicalDevice));
    };
#if defined(Q_OS_WIN)
    for (DWORD devIdx = 0;; ++devIdx)
    {
        DISPLAY_DEVICE displayDevice = {};
        displayDevice.cb = sizeof(displayDevice);
        if (!EnumDisplayDevicesA(nullptr, devIdx, &displayDevice, 0))
            break;

        if (!(displayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE))
            continue;

        const QRegularExpression rx(R"(VEN_([0-9a-f]+)&DEV_([0-9a-f]+))", QRegularExpression::CaseInsensitiveOption);
        const auto match = rx.match(displayDevice.DeviceID);

        bool okVen = false, okDev = false;
        const uint32_t ven = match.captured(1).toUInt(&okVen, 16);
        const uint32_t dev = match.captured(2).toUInt(&okDev, 16);
        if (okVen && okDev)
        {
            auto it = find_if(physicalDevices.begin(), physicalDevices.end(), [&](const shared_ptr<PhysicalDevice> &physicalDevice) {
                const auto &properties = physicalDevice->properties();
                return (properties.vendorID == ven && properties.deviceID == dev);
            });
            if (it != physicalDevices.begin() && it != physicalDevices.end())
            {
                setAsFirst(it);
            }
        }

        break;
    }
#elif defined(Q_OS_LINUX)
    const auto cards = QDir("/sys/class/drm").entryInfoList({"renderD*"}, QDir::Dirs);
    for (auto &&card : cards)
    {
        QFile f(card.filePath() + "/device/boot_vga");
        char c = 0;
        if (f.open(QFile::ReadOnly) && f.getChar(&c) && c == '1')
        {
            const auto cardRealPath = card.symLinkTarget();
            auto it = find_if(physicalDevices.begin(), physicalDevices.end(), [&](const shared_ptr<PhysicalDevice> &physicalDevice) {
                return cardRealPath.contains(QString::fromStdString(physicalDevice->linuxPCIPath()));
            });
            if (it != physicalDevices.begin() && it != physicalDevices.end())
            {
                setAsFirst(it);
            }
            break;
        }
    }
#else
    Q_UNUSED(physicalDevices)
    Q_UNUSED(setAsFirst)
#endif
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
