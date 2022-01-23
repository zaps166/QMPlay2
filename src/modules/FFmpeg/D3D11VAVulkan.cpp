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

#include <D3D11VAVulkan.hpp>

#include <QResource>
#include <QLibrary>
#include <QDebug>
#include <QDir>

#include <QMPlay2Core.hpp>
#include <GPUInstance.hpp>
#include <Frame.hpp>

#include "../../qmvk/PhysicalDevice.hpp"
#include "../../qmvk/Image.hpp"

#include <vulkan/VulkanInstance.hpp>

#include <d3dcompiler.h>
#include <dxgi1_2.h>

extern "C"
{
    #include <libavutil/hwcontext.h>
    #include <libavutil/hwcontext_d3d11va.h>
}

using namespace QmVk;

struct D3D11VAVulkan::D3D11Frame : public MemoryObjectBase::CustomData
{
    ~D3D11Frame()
    {
        if (handle2)
            CloseHandle(handle2);
        if (handle1)
            CloseHandle(handle1);
    }

    ComPtr<ID3D11UnorderedAccessView> uav1;
    ComPtr<ID3D11UnorderedAccessView> uav2;

    ComPtr<IDXGIKeyedMutex> keyedMutex1;
    ComPtr<IDXGIKeyedMutex> keyedMutex2;

    ComPtr<ID3D11Query> query;

    HANDLE handle1 = nullptr;
    HANDLE handle2 = nullptr;

    bool synced = false;
};

class D3D11SyncData : public HWInterop::SyncData
{
public:
    vk::Win32KeyedMutexAcquireReleaseInfoKHR keyedMutexInfo;
    vector<vk::DeviceMemory> deviceMemories;
    vector<uint64_t> keys;
    vector<uint32_t> timeouts;
};

/**/

D3D11VAVulkan::D3D11VAVulkan(AVBufferRef *hwDeviceBufferRef, bool zeroCopyAllowed)
    : m_hwDeviceBufferRef(av_buffer_ref(hwDeviceBufferRef))
    , m_vkInstance(static_pointer_cast<Instance>(QMPlay2Core.gpuInstance()))
    , m_devCtx((AVD3D11VADeviceContext *)((AVHWDeviceContext *)m_hwDeviceBufferRef->data)->hwctx)
{
    auto physicalDevice = static_pointer_cast<Instance>(QMPlay2Core.gpuInstance())->physicalDevice();

    switch (physicalDevice->properties().vendorID)
    {
        case 0x8086: // Intel
            // Direct mapping of tiled decoded image via SHARED_NTHANDLE is possible here.
            // No synchronization is done, but displayed frame is the previously decoded
            // frame - this should be enough.
            if (zeroCopyAllowed)
                m_zeroCopy = true;
            break;
        case 0x1002: // AMD
        case 0x10de: // NVIDIA
            // Mapping tiled images (especially small tiled images) doesn't work properly here,
            // so copy the image into a linear buffer and then map it as a linear image.
            m_linearImage = true;
            break;
    }

    auto isMemoryImportable = [&](vk::ExternalMemoryHandleTypeFlagBits externalMemoryHandleType) {
        try
        {
            const auto externalMemoryFeatures = Image::getExternalMemoryProperties(
                physicalDevice,
                externalMemoryHandleType,
                vk::Format::eR8Unorm,
                m_linearImage
            ).externalMemoryFeatures;
            if (externalMemoryFeatures & vk::ExternalMemoryFeatureFlagBits::eImportable)
                return true;
        }
        catch (const vk::SystemError &e)
        {}
        return false;
    };

    if (!m_zeroCopy && physicalDevice->checkExtension(VK_KHR_WIN32_KEYED_MUTEX_EXTENSION_NAME))
    {
        if (isMemoryImportable(vk::ExternalMemoryHandleTypeFlagBits::eD3D11TextureKmt))
        {
            m_hasKMT = true;
            m_externalMemoryHandleType = vk::ExternalMemoryHandleTypeFlagBits::eD3D11TextureKmt;
        }
    }

    if (!m_hasKMT && !isMemoryImportable(m_externalMemoryHandleType))
    {
        QMPlay2Core.logError("D3D11VA :: Can't interoperate with Vulkan");
        m_error = true;
        return;
    }

    if (m_zeroCopy)
        qDebug() << "D3D11VA :: Zero-copy mode";
    else
        qDebug() << "D3D11VA ::" << (m_linearImage ? "Linear" : "Tiled") << "mode" << (m_hasKMT ? "KMT" : "");
}
D3D11VAVulkan::~D3D11VAVulkan()
{
    m_images.clear();
    av_buffer_unref(&m_hwDeviceBufferRef);
}

