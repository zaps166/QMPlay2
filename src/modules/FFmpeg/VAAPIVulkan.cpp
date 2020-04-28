#include <VAAPIVulkan.hpp>
#include <VAAPI.hpp>

#include "../qmvk/Image.hpp"

#include <vulkan/VulkanInstance.hpp>

#include <QMPlay2Core.hpp>
#include <FFCommon.hpp>
#include <Frame.hpp>

#include <QDebug>

#include <va/va_drmcommon.h>
#include <linux/dma-buf.h>
#include <sys/ioctl.h>
#include <unistd.h>

using namespace QmVk;

struct FDCustomData : public MemoryObjectBase::CustomData
{
    ~FDCustomData()
    {
        for (auto &&fd : fds)
            ::close(fd);
    }

    vector<int> fds;
};

VAAPIVulkan::VAAPIVulkan(const shared_ptr<VAAPI> &vaapi)
    : m_vkInstance(static_pointer_cast<Instance>(QMPlay2Core.gpuInstance()))
    , m_vaapi(vaapi)
{}
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

    const auto format = (frame.pixelFormat() == AV_PIX_FMT_P016)
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

    auto &vkImage = m_images[id];

    if (!vkImage)
    {
        VADRMPRIMESurfaceDescriptor vaSurfaceDescr = {};
        const bool exported = vaExportSurfaceHandle(
            m_vaapi->VADisp,
            id,
            VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
            VA_EXPORT_SURFACE_READ_ONLY | VA_EXPORT_SURFACE_SEPARATE_LAYERS,
            &vaSurfaceDescr
        ) == VA_STATUS_SUCCESS;

        if (exported)
        {
            bool isLinear = true;

            MemoryObject::FdDescriptors fdDescriptors(vaSurfaceDescr.num_objects);
            for (uint32_t i = 0; i < vaSurfaceDescr.num_objects; ++i)
            {
                if (i == 0 && vaSurfaceDescr.objects[i].drm_format_modifier != 0)
                    isLinear = false;

                fdDescriptors[i].first = vaSurfaceDescr.objects[i].fd;
                fdDescriptors[i].second = (vaSurfaceDescr.objects[i].size > 0)
                    ? vaSurfaceDescr.objects[i].size
                    : ::lseek(vaSurfaceDescr.objects[i].fd, 0, SEEK_END)
                ;
            }

            vector<vk::DeviceSize> offsets(vaSurfaceDescr.num_layers);
            for (uint32_t i = 0; i < vaSurfaceDescr.num_layers; ++i)
                offsets[i] = vaSurfaceDescr.layers[i].offset[0];

            try
            {
                vkImage = Image::createExternalImport(
                    device,
                    vk::Extent2D(frame.width(), frame.height()),
                    format,
                    isLinear
                );

                auto fdCustomData = make_unique<FDCustomData>();
                for (auto &&fdDescriptor : fdDescriptors)
                    fdCustomData->fds.push_back(::dup(fdDescriptor.first));
                vkImage->setCustomData(move(fdCustomData));

                vkImage->importFD(
                    fdDescriptors,
                    offsets,
                    vk::ExternalMemoryHandleTypeFlagBits::eDmaBufEXT
                );
            }
            catch (const vk::SystemError &e)
            {
                QMPlay2Core.logError(QString("VA-API :: %1").arg(e.what()));
                vkImage.reset();
                for (uint32_t o = 0; o < vaSurfaceDescr.num_objects; ++o)
                    ::close(vaSurfaceDescr.objects[o].fd);
            }
        }

        if (!vkImage)
            m_error = true;
    }

    if (vkImage)
    {
        frame.setVulkanImage(vkImage);
        frame.setOnDestroyFn([vaapi = m_vaapi] {
        });
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

        const auto &fds = image->customData<FDCustomData>()->fds;
        for (auto &&fd : fds)
        {
            const dma_buf_sync sync = {
                DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW
            };
            ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
        }
        for (auto &&fd : fds)
        {
            const dma_buf_sync sync = {
                DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW
            };
            ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
        }
    }

    return nullptr;
}

void VAAPIVulkan::insertAvailableSurface(uintptr_t id)
{
    lock_guard<mutex> locker(m_mutex);
    m_availableSurfaces.insert(id);
}
