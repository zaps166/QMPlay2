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

#include "../../qmvk/PhysicalDevice.hpp"
#include "../../qmvk/Device.hpp"
#include "../../qmvk/MemoryPropertyFlags.hpp"
#include "../../qmvk/Image.hpp"

#include "VulkanInstance.hpp"
#include "VulkanBufferPool.hpp"
#include "VulkanImagePool.hpp"
#include "VulkanWriter.hpp"

#include <QGuiApplication>
#include <QVulkanInstance>
#include <QVersionNumber>
#include <QResource>
#include <QLibrary>
#include <QWindow>

#if defined(Q_OS_WIN)
#   include <QRegularExpression>
#elif defined(Q_OS_LINUX)
#   include <QFile>
#   include <QDir>
#endif

namespace QmVk {

#ifdef QT_DEBUG
#   define USE_VALIDATION_LAYER 1
#else
#   define USE_VALIDATION_LAYER 0
#endif

#if USE_VALIDATION_LAYER
static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagBitsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{
    if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
        qDebug() << pCallbackData->pMessage;
    else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
        qDebug() << pCallbackData->pMessage;
    else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
        qInfo() << pCallbackData->pMessage;
    else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose)
        qDebug() << pCallbackData->pMessage;
    return VK_FALSE;
}
#endif

#if defined(Q_OS_LINUX)
static QFileInfo getPrimaryX11CardFile()
{
    // FIXME: Is it better way to ask X11 about primary GPU?

    if (QGuiApplication::platformName() != "xcb")
        return QFileInfo();

    QLibrary libX11("libX11.so.6");
    QLibrary libEGL("libEGL.so.1");
    if (!libX11.load() || !libEGL.load())
        return QFileInfo();

    using XOpenDisplayType = void *(*)(const char *name);
    using XCloseDisplayType = int (*)(void *display);

    auto XOpenDisplayFunc = (XOpenDisplayType)libX11.resolve("XOpenDisplay");
    auto XCloseDisplayFunc = (XCloseDisplayType)libX11.resolve("XCloseDisplay");
    if (!XOpenDisplayFunc || !XCloseDisplayFunc)
        return QFileInfo();

    using eglGetProcAddressType = void *(*)(const char *);
    using eglGetDisplayType = void *(*)(void *);
    using eglInitializeType = unsigned (*)(void *, int *, int *);
    using eglQueryStringType = const char *(*)(void *, int);
    using eglQueryDisplayAttribEXTType = unsigned (*)(void *, int, void *);
    using eglTerminateType = unsigned (*)(void *);

    auto eglGetProcAddress = (eglGetProcAddressType)libEGL.resolve("eglGetProcAddress");
    auto eglGetDisplayFunc = (eglGetDisplayType)libEGL.resolve("eglGetDisplay");
    auto eglInitializeFunc = (eglInitializeType)libEGL.resolve("eglInitialize");
    auto eglQueryStringFunc = (eglQueryStringType)libEGL.resolve("eglQueryString");
    auto eglTerminateFunc = (eglTerminateType)libEGL.resolve("eglTerminate");
    if (!eglGetProcAddress || !eglGetDisplayFunc || !eglInitializeFunc || !eglQueryStringFunc || !eglTerminateFunc)
        return QFileInfo();

    auto dpy = XOpenDisplayFunc(nullptr);
    if (!dpy)
        return QFileInfo();

    QByteArray cardFilePath;

    auto eglDpy = eglGetDisplayFunc(dpy);
    if (eglDpy && eglInitializeFunc(eglDpy, nullptr, nullptr))
    {
        constexpr int EGLExtensions = 0x3055;
        const bool hasDeviceQuery = QByteArray(eglQueryStringFunc(nullptr, EGLExtensions)).contains("EGL_EXT_device_query");

        auto eglQueryDisplayAttribEXTFunc = (eglQueryDisplayAttribEXTType)eglGetProcAddress("eglQueryDisplayAttribEXT");
        auto eglQueryDeviceStringEXTFunc = (eglQueryStringType)eglGetProcAddress("eglQueryDeviceStringEXT");

        if (hasDeviceQuery && eglQueryDisplayAttribEXTFunc && eglQueryDeviceStringEXTFunc)
        {
            constexpr int EGLDeviceExt = 0x322C;
            constexpr int EGLDrmDeviceFileExt = 0x3233;
            void *eglDev = nullptr;
            if (eglQueryDisplayAttribEXTFunc(eglDpy, EGLDeviceExt, &eglDev) && eglDev)
            {
                if (const char *file = eglQueryDeviceStringEXTFunc(eglDev, EGLDrmDeviceFileExt))
                    cardFilePath = file;
            }
        }

        eglTerminateFunc(eglDpy);
    }

    XCloseDisplayFunc(dpy);

    return QFileInfo(cardFilePath);
}
#endif

