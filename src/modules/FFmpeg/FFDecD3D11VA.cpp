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

#include <FFDecD3D11VA.hpp>
#include <D3D11VAVulkan.hpp>

#include <DeintHWPrepareFilter.hpp>
#include <GPUInstance.hpp>
#include <StreamInfo.hpp>

#include "../../qmvk/PhysicalDevice.hpp"

#include <vulkan/VulkanInstance.hpp>

#include <QLibrary>
#include <QDebug>

#include <dxgi1_2.h>

extern "C"
{
    #include <libavutil/hwcontext.h>
    #include <libavutil/hwcontext_d3d11va.h>
}

using namespace std;

constexpr auto g_delayedFrames = 1;

static AVPixelFormat d3d11vaGetFormat(AVCodecContext *codecCtx, const AVPixelFormat *pixFmt)
{
    while (*pixFmt != AV_PIX_FMT_NONE)
    {
        if (*pixFmt == AV_PIX_FMT_D3D11)
        {
            auto hwFrameBufferRef = av_hwframe_ctx_alloc(codecCtx->hw_device_ctx);

            auto framesCtx = reinterpret_cast<AVHWFramesContext *>(hwFrameBufferRef->data);

            // From FFmpeg code
            framesCtx->initial_pool_size = 1;
            if (codecCtx->codec_id == AV_CODEC_ID_H264 || codecCtx->codec_id == AV_CODEC_ID_HEVC)
                framesCtx->initial_pool_size += 16;
            else if (codecCtx->codec_id == AV_CODEC_ID_VP9)
                framesCtx->initial_pool_size += 8;
            else
                framesCtx->initial_pool_size += 2;

            framesCtx->initial_pool_size += 3 + g_delayedFrames; // extra_hw_frames

            switch (codecCtx->sw_pix_fmt)
            {
                case AV_PIX_FMT_YUV420P:
                case AV_PIX_FMT_YUVJ420P:
                    framesCtx->sw_format = AV_PIX_FMT_NV12;
                    break;
                case AV_PIX_FMT_YUV420P10:
                    framesCtx->sw_format = AV_PIX_FMT_P010;
                    break;
                default:
                    framesCtx->sw_format = AV_PIX_FMT_NONE;
                    break;
            }
            framesCtx->format = *pixFmt;

            framesCtx->width = codecCtx->width;
            framesCtx->height = codecCtx->height;

            auto d3d11vaFramesCtx = reinterpret_cast<AVD3D11VAFramesContext *>(framesCtx->hwctx);
            const bool isZeroCopy = codecCtx->opaque;
            if (isZeroCopy)
            {
                // Map the decoded buffer as a Vulkan image.
                d3d11vaFramesCtx->BindFlags = D3D11_BIND_DECODER;
                d3d11vaFramesCtx->MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
            }
            else
            {
                // Copy decoded into a Vulkan image. Don't set SHARED misc flags here, because it
                // can cause decoding issues on some drivers.
                d3d11vaFramesCtx->BindFlags = D3D11_BIND_DECODER | D3D11_BIND_SHADER_RESOURCE;
            }

            av_hwframe_ctx_init(hwFrameBufferRef);

            codecCtx->hw_frames_ctx = hwFrameBufferRef;

            return *pixFmt;
        }
        ++pixFmt;
    }
    return AV_PIX_FMT_NONE;
}

/**/

FFDecD3D11VA::FFDecD3D11VA(Module &module)
{
    SetModule(module);
}
FFDecD3D11VA::~FFDecD3D11VA()
{
    av_buffer_unref(&m_hwDeviceBufferRef);
}

bool FFDecD3D11VA::set()
{
    bool restartPlayback = false;
    const bool zeroCopyAllowed = sets().getBool("DecoderD3D11VAZeroCopy");
    if (m_zeroCopyAllowed != zeroCopyAllowed)
    {
        m_zeroCopyAllowed = zeroCopyAllowed;
        restartPlayback = true;
    }
    return !restartPlayback && sets().getBool("DecoderD3D11VAEnabled");
}

QString FFDecD3D11VA::name() const
{
    return "FFmpeg/D3D11VA";
}

