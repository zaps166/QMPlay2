/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <CuvidVulkan.hpp>
#include <QMPlay2Core.hpp>
#include <Frame.hpp>

#include "../qmvk/PhysicalDevice.hpp"
#include "../qmvk/Device.hpp"
#include "../qmvk/Image.hpp"
#include "../qmvk/Semaphore.hpp"

#include <vulkan/VulkanInstance.hpp>
#include <vulkan/VulkanImagePool.hpp>

#include <QDebug>

#ifndef VK_USE_PLATFORM_WIN32_KHR
#   include <unistd.h>
#endif

using namespace QmVk;

class CudaCustomData : public MemoryObjectBase::CustomData
{
public:
    CudaCustomData(const shared_ptr<CUcontext> &cuCtx)
        : m_cuCtx(cuCtx)
    {}
    ~CudaCustomData()
    {
        cu::ContextGuard cuCtxGuard(m_cuCtx);
        cu::memFree(devPtr);
        cu::destroyExternalMemory(memory);
#ifdef VK_USE_PLATFORM_WIN32_KHR
        CloseHandle(handle);
#endif
    }

private:
    const shared_ptr<CUcontext> m_cuCtx;

public:
    CUexternalMemory memory = nullptr;
    CUdeviceptr devPtr = 0;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    HANDLE handle = nullptr;
#endif
};

class CudaSyncData : public HWInterop::SyncData
{
public:
    shared_ptr<Semaphore> semaphore;

    vector<vk::Semaphore> waitSemaphores;
    vector<vk::PipelineStageFlags> waitDstStageMask;
};

/**/

CuvidVulkan::CuvidVulkan(const shared_ptr<CUcontext> &cuCtx)
    : CuvidHWInterop(cuCtx)
    , m_vkImagePool(static_pointer_cast<Instance>(QMPlay2Core.gpuInstance())->createImagePool())
{
    auto physicalDevice = m_vkImagePool->instance()->physicalDevice();

    if (!physicalDevice->hasPciBusInfo() || !cu::deviceGetPCIBusId)
    {
        if (physicalDevice->properties().vendorID != 0x10de /* NVIDIA */)
        {
            QMPlay2Core.logError("CUVID :: Not an NVIDIA device");
            m_error = true;
            return;
        }
    }
    else
    {
        char cuPCIBusId[13] = {};
        if (cu::deviceGetPCIBusId(cuPCIBusId, sizeof(cuPCIBusId), 0) == CUDA_SUCCESS)
        {
            if (QString::fromStdString(physicalDevice->linuxPCIPath()).compare(cuPCIBusId, Qt::CaseInsensitive) != 0)
            {
                QMPlay2Core.logError("CUVID :: Primary CUDA device doesn't match");
                m_error = true;
                return;
            }
        }
    }

    auto cantInteroperateError = [this] {
        QMPlay2Core.logError("CUVID :: Can't interoperate with Vulkan");
        m_error = true;
    };

    if (!physicalDevice->checkExtensions({
        VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
        VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
#else
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
#endif
    })) {
        cantInteroperateError();
        return;
    }

    auto isMemoryExportable = [&] {
        try
        {
            const auto externalMemoryFeatures = Image::getExternalMemoryProperties(
                physicalDevice,
                m_vkMemType,
                vk::Format::eR8Unorm,
                true
            ).externalMemoryFeatures;
            if (externalMemoryFeatures & vk::ExternalMemoryFeatureFlagBits::eExportable)
                return true;
        }
        catch (const vk::SystemError &e)
        {}
        return false;
    };

#ifdef VK_USE_PLATFORM_WIN32_KHR
    m_vkMemType = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
    m_cuMemType = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32;

    m_vkSemHandleType = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32;
    m_cuSemHandleType = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32;

    if (!isMemoryExportable())
    {
        m_vkMemType = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32Kmt;
        m_cuMemType = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT;

        m_vkSemHandleType = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32Kmt;
        m_cuSemHandleType = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT;

        if (!physicalDevice->checkExtension(VK_KHR_WIN32_KEYED_MUTEX_EXTENSION_NAME) || !isMemoryExportable())
        {
            cantInteroperateError();
            return;
        }
    }
#else
    m_vkMemType = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
    m_cuMemType = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD;

    m_vkSemHandleType = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd;
    m_cuSemHandleType = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD;

    if (!isMemoryExportable())
    {
        cantInteroperateError();
        return;
    }
#endif

    if (cu::streamCreate(&m_cuStream, CU_STREAM_DEFAULT) != CUDA_SUCCESS)
        m_error = true;
}
CuvidVulkan::~CuvidVulkan()
{
    destroySemaphore();
    cu::streamDestroy(m_cuStream);
}

