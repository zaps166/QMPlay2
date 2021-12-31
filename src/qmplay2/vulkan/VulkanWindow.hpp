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

#pragma once

#include <VideoOutputCommon.hpp>
#include <Frame.hpp>

#include <QMatrix4x4>
#include <QWindow>
#include <QTimer>

#include <vulkan/vulkan.hpp>

#include <mutex>
#include <set>

class QMPlay2OSD;
class Frame;

namespace QmVk {

using namespace std;

struct VideoPipelineSpecializationData;
struct FrameProps;

class Instance;
class PhysicalDevice;
class Device;
class Queue;
class ShaderModule;
class CommandBuffer;
class Sampler;
class SwapChain;
class GraphicsPipeline;
class RenderPass;
class Buffer;
class Image;
class DescriptorPool;
class HWInterop;

class Window : public QWindow, public VideoOutputCommon
{
    Q_OBJECT

public:
    Window(const shared_ptr<HWInterop> &hwInterop);
    ~Window();

    void setConfig(
        Qt::CheckState vsync,
        bool hqScaleDown,
        bool hqScaleUp,
        bool bypassCompositor
    );
    void setParams(
        const QSize &size,
        double aRatio,
        double zoom,
        bool sphericalView,
        int flip,
        bool rotate90,
        float brightness,
        float contrast,
        float hue,
        float saturation,
        float sharpness
    );

    AVPixelFormats supportedPixelFormats() const;

    inline bool hasError() const;

    inline bool isDeepColor() const;

    void setFrame(const Frame &frame);
    void setOSD(const QList<const QMPlay2OSD *> &osd);

private:
    inline VideoPipelineSpecializationData *getVideoPipelineSpecializationData();

    void maybeRequestUpdate();

    bool obtainFrameProps();

    void updateSizesAndMatrix();
    void onMatrixChange();

private:
    void initResources();

    void handleException(const vk::SystemError &e);

    void resetImages(bool resetImageOptimalTiling);

    void render();

    vector<unique_lock<mutex>> prepareOSD(bool &changed);

    void beginRenderPass(uint32_t imageIdx);

    void maybeClear(uint32_t imageIdx);
    void renderVideo();
    void renderOSD();

    bool ensureDevice();

    bool ensureSurfaceAndRenderPass();
    void ensureSwapChain();
    void ensureClearPipeline();
    void ensureVideoPipeline();

    void resetVerticesBuffer();
    void fillVerticesBuffer();

    bool ensureHWImageMapped();

    void loadImage();
    void obtainVideoPipelineSpecializationFrameProps();

    void fillVideoPipelineFragmentUniform();

    bool ensureMipmaps();
    bool mustGenerateMipmaps();

    bool ensureSupportedSampledImage();

    void ensureBicubic();

    inline void draw4Vertices();

    void resetSwapChainAndGraphicsPipelines(bool takeOldSwapChain);
    void resetSurface();

private:
    bool eventFilter(QObject *o, QEvent *e) override;

    bool event(QEvent *e) override;

private:
    const shared_ptr<HWInterop> &m_vkHwInterop;

    const shared_ptr<Instance> m_instance;
    const shared_ptr<PhysicalDevice> m_physicalDevice;

    QTimer m_initResourcesTimer;

    bool m_useRenderPassClear = false;

    bool m_error = false;

    bool m_isDeepColor = false;
    bool m_updateName = false;

    bool m_canCreateSurface = false;

    struct
    {
        shared_ptr<Device> device;
        shared_ptr<Queue> queue;

        vk::SurfaceKHR surface;

        unique_lock<mutex> queueLocker;

        shared_ptr<ShaderModule> clearVertexShaderModule;
        shared_ptr<ShaderModule> clearFragmentShaderModule;

        shared_ptr<ShaderModule> vertexShaderModule;
        shared_ptr<ShaderModule> fragmentShaderModule;

        shared_ptr<ShaderModule> osdVertexShaderModule;
        shared_ptr<ShaderModule> osdAvFragmentShaderModule;
        shared_ptr<ShaderModule> osdAssFragmentShaderModule;

        shared_ptr<CommandBuffer> commandBuffer;

        shared_ptr<RenderPass> renderPass;
        shared_ptr<SwapChain> swapChain;
        vk::UniqueSwapchainKHR oldSwapChain;

        shared_ptr<Buffer> fragUniform;
        bool mustUpdateFragUniform = true;

        shared_ptr<Buffer> verticesStagingBuffer;
        shared_ptr<Buffer> verticesBuffer;
        function<void()> commandBufferDrawFn;

        shared_ptr<Sampler> sampler;
        shared_ptr<Image> image;
        shared_ptr<Image> imageOptimalTiling;
        bool imageFromFrame = false;
        bool shouldUpdateImageOptimalTiling = false;

        shared_ptr<GraphicsPipeline> clearPipeline;
        set<uint32_t> clearedImages;

        shared_ptr<GraphicsPipeline> videoPipeline;

        shared_ptr<GraphicsPipeline> osdAvPipeline;
        shared_ptr<DescriptorPool> osdAvDescriptorPool;

        shared_ptr<GraphicsPipeline> osdAssPipeline;
        shared_ptr<DescriptorPool> osdAssDescriptorPool;
    } m;

    vector<uint32_t> m_videoPipelineSpecializationData;

    set<uint64_t> m_osdIDs;

    Qt::CheckState m_vsync = Qt::PartiallyChecked;
    bool m_hqScaleDown = false;
    bool m_hqScaleUp = false;

    QSize m_imgSize;
    int m_flip = 0;
    bool m_rotate90 = false;

    float m_brightness = 0.0f;
    float m_contrast = 1.0f;
    float m_hue = 0.0f;
    float m_saturation = 1.0f;
    float m_sharpness = 0.0f;

    vk::Format m_format = vk::Format::eUndefined;
    Frame m_frame;
    bool m_frameChanged = false;
    unique_ptr<FrameProps> m_frameProps;

    mutex m_osdMutex;
    QList<const QMPlay2OSD *> m_osd;
};

/* Inline implementation */

bool Window::hasError() const
{
    return m_error;
}

bool Window::isDeepColor() const
{
    return m_isDeepColor;
}

}
