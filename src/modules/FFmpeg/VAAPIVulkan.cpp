#include <VAAPIVulkan.hpp>
#include <VAAPI.hpp>

#include "../qmvk/PhysicalDevice.hpp"
#include "../qmvk/Image.hpp"

#include <vulkan/VulkanInstance.hpp>

#include <QMPlay2Core.hpp>
#include <FFCommon.hpp>
#include <Frame.hpp>

#include <QDebug>

#include <va/va_drmcommon.h>
#include <unistd.h>

// libdrm/drm_fourcc.h

#define DRM_FORMAT_MOD_INVALID 0x00ffffffffffffffull
#define DRM_FORMAT_MOD_LINEAR 0ull

using namespace QmVk;

struct VaCustomData : public MemoryObjectBase::CustomData
{
    VaCustomData(VASurfaceID surface)
        : surface(surface)
    {}

    const VASurfaceID surface;
};

VAAPIVulkan::VAAPIVulkan()
    : m_vkInstance(static_pointer_cast<Instance>(QMPlay2Core.gpuInstance()))
{
    auto physicalDevice = m_vkInstance->physicalDevice();

    if (!physicalDevice->checkExtensions({
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
    })) {
        QMPlay2Core.logError("VA-API :: Can't interoperate with Vulkan");
        m_error = true;
        return;
    }

    m_hasDrmFormatModifier = physicalDevice->checkExtensions({
        VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME,
    });
}
VAAPIVulkan::~VAAPIVulkan()
{}

QString VAAPIVulkan::name() const
{
    return VAAPIWriterName;
}