QString CuvidVulkan::name() const
{
    return "CUVID";
}

void CuvidVulkan::map(Frame &frame) try
{
    if (m_error || frame.vulkanImage())
        return;

    cu::ContextGuard cuCtxGuard(m_cuCtx);

    const int pictureIndex = frame.customData();

    if (m_validPictures.count(pictureIndex) == 0)
        return;

    auto img = m_vkImagePool->assignLinearDeviceLocalExport(
        frame,
        m_vkMemType
    );
    if (!img)
    {
        m_error = true;
        return;
    }

    ensureSemaphore();

    auto cudaCustomData = img->customData<CudaCustomData>();
    if (!cudaCustomData)
    {
        auto cudaCustomDataUnique = make_unique<CudaCustomData>(m_cuCtx);

        CUDA_EXTERNAL_MEMORY_HANDLE_DESC externalMemHandleDesc = {};
        externalMemHandleDesc.type = m_cuMemType;

#ifdef VK_USE_PLATFORM_WIN32_KHR
        // Ownership is not transferred to the CUDA driver
        cudaCustomDataUnique->handle = img->exportMemoryWin32(m_vkMemType);
        externalMemHandleDesc.handle.win32.handle = cudaCustomDataUnique->handle;
#else
        // Ownership is transferred to the CUDA driver
        externalMemHandleDesc.handle.fd = img->exportMemoryFd(m_vkMemType);
#endif

        externalMemHandleDesc.size = img->memorySize();
        if (cu::importExternalMemory(&cudaCustomDataUnique->memory, &externalMemHandleDesc) != CUDA_SUCCESS)
        {
            QMPlay2Core.logError("CUVID :: Unable to import external memory");
#ifndef VK_USE_PLATFORM_WIN32_KHR
            ::close(externalMemHandleDesc.handle.fd);
#endif
            m_error = true;
            return;
        }

        CUDA_EXTERNAL_MEMORY_BUFFER_DESC externalMemBufferDesc = {};
        externalMemBufferDesc.size = img->memorySize();
        if (cu::externalMemoryGetMappedBuffer(&cudaCustomDataUnique->devPtr, cudaCustomDataUnique->memory, &externalMemBufferDesc) != CUDA_SUCCESS)
        {
            m_error = true;
            return;
        }

        cudaCustomData = cudaCustomDataUnique.get();
        img->setCustomData(move(cudaCustomDataUnique));
    }

    quintptr mappedFrame = 0;
    unsigned pitch = 0;

    CUVIDPROCPARAMS vidProcParams = {};
    vidProcParams.top_field_first = frame.isTopFieldFirst();
    if (frame.isInterlaced())
        vidProcParams.second_field = frame.isSecondField();
    else
        vidProcParams.progressive_frame = true;
    if (cuvid::mapVideoFrame(m_cuvidDec, pictureIndex, &mappedFrame, &pitch, &vidProcParams) != CUDA_SUCCESS)
    {
        m_error = true;
        return;
    }

    bool copied = true;

    CUDA_MEMCPY2D cpy = {};
    cpy.srcMemoryType = CU_MEMORYTYPE_DEVICE;
    cpy.dstMemoryType = CU_MEMORYTYPE_DEVICE;
    cpy.srcDevice = mappedFrame;
    cpy.srcPitch = pitch;
    cpy.WidthInBytes = frame.width();
    if (frame.pixelFormat() == AV_PIX_FMT_P016)
        cpy.WidthInBytes *= 2;
    for (int p = 0; p < 2; ++p)
    {
        cpy.srcY = p ? m_codedHeight : 0;
        cpy.dstDevice = cudaCustomData->devPtr;
        cpy.dstPitch = img->linesize();
        cpy.Height = frame.height(p);
        cpy.dstY = p ? img->size().height : 0;

        if (cu::memcpy2DAsync(&cpy, m_cuStream) != CUDA_SUCCESS)
        {
            copied = false;
            break;
        }
    }

    if (cuvid::unmapVideoFrame(m_cuvidDec, mappedFrame) != CUDA_SUCCESS || !copied)
    {
        m_error = true;
    }
    else
    {
        lock_guard<mutex> locker(m_picturesToSyncMutex);
        m_picturesToSync.insert(pictureIndex);
    }
} catch (const vk::SystemError &e) {
    QMPlay2Core.logError(QString("CUVID :: %1").arg(e.what()));
    m_error = true;
}
void CuvidVulkan::clear()
{
    lock_guard<mutex> locker(m_picturesToSyncMutex);
    m_picturesToSync.clear();
}