constexpr auto s_queueFlags = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute;

vector<uint32_t> Instance::readShader(const QString &fileName)
{
    const QResource res(":/vulkan/" + fileName + ".spv");
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    const auto data = res.uncompressedData();
    const auto u32Data = reinterpret_cast<const uint32_t *>(data.data());
    return vector<uint32_t>(u32Data, u32Data + data.size() / sizeof(uint32_t));
#else
    const auto resData = reinterpret_cast<const uint32_t *>(res.data());
    return vector<uint32_t>(resData, resData + res.size() / sizeof(uint32_t));
#endif
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
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 2, 100)
        case AV_PIX_FMT_P012:
#endif
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
            return vk::Format::eG8B8R83Plane444Unorm;
        case AV_PIX_FMT_GBRP9:
        case AV_PIX_FMT_GBRP10:
        case AV_PIX_FMT_GBRP12:
        case AV_PIX_FMT_GBRP14:
        case AV_PIX_FMT_GBRP16:
        case AV_PIX_FMT_YUV444P9:
        case AV_PIX_FMT_YUV444P10:
        case AV_PIX_FMT_YUV444P12:
        case AV_PIX_FMT_YUV444P14:
        case AV_PIX_FMT_YUV444P16:
            return vk::Format::eG16B16R163Plane444Unorm;

        case AV_PIX_FMT_RGBA:
            return vk::Format::eR8G8B8A8Unorm;
        case AV_PIX_FMT_RGBA64:
            return vk::Format::eR16G16B16A16Unorm;
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
            : Instance::create(false)
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

bool Instance::hasStorageImage(const shared_ptr<PhysicalDevice> &physicalDevice, vk::Format fmt)
{
    return Image::checkImageFormat(
        physicalDevice,
        fmt,
        false,
        vk::FormatFeatureFlagBits::eStorageImage
    );
}

bool Instance::checkFiltersSupported(const shared_ptr<PhysicalDevice> &physicalDevice)
{
    if (!physicalDevice)
        return false;

    if (!physicalDevice->isGpu() || !physicalDevice->getFeatures().shaderStorageImageWriteWithoutFormat)
        return false;

    return hasStorageImage(physicalDevice, vk::Format::eR8Unorm) && hasStorageImage(physicalDevice, vk::Format::eR8G8Unorm);
}

shared_ptr<Instance> Instance::create(bool doObtainPhysicalDevice)
{
    auto instance = make_shared<Instance>(Priv());
    instance->init(doObtainPhysicalDevice);
    return instance;
}

Instance::Instance(Priv)
    : m_qVulkanInstance(new QVulkanInstance)
{}
Instance::~Instance()
{
    delete m_testWin;
    delete m_qVulkanInstance;
    m_debugUtilsMessanger.reset();
    if (*this)
        destroy();
}

void Instance::prepareDestroy()
{
    m_physicalDevice.reset();
    fillSupportedFormats();
}