QString D3D11VAVulkan::name() const
{
    return "D3D11VA";
}

void D3D11VAVulkan::map(Frame &frame)
{
    if (m_error || frame.vulkanImage() || !frame.isHW())
        return;

    m_formatVk = (frame.pixelFormat() == AV_PIX_FMT_P016)
        ? vk::Format::eG16B16R162Plane420Unorm
        : vk::Format::eG8B8R82Plane420Unorm
    ;

    unique_lock<mutex> locker;
    if (!m_hasKMT)
        locker = unique_lock<mutex>(m_mutex);

    auto device = m_vkInstance->device();

    if (!m_images.empty())
    {
        auto image = m_images.begin()->second.get();
        if (image->device() != device || image->format() != m_formatVk)
            clear();
    }

    if (!device)
        return;

    if (m_images.empty())
    {
        if (frame.pixelFormat() == AV_PIX_FMT_P016)
        {
            m_format1 = DXGI_FORMAT_R16_UNORM;
            m_format2 = DXGI_FORMAT_R16G16_UNORM;
            m_nElementsDivider = 2;
        }
        else
        {
            m_format1 = DXGI_FORMAT_R8_UNORM;
            m_format2 = DXGI_FORMAT_R8G8_UNORM;
            m_nElementsDivider = 1;
        }
    }

    const UINT textureIdx = frame.hwData(1);
    auto &image = m_images[textureIdx];

    if (!image && !createImageInterop(device, frame, image))
    {
        m_error = true;
        return;
    }

    frame.setVulkanImage(image);

    if (m_zeroCopy)
        return;

    auto d3d11Frame = image->customData<D3D11Frame>();

    const auto texture = reinterpret_cast<ID3D11Texture2D *>(frame.hwData(0));
    auto srv1 = createSRV(m_format1, textureIdx, texture);
    auto srv2 = createSRV(m_format2, textureIdx, texture);
    if (!srv1 || !srv2)
    {
        m_error = true;
        return;
    }

    if (m_constData1 && m_constData2)
    {
        const int linesize1 = image->linesize(0) / m_nElementsDivider;
        const int linesize2 = image->linesize(1) / m_nElementsDivider;
        if (m_lastLinesize1 != linesize1 && !setConstData(m_constData1.Get(), linesize1))
        {
            m_error = true;
            return;
        }
        if (m_lastLinesize2 != linesize2 && !setConstData(m_constData2.Get(), linesize2 / 2))
        {
            m_error = true;
            return;
        }
        m_lastLinesize1 = linesize1;
        m_lastLinesize2 = linesize2;
    }

    if (d3d11Frame->keyedMutex1)
        d3d11Frame->keyedMutex1->AcquireSync(0, INFINITE);
    if (d3d11Frame->keyedMutex2)
        d3d11Frame->keyedMutex2->AcquireSync(0, INFINITE);

    m_devCtx->device_context->CSSetShader(m_cs.Get(), nullptr, 0);

    m_devCtx->device_context->CSSetShaderResources(0, 1, srv1.GetAddressOf());
    m_devCtx->device_context->CSSetUnorderedAccessViews(0, 1, d3d11Frame->uav1.GetAddressOf(), nullptr);
    if (m_constData1)
        m_devCtx->device_context->CSSetConstantBuffers(0, 1, m_constData1.GetAddressOf());
    m_devCtx->device_context->Dispatch(getThreadGroupCountX(frame.width(0)), getThreadGroupCountY(frame.height(0)), 1);

    m_devCtx->device_context->CSSetShaderResources(0, 1, srv2.GetAddressOf());
    m_devCtx->device_context->CSSetUnorderedAccessViews(0, 1, d3d11Frame->uav2.GetAddressOf(), nullptr);
    if (m_constData2)
        m_devCtx->device_context->CSSetConstantBuffers(0, 1, m_constData2.GetAddressOf());
    m_devCtx->device_context->Dispatch(getThreadGroupCountX(frame.width(1)), getThreadGroupCountY(frame.height(1)), 1);

    if (d3d11Frame->keyedMutex1)
        d3d11Frame->keyedMutex1->ReleaseSync(0);
    if (d3d11Frame->keyedMutex2)
        d3d11Frame->keyedMutex2->ReleaseSync(0);

    if (d3d11Frame->query)
    {
        m_devCtx->device_context->End(d3d11Frame->query.Get());
        m_devCtx->device_context->Flush();
    }

    d3d11Frame->synced = false;
}
void D3D11VAVulkan::clear()
{
    unique_lock<mutex> locker;
    if (!m_hasKMT)
        locker = unique_lock<mutex>(m_mutex);
    m_images.clear();
}