void VAAPIVulkan::map(Frame &frame)
{
    if (m_error || frame.vulkanImage())
        return;

    lock_guard<mutex> locker(m_mutex);

    const auto format = (frame.depth() > 8)
        ? vk::Format::eG16B16R162Plane420Unorm
        : vk::Format::eG8B8R82Plane420Unorm
    ;

    auto device = m_vkInstance->device();

    if (!m_images.empty())
    {
        auto image = m_images.begin()->second.get();
        if (image->device() != device || image->format() != format)
            m_images.clear();
    }

    if (!device || !frame.isHW())
        return;

    VASurfaceID id = frame.hwData();
    if (m_availableSurfaces.count(id) == 0)
        return;

    int vaField = frame.isInterlaced()
        ? (frame.isTopFieldFirst() != frame.isSecondField())
            ? VA_TOP_FIELD
            : VA_BOTTOM_FIELD
        : VA_FRAME_PICTURE
    ;
    if (!m_vaapi->filterVideo(frame, id, vaField))
        return;

    VADRMPRIMESurfaceDescriptor vaSurfaceDescr = {};
    MemoryObject::FdDescriptors fdDescriptors;
    vector<vk::DeviceSize> offsets;
    bool isLinear = false;
    bool exported = false;

    auto &vkImage = m_images[id];

    if (!vkImage || (vaField != VA_FRAME_PICTURE && m_vaapi->m_isMesaRadeon && m_vaapi->m_driverVersion >= QVersionNumber(23, 3, 0)))
    {
        if (m_vaapi->m_mutex)
            m_vaapi->m_mutex->lock();
        exported = vaExportSurfaceHandle(
            m_vaapi->VADisp,
            id,
            VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
            VA_EXPORT_SURFACE_READ_ONLY | VA_EXPORT_SURFACE_SEPARATE_LAYERS,
            &vaSurfaceDescr
        ) == VA_STATUS_SUCCESS;
        if (m_vaapi->m_mutex)
            m_vaapi->m_mutex->unlock();

        if (exported)
        {
            fdDescriptors.resize(vaSurfaceDescr.num_objects);
            offsets.resize(vaSurfaceDescr.num_layers);

            for (uint32_t i = 0; i < vaSurfaceDescr.num_layers; ++i)
            {
                const auto &layer = vaSurfaceDescr.layers[i];
                if (!m_hasDrmFormatModifier)
                    offsets[i] = layer.offset[0];

                const auto objIdx = layer.object_index[0];
                const auto &object = vaSurfaceDescr.objects[objIdx];
                fdDescriptors[objIdx].first = object.fd;
                fdDescriptors[objIdx].second = (object.size > 0)
                    ? object.size
                    : ::lseek(object.fd, 0, SEEK_END) // Older AMD Mesa drivers requires this, because the reported size is 0
                ;

                if (!m_hasDrmFormatModifier && i == 0)
                {
                    const auto drmFormatModifier = object.drm_format_modifier;
                    isLinear = (drmFormatModifier == DRM_FORMAT_MOD_LINEAR || drmFormatModifier == DRM_FORMAT_MOD_INVALID);
                }
            }
        }
    }

    auto closeFDs = [&] {
        for (auto &&fdDescriptor : fdDescriptors)
        {
            ::close(fdDescriptor.first);
        }
    };

    if (!vkImage)
    {
        if (exported) try
        {
            constexpr auto externalMemoryHandleType = vk::ExternalMemoryHandleTypeFlagBits::eDmaBufEXT;

            vk::ImageDrmFormatModifierExplicitCreateInfoEXT imageDrmFormatModifierExplicitCreateInfo;
            vk::SubresourceLayout drmFormatModifierPlaneLayout;

            auto imageCreateInfoCallback = [&](uint32_t plane, vk::ImageCreateInfo &imageCreateInfo) {
                if (!m_hasDrmFormatModifier)
                    return;

                if (plane >= vaSurfaceDescr.num_layers)
                    throw vk::LogicError("Pitches count and planes count missmatch");

                const auto &layer = vaSurfaceDescr.layers[plane];

                auto drmFormatModifier = vaSurfaceDescr.objects[layer.object_index[0]].drm_format_modifier;
                if (drmFormatModifier == DRM_FORMAT_MOD_INVALID)
                    drmFormatModifier = DRM_FORMAT_MOD_LINEAR;

                imageDrmFormatModifierExplicitCreateInfo.drmFormatModifier = drmFormatModifier;
                imageDrmFormatModifierExplicitCreateInfo.drmFormatModifierPlaneCount = 1;
                imageDrmFormatModifierExplicitCreateInfo.pPlaneLayouts = &drmFormatModifierPlaneLayout;
                imageDrmFormatModifierExplicitCreateInfo.pNext = imageCreateInfo.pNext;

                drmFormatModifierPlaneLayout.offset = layer.offset[0];
                drmFormatModifierPlaneLayout.rowPitch = layer.pitch[0];

                imageCreateInfo.tiling = vk::ImageTiling::eDrmFormatModifierEXT;
                imageCreateInfo.pNext = &imageDrmFormatModifierExplicitCreateInfo;
            };

            vkImage = Image::createExternalImport(
                device,
                vk::Extent2D(frame.width(), frame.height()),
                format,
                isLinear,
                externalMemoryHandleType,
                imageCreateInfoCallback
            );

            vkImage->setCustomData(make_unique<VaCustomData>(id));

            vkImage->importFD(
                fdDescriptors,
                offsets,
                externalMemoryHandleType
            );
        }
        catch (const vk::Error &e)
        {
            QMPlay2Core.logError(QString("VA-API :: %1").arg(e.what()));
            vkImage.reset();
            closeFDs();
        }

        if (!vkImage)
            m_error = true;
    }
    else if (exported)
    {
        // Interlaced videos needs to call "vaExportSurfaceHandle" on every frame since Mesa 23.3.0,
        // but it reuses the image buffer, so close FDs here.
        closeFDs();
    }

    if (vkImage)
    {
        frame.setVulkanImage(vkImage);
    }
    else
    {
        m_images.erase(id);
    }
}
void VAAPIVulkan::clear()
{
    lock_guard<mutex> locker(m_mutex);
    m_availableSurfaces.clear();
    m_images.clear();
}

HWInterop::SyncDataPtr VAAPIVulkan::sync(const vector<Frame> &frames, vk::SubmitInfo *submitInfo)
{
    Q_UNUSED(submitInfo)

    for (auto &&frame : frames)
    {
        if (!frame.isHW())
            continue;

        auto image = frame.vulkanImage();
        if (!image)
            continue;

        {
            lock_guard<mutex> locker(m_mutex);
            if (m_images.count(frame.hwData()) == 0)
                continue;
        }

        vaSyncSurface(m_vaapi->VADisp, image->customData<VaCustomData>()->surface);
    }

    return nullptr;
}

void VAAPIVulkan::insertAvailableSurface(uintptr_t id)
{
    lock_guard<mutex> locker(m_mutex);
    m_availableSurfaces.insert(id);
}
