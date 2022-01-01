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

#pragma once

#include <vulkan/VulkanHWInterop.hpp>

#include <unordered_map>
#include <mutex>

#include <wrl/client.h>
#include <d3d11.h>

using Microsoft::WRL::ComPtr;

namespace QmVk {
class Instance;
class Device;
class Image;
}

struct AVD3D11VADeviceContext;
struct AVBufferRef;

class D3D11VAVulkan : public QmVk::HWInterop
{
    struct D3D11Frame;

public:
    D3D11VAVulkan(AVBufferRef *hwDeviceBufferRef, bool zeroCopyAllowed);
    ~D3D11VAVulkan();

    QString name() const override;

    void map(Frame &frame) override;
    void clear() override;

    SyncDataPtr sync(const std::vector<Frame> &frames, vk::SubmitInfo *submitInfo) override;

public:
    bool init();

    inline bool isZeroCopy() const
    {
        return m_zeroCopy;
    }
    inline bool hasKMT() const
    {
        return m_hasKMT;
    }

private:
    ComPtr<ID3D11UnorderedAccessView> createUAV(DXGI_FORMAT format, UINT firstElement, UINT nElements, ID3D11Buffer *buffer);
    ComPtr<ID3D11UnorderedAccessView> createUAV(DXGI_FORMAT format, ID3D11Texture2D *texture);
    ComPtr<ID3D11ShaderResourceView> createSRV(DXGI_FORMAT format, UINT idx, ID3D11Texture2D *texture);
    ComPtr<ID3D11Texture2D> createTexture(UINT width, UINT height, DXGI_FORMAT format);
    ComPtr<ID3D11Buffer> createBuffer(UINT size);

    bool getHandle(ID3D11Resource *resource, HANDLE &handle, ComPtr<IDXGIKeyedMutex> &keyedMutex);

    UINT getMiscFlags();

    static inline UINT getThreadGroupCountX(int w);
    static inline UINT getThreadGroupCountY(int h);

    bool createImageInterop(const std::shared_ptr<QmVk::Device> &device, Frame &frame, std::shared_ptr<QmVk::Image> &image);
    bool createImageInteropZeroCopy(const std::shared_ptr<QmVk::Device> &device, Frame &frame, std::shared_ptr<QmVk::Image> &image);
    bool createImageInteropLinear(const std::shared_ptr<QmVk::Device> &device, Frame &frame, std::shared_ptr<QmVk::Image> &image, D3D11Frame *d3d11Frame);
    bool createImageInteropTexture(const std::shared_ptr<QmVk::Device> &device, Frame &frame, std::shared_ptr<QmVk::Image> &image, D3D11Frame *d3d11Frame);

    bool setConstData(ID3D11Buffer *buffer, uint32_t value);

public:
    AVBufferRef *m_hwDeviceBufferRef = nullptr;

private:
    const std::shared_ptr<QmVk::Instance> m_vkInstance;

    const AVD3D11VADeviceContext *const m_devCtx;

    bool m_zeroCopy = false;
    bool m_hasKMT = false;
    bool m_linearImage = false;

    vk::ExternalMemoryHandleTypeFlagBits m_externalMemoryHandleType = vk::ExternalMemoryHandleTypeFlagBits::eD3D11Texture;

    std::mutex m_mutex;

    ComPtr<ID3D11Buffer> m_constData1;
    ComPtr<ID3D11Buffer> m_constData2;
    ComPtr<ID3D11ComputeShader> m_cs;

    int m_lastLinesize1 = -1;
    int m_lastLinesize2 = -1;

    std::unordered_map<UINT, std::shared_ptr<QmVk::Image>> m_images;

    DXGI_FORMAT m_format1 = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT m_format2 = DXGI_FORMAT_UNKNOWN;
    vk::Format m_formatVk = vk::Format::eUndefined;
    vk::DeviceSize m_nElementsDivider = 1;
};