HWInterop::SyncDataPtr CuvidVulkan::sync(const vector<Frame> &frames, vk::SubmitInfo *submitInfo)
{
    unique_lock<mutex> locker(m_picturesToSyncMutex);
    bool needSync = false;
    for (auto &&frame : frames)
    {
        if (m_picturesToSync.erase(frame.customData()) > 0)
            needSync = true;
    }
    if (!needSync)
    {
        return nullptr;
    }
    locker.unlock();

    CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS signalParams = {};
    if (cu::signalExternalSemaphoresAsync(&m_cuSemaphore, &signalParams, 1, m_cuStream) != CUDA_SUCCESS)
    {
        m_error = true;
        return nullptr;
    }

    unique_ptr<vk::SubmitInfo> submitInfoPtr;
    if (!submitInfo)
    {
        submitInfoPtr = make_unique<vk::SubmitInfo>();
        submitInfo = submitInfoPtr.get();
    }

    auto syncData = make_unique<CudaSyncData>();

    syncData->semaphore = m_semaphore;

    for (uint32_t i = 0; i < submitInfo->waitSemaphoreCount; ++i)
        syncData->waitSemaphores.push_back(submitInfo->pWaitSemaphores[i]);
    syncData->waitSemaphores.push_back(*m_semaphore);

    for (uint32_t i = 0; i < submitInfo->waitSemaphoreCount; ++i)
        syncData->waitDstStageMask.push_back(submitInfo->pWaitDstStageMask[i]);
    syncData->waitDstStageMask.push_back(vk::PipelineStageFlagBits::eAllCommands);

    submitInfo->waitSemaphoreCount = syncData->waitSemaphores.size();
    submitInfo->pWaitSemaphores = syncData->waitSemaphores.data();
    submitInfo->pWaitDstStageMask = syncData->waitDstStageMask.data();

    if (!submitInfoPtr)
        return syncData;

    if (!syncNow(*submitInfo))
        m_error = true;

    return nullptr;
}

void CuvidVulkan::ensureSemaphore()
{
    auto device = m_vkImagePool->instance()->device();
    if (m_semaphore && m_semaphore->device() == device)
        return;

    Q_ASSERT(device);

    destroySemaphore();

    m_semaphore = Semaphore::createExport(device, m_vkSemHandleType);
#ifdef VK_USE_PLATFORM_WIN32_KHR
    m_semaphoreHandle = m_semaphore->exportWin32Handle();
#else
    m_semaphoreHandle = m_semaphore->exportFD();
#endif

    CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC cuExternalSemaphoreHandleDesc = {};
    cuExternalSemaphoreHandleDesc.type = m_cuSemHandleType;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    // Ownership is not transferred to the CUDA driver
    cuExternalSemaphoreHandleDesc.handle.win32.handle = m_semaphoreHandle;
#else
    // Ownership is transferred to the CUDA driver
    cuExternalSemaphoreHandleDesc.handle.fd = m_semaphoreHandle;
#endif
    if (cu::importExternalSemaphore(&m_cuSemaphore, &cuExternalSemaphoreHandleDesc) != CUDA_SUCCESS)
    {
        destroySemaphore();
        throw vk::InitializationFailedError("Can't import Vulkan semaphore");
    }
#ifndef VK_USE_PLATFORM_WIN32_KHR
    else
    {
        m_semaphoreHandle = -1;
    }
#endif
}
void CuvidVulkan::destroySemaphore()
{
    cu::destroyExternalSemaphore(m_cuSemaphore);
    m_cuSemaphore = nullptr;

#ifdef VK_USE_PLATFORM_WIN32_KHR
    if (m_semaphoreHandle)
    {
        CloseHandle(m_semaphoreHandle);
        m_semaphoreHandle = nullptr;
    }
#else
    if (m_semaphoreHandle != -1)
    {
        ::close(m_semaphoreHandle);
        m_semaphoreHandle = -1;
    }
#endif

    m_semaphore.reset();
}
