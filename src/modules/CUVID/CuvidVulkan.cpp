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

#include <CuvidVulkan.hpp>
#include <QMPlay2Core.hpp>
#include <Frame.hpp>

#include "../qmvk/PhysicalDevice.hpp"
#include "../qmvk/Image.hpp"

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

/**/

CuvidVulkan::CuvidVulkan(const shared_ptr<CUcontext> &cuCtx)
    : CuvidHWInterop(cuCtx)
    , m_vkImagePool(static_pointer_cast<Instance>(QMPlay2Core.gpuInstance())->createImagePool())
{
    m_error = !m_vkImagePool->instance()->physicalDevice()->checkExtensions({
        VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
        VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME
#else
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
#endif
    });
    if (!m_error && cu::streamCreate(&m_cuStream, CU_STREAM_NON_BLOCKING) != CUDA_SUCCESS)
        m_error = true;
}
CuvidVulkan::~CuvidVulkan()
{
    cu::streamDestroy(m_cuStream);
}

QString CuvidVulkan::name() const
{
    return "CUVID";
}

void CuvidVulkan::map(Frame &frame)
{
    if (frame.vulkanImage())
        return;

    cu::ContextGuard cuCtxGuard(m_cuCtx);

#ifdef VK_USE_PLATFORM_WIN32_KHR
    constexpr auto vkMemType = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
    constexpr auto cuMemType = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32;
#else
    constexpr auto vkMemType = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
    constexpr auto cuMemType = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD;
#endif

    const int pictureIndex = frame.customData();

    if (m_validPictures.count(pictureIndex) == 0)
        return;

    auto img = m_vkImagePool->assignLinearDeviceLocalExport(
        frame,
        vkMemType
    );
    if (!img)
    {
        m_error = true;
        return;
    }

    auto cudaCustomData = img->customData<CudaCustomData>();
    if (!cudaCustomData)
    {
        auto cudaCustomDataUnique = make_unique<CudaCustomData>(m_cuCtx);

        CUDA_EXTERNAL_MEMORY_HANDLE_DESC externalMemHandleDesc = {};
        externalMemHandleDesc.type = cuMemType;
        try
        {
#ifdef VK_USE_PLATFORM_WIN32_KHR
            // Ownership is not transferred to the CUDA driver
            cudaCustomDataUnique->handle = img->exportMemoryWin32(vkMemType);
            externalMemHandleDesc.handle.win32.handle = cudaCustomDataUnique->handle;
#else
            // Ownership is transferred to the CUDA driver
            externalMemHandleDesc.handle.fd = img->exportMemoryFd(vkMemType);
#endif
        }
        catch (const vk::SystemError &e)
        {
            Q_UNUSED(e)
            m_error = true;
            return;
        }
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
        cu::streamSynchronize(m_cuStream);
    }
}
void CuvidVulkan::clear()
{
}