void Instance::init(bool doObtainPhysicalDevice)
{
    static const auto getInstanceProcAddr = AbstractInstance::loadVulkanLibrary(
        qEnvironmentVariable("QT_VULKAN_LIB").toStdString()
    );
    if (!getInstanceProcAddr)
        throw vk::InitializationFailedError("Can't resolve \"vkGetInstanceProcAddr\" Vulkan function");

    AbstractInstance::initDispatchLoaderDynamic(getInstanceProcAddr);
    fetchAllExtensions();

    vector<const char *> layers;
    const auto instanceExtensions = filterAvailableExtensions({
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
        "VK_KHR_android_surface",
        "VK_KHR_wayland_surface",
        "VK_KHR_win32_surface",
        "VK_KHR_xcb_surface",
#if USE_VALIDATION_LAYER
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    });

#if USE_VALIDATION_LAYER
    const bool hasValidationLayer =
           checkExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
        && getAllInstanceLayers().count("VK_LAYER_KHRONOS_validation") > 0
    ;
    if (hasValidationLayer)
        layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName = "QMPlay2";
    appInfo.apiVersion = AbstractInstance::isVk10()
        ? VK_API_VERSION_1_0
        : VK_API_VERSION_1_1
    ;

    vk::InstanceCreateInfo createInfo;
    if (checkExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
        createInfo.flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = layers.size();
    createInfo.ppEnabledLayerNames = layers.data();
    createInfo.enabledExtensionCount = instanceExtensions.size();
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    static_cast<vk::Instance &>(*this) = vk::createInstance(createInfo);
    AbstractInstance::initDispatchLoaderDynamic(getInstanceProcAddr, *this);

    m_extensions.clear();
    m_extensions.reserve(instanceExtensions.size());
    for (auto &&extension : instanceExtensions)
        m_extensions.insert(extension);

#if USE_VALIDATION_LAYER
    if (hasValidationLayer)
    {
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessagngerCreateInfo;
        debugUtilsMessagngerCreateInfo.messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
        ;
        debugUtilsMessagngerCreateInfo.messageType =
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
        ;
        debugUtilsMessagngerCreateInfo.pfnUserCallback = reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(debugCallback);
        m_debugUtilsMessanger = createDebugUtilsMessengerEXTUnique(debugUtilsMessagngerCreateInfo);
    }
    else
    {
        qWarning() << "Vulkan validation layer not found!";
    }
#endif

    m_qVulkanInstance->setVkInstance(*this);
    if (!m_qVulkanInstance->create())
        throw vk::InitializationFailedError("Can't create Qt Vulkan instance");

    m_vkGetInstanceProcAddr = getInstanceProcAddr;

    m_testWin = new QWindow;
    m_testWin->setSurfaceType(QWindow::VulkanSurface);
    m_testWin->setVulkanInstance(m_qVulkanInstance);
    m_testWin->create();

    if (doObtainPhysicalDevice)
        obtainPhysicalDevice();

    m_unsetFiltersOnOtherQueueFamiliyFn = [this] {
        --m_filtersOnOtherQueueFamiliy;
    };
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

bool Instance::checkFiltersSupported() const
{
    return checkFiltersSupported(m_physicalDevice);
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
            fillSupportedFormats();
            return;
        }
    }

    m_physicalDevice = supportedPhysicalDevices[0];
    fillSupportedFormats();
}

bool Instance::isPhysicalDeviceGpu() const
{
    if (m_physicalDevice)
        return m_physicalDevice->isGpu();
    return false;
}

bool Instance::hasStorage16bit() const
{
    return hasStorageImage(m_physicalDevice, vk::Format::eR16Unorm) && hasStorageImage(m_physicalDevice, vk::Format::eR16G16Unorm);
}