HWInterop::SyncDataPtr D3D11VAVulkan::sync(
    const vector<Frame> &frames,
    vk::SubmitInfo *submitInfo)
{
    unique_ptr<D3D11SyncData> syncData;

    for (auto &&frame : frames)
    {
        auto image = frame.vulkanImage();
        if (!image)
            continue;

        auto d3d11frame = image->customData<D3D11Frame>();
        if (!d3d11frame)
            continue;

        if (d3d11frame->synced)
            continue;

        if (m_hasKMT)
        {
            const uint32_t n = image->deviceMemoryCount();
            if (!syncData)
            {
                const size_t capacity = frames.size() * n;
                syncData = make_unique<D3D11SyncData>();
                syncData->deviceMemories.reserve(capacity);
                syncData->keys.reserve(capacity);
                syncData->timeouts.reserve(capacity);
            }
            for (uint32_t i = 0; i < n; ++i)
            {
                syncData->deviceMemories.push_back(image->deviceMemory(i));
                syncData->keys.push_back(0);
                syncData->timeouts.push_back(INFINITE);
            }
        }
        else
        {
            BOOL queryData = FALSE;
            while (m_devCtx->device_context->GetData(d3d11frame->query.Get(), &queryData, sizeof(queryData), 0) == S_FALSE && queryData == FALSE)
            {
                Sleep(1);
            }
        }

        d3d11frame->synced = true;
    }

    if (!syncData)
        return nullptr;

    syncData->keyedMutexInfo.acquireCount = syncData->keys.size();
    syncData->keyedMutexInfo.pAcquireSyncs = syncData->deviceMemories.data();
    syncData->keyedMutexInfo.pAcquireKeys = syncData->keys.data();
    syncData->keyedMutexInfo.pAcquireTimeouts = syncData->timeouts.data();
    syncData->keyedMutexInfo.releaseCount = syncData->keys.size();
    syncData->keyedMutexInfo.pReleaseSyncs = syncData->deviceMemories.data();
    syncData->keyedMutexInfo.pReleaseKeys = syncData->keys.data();

    unique_ptr<vk::SubmitInfo> submitInfoPtr;
    if (!submitInfo)
    {
        submitInfoPtr = make_unique<vk::SubmitInfo>();
        submitInfo = submitInfoPtr.get();
    }

    submitInfo->pNext = &syncData->keyedMutexInfo;

    if (!submitInfoPtr)
        return syncData;

    if (!syncNow(*submitInfo))
        m_error = true;

    return nullptr;
}