int FFDecD3D11VA::decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurryUp)
{
    if (flush)
    {
        m_d3d11vaVulkan->clear();
        m_frames.clear();
    }

    Frame tmpDecoded;
    int ret = FFDecHWAccel::decodeVideo(encodedPacket, tmpDecoded, newPixFmt, flush, hurryUp);

    bool added = false;
    if (ret > -1 && !tmpDecoded.isEmpty())
    {
        if (m_d3d11vaVulkan->hasKMT()) // Works better if non-KMT handler on Intel doesn't map here
        {
            m_d3d11vaVulkan->map(tmpDecoded);
            if (m_d3d11vaVulkan->hasError())
            {
                QMPlay2Core.logError("D3D11VA :: Frame map error");
                m_hasCriticalError = true;
                return -1;
            }
        }
        m_frames.emplace_back(move(tmpDecoded));
        added = true;
    }

    if (!m_frames.empty() && (m_frames.size() > g_delayedFrames || (!added && encodedPacket.isEmpty())))
    {
        decoded = move(m_frames.front());
        m_frames.pop_front();
        if (ret < 0)
            ret = 0;
    }

    return ret;
}

bool FFDecD3D11VA::open(StreamInfo &streamInfo)
{
    if (streamInfo.codec_type != AVMEDIA_TYPE_VIDEO)
        return false;

    const auto pixFmt = streamInfo.pixelFormat();
    if (pixFmt == AV_PIX_FMT_YUV420P10)
    {
        auto vkInstance = static_pointer_cast<QmVk::Instance>(QMPlay2Core.gpuInstance());
        if (!vkInstance->supportedPixelFormats().contains(pixFmt))
            return false;
    }
    else if (pixFmt != AV_PIX_FMT_YUV420P && pixFmt != AV_PIX_FMT_YUVJ420P)
    {
        return false;
    }

    AVCodec *codec = init(streamInfo);
    if (!codec || !hasHWAccel("d3d11va"))
        return false;

    m_d3d11vaVulkan = QMPlay2Core.gpuInstance()->getHWDecContext<D3D11VAVulkan>();
    if (m_d3d11vaVulkan)
    {
        m_hwDeviceBufferRef = av_buffer_ref(m_d3d11vaVulkan->m_hwDeviceBufferRef);
        m_d3d11vaVulkan->clear();
    }

    if (!m_hwDeviceBufferRef)
    {
        // Check if primary Direct3D device matches the chosen Vulkan device.
        // Don't enforce the Direct3D device, because exporting the  shared
        // handle might not work.
        if (!comparePrimaryDevice())
        {
            QMPlay2Core.logError("D3D11VA :: Primary device doesn't match");
            return false;
        }

        av_hwdevice_ctx_create(&m_hwDeviceBufferRef, AV_HWDEVICE_TYPE_D3D11VA, nullptr, nullptr, 0);
        if (!m_hwDeviceBufferRef)
            return false;
    }

    if (!m_d3d11vaVulkan)
    {
        m_d3d11vaVulkan = make_shared<D3D11VAVulkan>(m_hwDeviceBufferRef, m_zeroCopyAllowed);
        if (!m_d3d11vaVulkan->init())
            return false;
        if (!QMPlay2Core.gpuInstance()->setHWDecContextForVideoOutput(m_d3d11vaVulkan))
            return false;
    }

    m_hasHWDecContext = true;

    codec_ctx->hw_device_ctx = av_buffer_ref(m_hwDeviceBufferRef);
    codec_ctx->get_format = d3d11vaGetFormat;
    codec_ctx->thread_count = 1;
    codec_ctx->opaque = reinterpret_cast<void *>(m_d3d11vaVulkan->isZeroCopy());
    if (!openCodec(codec))
        return false;

    m_timeBase = streamInfo.time_base;
    return true;
}

bool FFDecD3D11VA::comparePrimaryDevice() const
{
    static const auto createDXGIFactory = [] {
        HRESULT(WINAPI *createDXGIFactory)(REFIID, void **) = nullptr;
        QLibrary dxgi("dxgi.dll");
        if (dxgi.load())
            createDXGIFactory =  reinterpret_cast<decltype(createDXGIFactory)>(dxgi.resolve("CreateDXGIFactory"));
        return createDXGIFactory;
    }();
    if (!createDXGIFactory)
        return true;

    ComPtr<IDXGIFactory2> dxgiFactory;
    createDXGIFactory(__uuidof(IDXGIFactory2), reinterpret_cast<void **>(dxgiFactory.GetAddressOf()));
    if (!dxgiFactory)
        return true;

    ComPtr<IDXGIAdapter> adapter;
    if (SUCCEEDED(dxgiFactory->EnumAdapters(0, adapter.GetAddressOf())))
    {
        DXGI_ADAPTER_DESC desc  {};
        if (SUCCEEDED(adapter->GetDesc(&desc)))
        {
            const auto &vkDeviceProperties = static_pointer_cast<QmVk::Instance>(QMPlay2Core.gpuInstance())->physicalDevice()->properties();
            return (vkDeviceProperties.vendorID == desc.VendorId && vkDeviceProperties.deviceID == desc.DeviceId);
        }
    }

    return true;
}