shared_ptr<Device> Instance::createDevice(const shared_ptr<PhysicalDevice> &physicalDevice)
{
    auto physicalDeviceExtensions = requiredPhysicalDeviceExtenstions();
    physicalDeviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);

    physicalDeviceExtensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);

    if (!AbstractInstance::isVk10() && !physicalDevice->isVk10())
    {
        // Required for Vulkan video decoding
        physicalDeviceExtensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
        physicalDeviceExtensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
        physicalDeviceExtensions.push_back(VK_KHR_VIDEO_QUEUE_EXTENSION_NAME);
        physicalDeviceExtensions.push_back(VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME);
        physicalDeviceExtensions.push_back(VK_KHR_VIDEO_DECODE_H264_EXTENSION_NAME);
        physicalDeviceExtensions.push_back(VK_KHR_VIDEO_DECODE_H265_EXTENSION_NAME);
        physicalDeviceExtensions.push_back(VK_KHR_VIDEO_DECODE_AV1_EXTENSION_NAME);
#if 0
        // Preferred when FFmpeg does a host-copy after decoding
        physicalDeviceExtensions.push_back(VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME);
#endif
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR
    physicalDeviceExtensions.push_back(VK_KHR_WIN32_KEYED_MUTEX_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
#else
    physicalDeviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME);
    physicalDeviceExtensions.push_back(VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME);
#endif

    m_sycf.samplerYcbcrConversion = false;
    m_tsf.timelineSemaphore = false;
    m_s2f.synchronization2 = false;
    if (!AbstractInstance::isVk10() || checkExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
    {
        vk::PhysicalDeviceFeatures2 features2;
        tie(features2, m_sycf, m_tsf, m_s2f) = physicalDevice->getFeatures2KHR<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceSamplerYcbcrConversionFeatures,
            vk::PhysicalDeviceTimelineSemaphoreFeatures,
            vk::PhysicalDeviceSynchronization2Features
        >().get<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceSamplerYcbcrConversionFeatures,
            vk::PhysicalDeviceTimelineSemaphoreFeatures,
            vk::PhysicalDeviceSynchronization2Features
        >();
        m_enabledDeviceFeatures.features.shaderStorageImageWriteWithoutFormat = features2.features.shaderStorageImageWriteWithoutFormat;
    }
    else
    {
        m_enabledDeviceFeatures.features.shaderStorageImageWriteWithoutFormat = physicalDevice->getFeatures().shaderStorageImageWriteWithoutFormat;
    }
    m_enabledDeviceFeatures.pNext = &m_sycf;
    m_sycf.pNext = &m_tsf;
    m_tsf.pNext = &m_s2f;
    m_s2f.pNext = nullptr;

    const auto cq = physicalDevice->getQueuesFamily(vk::QueueFlagBits::eCompute);
    const auto dq = physicalDevice->getQueuesFamily(vk::QueueFlagBits::eVideoDecodeKHR);
    auto queues = physicalDevice->getQueuesFamily(s_queueFlags, false, false, true);
    queues.reserve(queues.size() + cq.size() + dq.size());
    queues.insert(queues.end(), cq.begin(), cq.end());
    queues.insert(queues.end(), dq.begin(), dq.end());

    return AbstractInstance::createDevice(
        physicalDevice,
        m_enabledDeviceFeatures,
        physicalDeviceExtensions,
        queues
    );
}
shared_ptr<Device> Instance::createOrGetDevice(const shared_ptr<PhysicalDevice> &physicalDevice)
{
    lock_guard<mutex> locker(m_createOrGetDeviceMutex);
    if (auto dev = device())
    {
        if (dev && *dev->physicalDevice() != *physicalDevice)
        {
            resetDevice(dev);
            dev.reset();
        }
        else
        {
            return dev;
        }
    }
    return createDevice(physicalDevice);
}

shared_ptr<BufferPool> Instance::createBufferPool()
{
    return make_shared<BufferPool>(static_pointer_cast<Instance>(shared_from_this()));
}
shared_ptr<ImagePool> Instance::createImagePool()
{
    return make_shared<ImagePool>(static_pointer_cast<Instance>(shared_from_this()));
}

shared_ptr<function<void()>> Instance::setFiltersOnOtherQueueFamiliy()
{
    ++m_filtersOnOtherQueueFamiliy;
    return shared_ptr<function<void()>>(&m_unsetFiltersOnOtherQueueFamiliyFn, [](auto &&fn) {
        (*fn)();
    });
}

bool Instance::isCompatibleDevice(const shared_ptr<PhysicalDevice> &physicalDevice) const try
{
    const auto &properties = physicalDevice->properties();
    const auto &limits = physicalDevice->limits();

    QStringList errors;

    if (limits.maxPushConstantsSize < 128)
        errors.push_back("Push constants size is too small");

    const auto requiredExtenstions = requiredPhysicalDeviceExtenstions();
    if (!physicalDevice->checkExtensions(requiredExtenstions))
    {
        QString names;
        for (auto &&requiredPhysicalDeviceExtenstion : requiredExtenstions)
        {
            names += requiredPhysicalDeviceExtenstion;
            names += ", ";
        }
        names.chop(2);
        errors.push_back("Missing one or more required physical device extensions: " + names);
    }

    vector<pair<uint32_t, uint32_t>> queuesFamily;
    try
    {
        queuesFamily = physicalDevice->getQueuesFamily(s_queueFlags, false, true, true);
    }
    catch (const vk::SystemError &e)
    {
        errors.push_back(e.what());
    }

    try
    {
        const auto requiredHostMemoryFlags =
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent
        ;
        physicalDevice->findMemoryType(requiredHostMemoryFlags);
    }
    catch (const vk::SystemError &e)
    {
        errors.push_back(e.what());
    }

    auto checkFormat = [&](vk::Format format, bool img, bool buff) {
        const auto &fmtProps = physicalDevice->getFormatPropertiesCached(format);
        if (img)
        {
            constexpr auto flags = vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eSampledImageFilterLinear;
            if ((fmtProps.optimalTilingFeatures & flags) != flags)
                errors.push_back(QString::fromStdString("Missing optimal tiling sampled or storage image for format: " + vk::to_string(format)));
        }
        if (buff)
        {
            if (!(fmtProps.bufferFeatures & vk::FormatFeatureFlagBits::eUniformTexelBuffer))
                errors.push_back(QString::fromStdString("Missing uniform texel buffer for format: " + vk::to_string(format)));
        }
    };
    checkFormat(vk::Format::eR8Unorm, true, true);
    checkFormat(vk::Format::eR8G8Unorm, true, false);
    checkFormat(vk::Format::eB8G8R8A8Unorm, false, true);

    if (!queuesFamily.empty() && !m_qVulkanInstance->supportsPresent(*physicalDevice, queuesFamily[0].first, m_testWin))
        errors.push_back("Present is not supported");

    if (errors.isEmpty())
        return true;

    QString errorString = "Vulkan :: Discarding \"";
    errorString += properties.deviceName.data();
    errorString += "\", because:";
    for (auto &&error : std::as_const(errors))
        errorString += "\n   - " + error;
    qDebug().noquote() << errorString;

    return false;
} catch (const vk::SystemError &e) {
    Q_UNUSED(e)
    return false;
}
void Instance::sortPhysicalDevices(vector<shared_ptr<PhysicalDevice>> &physicalDevices) const
{
    int nGpus = 0;
    for (auto &&physicalDevice : physicalDevices)
    {
        if (physicalDevice->isGpu())
            ++nGpus;
    }
    if (nGpus <= 1)
        return; // Nothing to do

    auto setAsFirst = [&](auto &&it) {
        auto primaryPhysicalDevice = move(*it);
        physicalDevices.erase(it);
        physicalDevices.insert(physicalDevices.begin(), move(primaryPhysicalDevice));
    };

#if defined(Q_OS_WIN)
    for (DWORD devIdx = 0;; ++devIdx)
    {
        DISPLAY_DEVICEA displayDevice = {};
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
    auto maybeSetAsFirst = [&](const QString &nameWithPath) {
        auto it = find_if(physicalDevices.begin(), physicalDevices.end(), [&](const shared_ptr<PhysicalDevice> &physicalDevice) {
            return nameWithPath.contains(QString::fromStdString(physicalDevice->linuxPCIPath()));
        });
        if (it != physicalDevices.begin() && it != physicalDevices.end())
        {
            setAsFirst(it);
        }
    };

    const auto cards = QDir("/sys/class/drm").entryInfoList({"renderD*"}, QDir::Dirs);
    for (auto &&card : cards)
    {
        QFile f(card.filePath() + "/device/boot_vga");
        char c = 0;
        if (f.open(QFile::ReadOnly) && f.getChar(&c) && c == '1')
        {
            maybeSetAsFirst(card.symLinkTarget());
            break;
        }
    }

    const auto primaryCard = getPrimaryX11CardFile();
    if (primaryCard.exists())
    {
        const auto cards = QDir("/dev/dri/by-path").entryInfoList({"*-card"}, QDir::Files);
        for (auto &&card : cards)
        {
            if (card.symLinkTarget() == primaryCard.filePath())
            {
                maybeSetAsFirst(card.fileName());
                break;
            }
        }
    }
#else
    Q_UNUSED(physicalDevices)
    Q_UNUSED(setAsFirst)
#endif
}

void Instance::fillSupportedFormats()
{
    m_supportedPixelFormats.clear();

    if (!m_physicalDevice)
        return;

    // Supported image formats

    auto checkImageFormat = [this](vk::Format format) {
        return Image::checkImageFormat(
            m_physicalDevice,
            format,
            false,
            vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eSampledImageFilterLinear
        );
    };

    m_supportedPixelFormats += {
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
        m_supportedPixelFormats += {
            AV_PIX_FMT_GRAY9,
            AV_PIX_FMT_GRAY10,
            AV_PIX_FMT_GRAY12,
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(56, 22, 100)
            AV_PIX_FMT_GRAY14,
#endif
            AV_PIX_FMT_GRAY16,

            AV_PIX_FMT_P010,
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 2, 100)
            AV_PIX_FMT_P012,
#endif
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

            AV_PIX_FMT_GBRP9,
            AV_PIX_FMT_GBRP10,
            AV_PIX_FMT_GBRP12,
            AV_PIX_FMT_GBRP14,
            AV_PIX_FMT_GBRP16,
        };
    }

    if (checkImageFormat(vk::Format::eR8G8B8A8Unorm))
        m_supportedPixelFormats += AV_PIX_FMT_RGBA;
    if (checkImageFormat(vk::Format::eR16G16B16A16Unorm))
        m_supportedPixelFormats += AV_PIX_FMT_RGBA64;

    if (checkImageFormat(vk::Format::eB8G8R8A8Unorm))
        m_supportedPixelFormats += AV_PIX_FMT_BGRA;
}

vector<const char *> Instance::requiredPhysicalDeviceExtenstions()
{
    return {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
}

}