bool D3D11VAVulkan::init()
{
    if (m_error)
        return false;

    if (m_zeroCopy)
        return true;

    static auto d3dCompile = [] {
        pD3DCompile d3dCompile = nullptr;
        const auto files = QDir("C:\\Windows\\System32").entryInfoList({"D3DCompiler_*.dll"}, QDir::Files, QDir::Name);
        for (int i = files.size() - 1; i >= 0; --i)
        {
            QLibrary lib(files[i].filePath());
            if (!lib.load())
                continue;

            d3dCompile = reinterpret_cast<pD3DCompile>(lib.resolve("D3DCompile"));
            if (d3dCompile)
                break;
        }
        return d3dCompile;
    }();
    if (!d3dCompile)
        return false;

    if (m_linearImage)
    {
        D3D11_BUFFER_DESC constDataDesc = {};
        constDataDesc.ByteWidth = FFALIGN(sizeof(uint32_t), 16);
        constDataDesc.Usage = D3D11_USAGE_DYNAMIC;
        constDataDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        constDataDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        m_devCtx->device->CreateBuffer(&constDataDesc, nullptr, m_constData1.GetAddressOf());
        m_devCtx->device->CreateBuffer(&constDataDesc, nullptr, m_constData2.GetAddressOf());
        if (!m_constData1 || !m_constData2)
            return false;
    }

    const QResource res(m_linearImage ? ":/D3D11VABuffer.dxcs" : ":/D3D11VATexture.dxcs");
    auto dxcs = QByteArray::fromRawData(reinterpret_cast<const char *>(res.data()), res.size());

    ComPtr<ID3DBlob> csData;
    d3dCompile(
        dxcs.constData(),
        dxcs.size(),
        nullptr,
        nullptr,
        nullptr,
        "main",
        "cs_5_0",
        0,
        0,
        csData.GetAddressOf(),
        nullptr
    );
    if (!csData)
        return false;

    m_devCtx->device->CreateComputeShader(
        csData->GetBufferPointer(),
        csData->GetBufferSize(),
        nullptr,
        m_cs.GetAddressOf()
    );
    if (!m_cs)
        return false;

    return true;
}

ComPtr<ID3D11UnorderedAccessView> D3D11VAVulkan::createUAV(DXGI_FORMAT format, UINT firstElement, UINT nElements, ID3D11Buffer *buffer)
{
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = format;
    uavDesc.Buffer.FirstElement = firstElement;
    uavDesc.Buffer.NumElements = nElements;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    ComPtr<ID3D11UnorderedAccessView> uav;
    m_devCtx->device->CreateUnorderedAccessView(buffer, &uavDesc, uav.GetAddressOf());
    return uav;
}
ComPtr<ID3D11UnorderedAccessView> D3D11VAVulkan::createUAV(DXGI_FORMAT format, ID3D11Texture2D *texture)
{
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = format;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    ComPtr<ID3D11UnorderedAccessView> uav;
    m_devCtx->device->CreateUnorderedAccessView(texture, &uavDesc, uav.GetAddressOf());
    return uav;
}
ComPtr<ID3D11ShaderResourceView> D3D11VAVulkan::createSRV(DXGI_FORMAT format, UINT idx, ID3D11Texture2D *texture)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MipLevels = 1;
    srvDesc.Texture2DArray.FirstArraySlice = idx;
    srvDesc.Texture2DArray.ArraySize = 1;
    ComPtr<ID3D11ShaderResourceView> srv;
    m_devCtx->device->CreateShaderResourceView(texture, &srvDesc, srv.GetAddressOf());
    return srv;
}
ComPtr<ID3D11Texture2D> D3D11VAVulkan::createTexture(UINT width, UINT height, DXGI_FORMAT format)
{
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.Format = format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    textureDesc.MiscFlags = getMiscFlags();
    ComPtr<ID3D11Texture2D> texture;
    m_devCtx->device->CreateTexture2D(&textureDesc, nullptr, texture.GetAddressOf());
    return texture;
}
ComPtr<ID3D11Buffer> D3D11VAVulkan::createBuffer(UINT size)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = size;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.MiscFlags = getMiscFlags();
    ComPtr<ID3D11Buffer> buffer;
    m_devCtx->device->CreateBuffer(&bufferDesc, nullptr, buffer.GetAddressOf());
    return buffer;
}

bool D3D11VAVulkan::getHandle(ID3D11Resource *resource, HANDLE &handle, ComPtr<IDXGIKeyedMutex> &keyedMutex)
{
    if (m_hasKMT)
    {
        ComPtr<IDXGIResource> dxgiResource;
        resource->QueryInterface(
            __uuidof(IDXGIResource),
            reinterpret_cast<void **>(dxgiResource.GetAddressOf())
        );
        if (!dxgiResource)
            return false;

        dxgiResource->QueryInterface(
            __uuidof(IDXGIKeyedMutex),
            reinterpret_cast<void **>(keyedMutex.GetAddressOf())
        );
        if (!keyedMutex)
            return false;

        dxgiResource->GetSharedHandle(&handle);
        if (!handle)
            return false;

        return true;
    }
    else
    {
        ComPtr<IDXGIResource1> dxgiResource;
        resource->QueryInterface(
            __uuidof(IDXGIResource1),
            reinterpret_cast<void **>(dxgiResource.GetAddressOf())
        );
        if (!dxgiResource)
            return false;

        dxgiResource->CreateSharedHandle(
            nullptr,
            DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE,
            nullptr,
            &handle
        );
        if (!handle)
            return false;

        return true;
    }
}

UINT D3D11VAVulkan::getMiscFlags()
{
    return m_hasKMT
        ? D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX
        : D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE
    ;
}

inline UINT D3D11VAVulkan::getThreadGroupCountX(int w)
{
    constexpr auto g_dispatchSizeX = 16;
    return (w + g_dispatchSizeX - 1) / g_dispatchSizeX;
}
inline UINT D3D11VAVulkan::getThreadGroupCountY(int h)
{
    constexpr auto g_dispatchSizeY = 16;
    return (h + g_dispatchSizeY - 1) / g_dispatchSizeY;
}

bool D3D11VAVulkan::createImageInterop(const shared_ptr<Device> &device, Frame &frame, shared_ptr<Image> &image) try
{
    vk::Extent2D imageSize;

    if (m_zeroCopy)
    {
        const auto texture = reinterpret_cast<ID3D11Texture2D *>(frame.hwData(0));
        D3D11_TEXTURE2D_DESC desc = {};
        texture->GetDesc(&desc);
        imageSize = vk::Extent2D(desc.Width, desc.Height);
    }
    else
    {
        imageSize = vk::Extent2D(frame.width(), frame.height());
    }

    image = Image::createExternalImport(
        device,
        imageSize,
        m_formatVk,
        m_linearImage,
        m_externalMemoryHandleType
    );

    if (m_zeroCopy)
        return createImageInteropZeroCopy(device, frame, image);

    auto d3d11Frame = make_unique<D3D11Frame>();

    const bool ok = m_linearImage
        ? createImageInteropLinear(device, frame, image, d3d11Frame.get())
        : createImageInteropTexture(device, frame, image, d3d11Frame.get())
    ;
    if (!ok)
        return false;

    if (!m_hasKMT)
    {
        D3D11_QUERY_DESC queryDesc = {};
        queryDesc.Query = D3D11_QUERY_EVENT;
        m_devCtx->device->CreateQuery(&queryDesc, d3d11Frame->query.GetAddressOf());
        if (!d3d11Frame->query)
            return false;
    }

    image->setCustomData(move(d3d11Frame));

    return true;
} catch (const vk::SystemError &e) {
    QMPlay2Core.logError(QString("D3D11VA :: %1").arg(e.what()));
    return false;
}
bool D3D11VAVulkan::createImageInteropZeroCopy(const shared_ptr<Device> &device, Frame &frame, shared_ptr<Image> &image)
{
    const auto texture = reinterpret_cast<ID3D11Texture2D *>(frame.hwData(0));
    const UINT textureIdx = frame.hwData(1);

    ComPtr<IDXGIResource1> dxgiResource;
    texture->QueryInterface(
        __uuidof(IDXGIResource1),
        reinterpret_cast<void **>(dxgiResource.GetAddressOf())
    );
    if (!dxgiResource)
    {
        m_error = true;
        return false;
    }

    HANDLE handle = nullptr;
    dxgiResource->CreateSharedHandle(
        nullptr,
        DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE,
        nullptr,
        &handle
    );
    if (!handle)
    {
        m_error = true;
        return false;
    }

    frame.setOnDestroyFn([handle] {
        CloseHandle(handle);
    });

    image->importWin32Handle(
        {
            handle,
        },
        {
            image->planeOffset(0),
            image->planeOffset(1),
        },
        m_externalMemoryHandleType,
        image->memorySize() * textureIdx
    );

    return true;
}
bool D3D11VAVulkan::createImageInteropLinear(const shared_ptr<Device> &device, Frame &frame, shared_ptr<Image> &image, D3D11Frame *d3d11Frame)
{
    auto buffer = createBuffer(image->memorySize());
    if (!buffer)
        return false;

    d3d11Frame->uav1 = createUAV(
        m_format1,
        image->planeOffset(0),
        image->memorySize(0) / m_nElementsDivider,
        buffer.Get()
    );
    if (!d3d11Frame->uav1)
        return false;

    d3d11Frame->uav2 = createUAV(
        m_format2,
        image->planeOffset(1) / 2 / m_nElementsDivider,
        image->memorySize(1) / 2 / m_nElementsDivider,
        buffer.Get()
    );
    if (!d3d11Frame->uav2)
        return false;

    if (!getHandle(buffer.Get(), d3d11Frame->handle1, d3d11Frame->keyedMutex1))
        return false;

    image->importWin32Handle(
        {
            d3d11Frame->handle1,
        },
        {
            image->planeOffset(0),
            image->planeOffset(1),
        },
        m_externalMemoryHandleType
    );

    return true;
}
bool D3D11VAVulkan::createImageInteropTexture(const shared_ptr<Device> &device, Frame &frame, shared_ptr<Image> &image, D3D11Frame *d3d11Frame)
{
    auto texture1 = createTexture(frame.width(0), frame.height(0), m_format1);
    if (!texture1)
        return false;

    auto texture2 = createTexture(frame.width(1), frame.height(1), m_format2);
    if (!texture2)
        return false;

    d3d11Frame->uav1 = createUAV(m_format1, texture1.Get());
    if (!d3d11Frame->uav1)
        return false;

    d3d11Frame->uav2 = createUAV(m_format2, texture2.Get());
    if (!d3d11Frame->uav2)
        return false;

    if (!getHandle(texture1.Get(), d3d11Frame->handle1, d3d11Frame->keyedMutex1))
        return false;

    if (!getHandle(texture2.Get(), d3d11Frame->handle2, d3d11Frame->keyedMutex2))
        return false;

    image->importWin32Handle(
        {
            d3d11Frame->handle1,
            d3d11Frame->handle2,
        },
        {
            0,
            0,
        },
        m_externalMemoryHandleType
    );

    return true;
}

bool D3D11VAVulkan::setConstData(ID3D11Buffer *buffer, uint32_t value)
{
    D3D11_MAPPED_SUBRESOURCE constMappedSubresource = {};
    if (m_devCtx->device_context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constMappedSubresource) != 0)
        return false;

    *reinterpret_cast<uint32_t *>(constMappedSubresource.pData) = value;
    m_devCtx->device_context->Unmap(buffer, 0);
    return true;
}
