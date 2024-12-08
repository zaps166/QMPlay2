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

#include "VulkanWindow.hpp"

#include "../../qmvk/DescriptorSetLayout.hpp"
#include "../../qmvk/MemoryObjectDescr.hpp"
#include "../../qmvk/GraphicsPipeline.hpp"
#include "../../qmvk/PhysicalDevice.hpp"
#include "../../qmvk/DescriptorPool.hpp"
#include "../../qmvk/CommandBuffer.hpp"
#include "../../qmvk/ShaderModule.hpp"
#include "../../qmvk/BufferView.hpp"
#include "../../qmvk/RenderPass.hpp"
#include "../../qmvk/SwapChain.hpp"
#include "../../qmvk/Sampler.hpp"
#include "../../qmvk/Device.hpp"
#include "../../qmvk/Buffer.hpp"
#include "../../qmvk/Queue.hpp"
#include "../../qmvk/Image.hpp"

#include "VulkanInstance.hpp"
#include "VulkanHWInterop.hpp"

#include <Functions.hpp>
#include <Sphere.hpp>

#include <QGuiApplication>
#include <QVulkanInstance>
#include <QElapsedTimer>
#include <qevent.h>
#include <QWidget>

#include <cmath>

namespace QmVk {

struct FrameProps
{
    bool isHdr10St2084() const
    {
        return (colorPrimaries == AVCOL_PRI_BT2020 && colorTrc == AVCOL_TRC_SMPTE2084);
    }

    AVColorPrimaries colorPrimaries;
    AVColorTransferCharacteristic colorTrc;
    AVColorSpace colorSpace;
    bool limited;
    bool gray;
    bool plannar;
    bool rgb;
    int numPlanes;
    int paddingBits;
    float maxLuminance;
};

struct VideoPipelineSpecializationData
{
    int numPlanes;
    int idxPlane0;
    int idxPlane1;
    int idxPlane2;
    int idxComponent0;
    int idxComponent1;
    int idxComponent2;
    vk::Bool32 hasLuma;
    vk::Bool32 isGray;
    vk::Bool32 useBicubic;
    vk::Bool32 useBrightnessContrast;
    vk::Bool32 useHueSaturation;
    vk::Bool32 useSharpness;
    vk::Bool32 negative;
    int trc;
};

struct alignas(16) VertPushConstants
{
    QGenericMatrix<4, 4, float> mat;
    QVector2D imgSizeMultiplier;
};

struct alignas(16) FragUniform
{
    QGenericMatrix<3, 4, float> yuvToRgbMatrix;
    QGenericMatrix<3, 4, float> colorPrimariesMatrix;

    QVector2D levels;
    QVector2D rangeMultiplier;
    float bitsMultiplier;

    float brightness;
    float contrast;
    float hue;
    float saturation;
    float sharpness;

    float maxLuminance;
};

struct alignas(16) OSDPushConstants
{
    QGenericMatrix<4, 4, float> mat;
    QSize size;
    int linesize;
    int padding;
    QVector4D color; // ASS only
};

static inline bool isTrcSupported(AVColorTransferCharacteristic trc)
{
    switch (trc)
    {
        case AVCOL_TRC_BT709:
        case AVCOL_TRC_SMPTE2084:
        case AVCOL_TRC_ARIB_STD_B67:
            return true;
        default:
            break;
    }
    return false;
}

Window::Window(const shared_ptr<HWInterop> &hwInterop)
    : VideoOutputCommon(true)
    , m_vkHwInterop(hwInterop)
    , m_instance(static_pointer_cast<Instance>(QMPlay2Core.gpuInstance()))
    , m_physicalDevice(m_instance->physicalDevice())
    , m_platformName(QGuiApplication::platformName())
    , m_isWayland(m_platformName.contains("wayland"))
    , m_passEventsToParent(m_platformName != "xcb" && m_platformName != "android")
    , m_videoPipelineSpecializationData(sizeof(VideoPipelineSpecializationData) / sizeof(int))
    , m_frameProps(make_unique<FrameProps>())
{
    setSurfaceType(VulkanSurface);
    setVulkanInstance(m_instance->qVulkanInstance());

    if (m_platformName == "xcb" && !qgetenv("WAYLAND_DISPLAY").isEmpty())
    {
        m_useRenderPassClear = true;
    }
    else switch (m_physicalDevice->properties().vendorID)
    {
#if !defined(Q_OS_WIN)
        case 0x8086: // Intel
        case 0x1002: // AMD
#endif
        case 0x10de: // NVIDIA
            break;
        case 0x10005: // LVP
            break;
        default:
            m_useRenderPassClear = true;
            break;
    }

    if (!m_passEventsToParent)
        setFlags(Qt::WindowTransparentForInput);

    connect(qGuiApp, &QGuiApplication::applicationStateChanged,
            this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationSuspended)
            resetSurface();
    });

    m_widget = QWidget::createWindowContainer(this);
    if (!m_isWayland && !m_platformName.contains("android"))
        m_widget->setAttribute(Qt::WA_NativeWindow);
    m_widget->grabGesture(Qt::PinchGesture);
    m_widget->installEventFilter(this);
    m_widget->setMouseTracking(true);
    m_widget->setAcceptDrops(false);

    connect(&m_initResourcesTimer, &QTimer::timeout,
            this, &Window::initResources);
    m_initResourcesTimer.setSingleShot(true);
    m_initResourcesTimer.setInterval(1000);

    m_matrixChangeFn = bind(&Window::onMatrixChange, this);

    initResources();
}
Window::~Window()
{
}

void Window::deleteWidget()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (m_isWayland)
    {
        // "SurfaceAboutToBeDestroyed" is delivered too late when deleting a widget container (Qt bug, tested on 6.5.1)
        resetSurface();
    }
#endif
    delete widget();
}

void Window::setConfig(
    Qt::CheckState vsync,
    bool nearestScaling,
    bool hqScaleDown,
    bool hqScaleUp,
    bool bypassCompositor,
    bool hdr)
{
    if (nearestScaling)
    {
        hqScaleDown = false;
        hqScaleUp = false;
    }

    if (m_vsync != vsync)
    {
        m_vsync = vsync;
        resetSwapChainAndGraphicsPipelines(true);
        maybeRequestUpdate();
    }
    if (m_nearestScaling != nearestScaling)
    {
        m_nearestScaling = nearestScaling;
        maybeRequestUpdate();
    }
    if (m_hqScaleDown != hqScaleDown)
    {
        m_hqScaleDown = hqScaleDown;
        maybeRequestUpdate();
    }
    if (m_hqScaleUp != hqScaleUp)
    {
        m_hqScaleUp = hqScaleUp;
        maybeRequestUpdate();
    }
    if (QGuiApplication::platformName() == "xcb")
    {
       setX11BypassCompositor(bypassCompositor);
    }
#ifdef Q_OS_WIN
    else
    {
        if (!m_physicalDevice->checkExtension(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME))
            bypassCompositor = true;
        if (m_widget->property("bypassCompositor").toBool() != bypassCompositor)
        {
            m_widget->setProperty("bypassCompositor", bypassCompositor);
            resetSwapChainAndGraphicsPipelines(false);
            maybeRequestUpdate();
        }
    }
    if (m_hdr != hdr)
    {
        m_hdr = hdr;
        m.checkSurfaceColorSpace = true;
        m.hdrSettingsChanged = true;
        m.mustUpdateVideoPipelineSpecialization = true;
        maybeRequestUpdate();
    }
#endif
}
void Window::setParams(
    const QSize &imgSize,
    double aRatio,
    double zoom,
    bool sphericalView,
    int flip,
    bool rotate90,
    float brightness,
    float contrast,
    float hue,
    float saturation,
    float sharpness,
    bool negative,
    AVColorPrimaries colorPrimaries,
    AVColorTransferCharacteristic colorTrc)
{
    const bool flipRotateChanged = (m_flip != flip || m_rotate90 != rotate90);

    if (m_imgSize != imgSize)
    {
        resetImages(true);
        m_frame.clear();
    }

    m_aRatio = aRatio;
    m_zoom = zoom;

    m_imgSize = imgSize;
    m_flip = flip;
    m_rotate90 = rotate90;

    m_negative = negative;

    if (
        !qFuzzyCompare(m_brightness, brightness) ||
        !qFuzzyCompare(m_contrast, contrast) ||
        !qFuzzyCompare(m_hue, hue) ||
        !qFuzzyCompare(m_saturation, saturation) ||
        !qFuzzyCompare(m_sharpness, sharpness)
    ) {
        m_brightness = brightness;
        m_contrast = contrast;
        m_hue = hue;
        m_saturation = saturation;
        m_sharpness = sharpness;
        m.mustUpdateFragUniform = true;
    }

    auto specializationData = getVideoPipelineSpecializationData();
    specializationData->useBrightnessContrast = (!qFuzzyIsNull(brightness) || !qFuzzyCompare(contrast, 1.0f));
    specializationData->useHueSaturation = (!qFuzzyIsNull(hue) || !qFuzzyCompare(saturation, 1.0f));
    specializationData->useSharpness = !qFuzzyIsNull(sharpness);
    specializationData->negative = m_negative;

    if (setSphericalView(sphericalView) || (!m_sphericalView && flipRotateChanged))
        resetVerticesBuffer();

    if (m_frameProps->numPlanes == 0)
    {
        // Set only if frame properties are not yet set
        m_frameProps->colorPrimaries = colorPrimaries;
        m_frameProps->colorTrc = colorTrc;
    }

    updateSizesAndMatrix();
    maybeRequestUpdate();
}

AVPixelFormats Window::supportedPixelFormats() const
{
    return m_instance->supportedPixelFormats();
}

void Window::setFrame(const Frame &frame, QMPlay2OSDList &&osdList)
{
    m_osd = move(osdList);
    if (m.imageFromFrame)
        resetImages(false);
    m_frame = frame;
    m_frameChanged = true;
    if (obtainFrameProps())
    {
        // Frame properties changed
        m.checkSurfaceColorSpace = true;
        m.mustUpdateVideoPipelineSpecialization = true;
        m.mustUpdateFragUniform = true;
    }
    maybeRequestUpdate();
}

inline VideoPipelineSpecializationData *Window::getVideoPipelineSpecializationData()
{
    return reinterpret_cast<VideoPipelineSpecializationData *>(m_videoPipelineSpecializationData.data());
}

void Window::maybeRequestUpdate()
{
    if (m.device && isExposed())
        requestUpdate();
}

bool Window::obtainFrameProps()
{
    FrameProps frameProps;
    frameProps.colorPrimaries = m_frame.colorPrimaries();
    frameProps.colorTrc = m_frame.colorTrc();
    frameProps.colorSpace = m_frame.colorSpace();
    frameProps.limited = m_frame.isLimited();
    frameProps.gray = m_frame.isGray();
    frameProps.plannar = m_frame.isPlannar();
    frameProps.rgb = m_frame.isRGB();
    frameProps.numPlanes = m_frame.numPlanes();
    frameProps.paddingBits = m_frame.paddingBits();
    frameProps.maxLuminance = 1000.0f;

    if (auto metadata = m_frame.masteringDisplayMetadata())
    {
        if (metadata->has_luminance)
        {
            const float maxLuminance = av_q2d(metadata->max_luminance);
            if (maxLuminance > 1.0f && maxLuminance <= 10000.0f)
                frameProps.maxLuminance = maxLuminance;
        }
    }

    const auto comp = memcmp(m_frameProps.get(), &frameProps, sizeof(FrameProps));
    if (comp != 0)
    {
        *m_frameProps = frameProps;
        return true;
    }
    return false;
}

void Window::updateSizesAndMatrix()
{
    m.clearedImages.clear();
    updateSizes(m_rotate90);
    updateMatrix();
}
void Window::onMatrixChange()
{
    m.clearedImages.clear();
    updateMatrix();
    maybeRequestUpdate();
}

void Window::initResources() try
{
    m.device = m_instance->createOrGetDevice(m_physicalDevice);

    m.queue = m.device->firstQueue();

    m.vertexShaderModule = ShaderModule::create(
        m.device,
        vk::ShaderStageFlagBits::eVertex,
        Instance::readShader("video.vert")
    );
    m.fragmentShaderModule = ShaderModule::create(
        m.device,
        vk::ShaderStageFlagBits::eFragment,
        Instance::readShader("video.frag")
    );

    m.osdVertexShaderModule = ShaderModule::create(
        m.device,
        vk::ShaderStageFlagBits::eVertex,
        Instance::readShader("osd.vert")
    );
    m.osdAvFragmentShaderModule = ShaderModule::create(
        m.device,
        vk::ShaderStageFlagBits::eFragment,
        Instance::readShader("osd_av.frag")
    );
    m.osdAssFragmentShaderModule = ShaderModule::create(
        m.device,
        vk::ShaderStageFlagBits::eFragment,
        Instance::readShader("osd_ass.frag")
    );

    m.commandBuffer = CommandBuffer::create(m.queue);

    m.fragUniform = Buffer::createUniformWrite(m.device, sizeof(FragUniform));
} catch (const vk::SystemError &e) {
    handleException(e);
}

void Window::handleException(const vk::SystemError &e)
{
    m_instance->resetDevice(m.device);
    m = {};

    if (e.code() == vk::Result::eErrorDeviceLost)
    {
        qDebug() << e.what();
        m_initResourcesTimer.start();
    }
    else
    {
        QMPlay2Core.logError(QString("Vulkan :: %1").arg(e.what()));
        m_error = true;
    }
}

void Window::resetImages(bool resetImageOptimalTiling)
{
    m.image.reset();
    if (resetImageOptimalTiling)
        m.imageOptimalTiling.reset();
    m.imageFromFrame = false;
    m.shouldUpdateImageOptimalTiling = false;
}

void Window::render()
{
    bool suboptimal = false;
    bool outOfDate = false;
    bool canSubmitCommandBufferFromError = false;

    if (!ensureDevice())
        return;

    auto submitCommandBuffer = [&] {
        // In some cases the command buffer has image copy or mipmap generation commands,
        // so we have to submit the command buffer to prevent desynchronization with
        // local variables inside the Image class.
        try
        {
            if (canSubmitCommandBufferFromError)
                m.commandBuffer->endSubmitAndWait();
        }
        catch (const vk::SystemError &e)
        {
            handleException(e);
        }
    };

    if (m.checkSurfaceColorSpace)
    {
        const bool surfaceMatchesFrameProps = (isHdr10St2084() == m_frameProps->isHdr10St2084());
        if (m.renderPass && ((m.hasHdr10St2084 && !surfaceMatchesFrameProps) || (m.hdrSettingsChanged && m_hdr != surfaceMatchesFrameProps)))
        {
            resetSwapChainAndGraphicsPipelines(true);
            m.renderPass.reset();
        }
        m.checkSurfaceColorSpace = false;
        m.hdrSettingsChanged = false;
    }

    try
    {
        const bool imageReady = ensureHWImageMapped();

        if (!ensureSurfaceAndRenderPass())
            return;

        m.commandBuffer->resetAndBegin();
        canSubmitCommandBufferFromError = true;

        ensureSwapChain();

        if (!m.commandBufferDrawFn)
            fillVerticesBuffer();

        if (imageReady)
            loadImage();

        if (m.mustUpdateVideoPipelineSpecialization)
            obtainVideoPipelineSpecializationFrameProps();

        const bool mustGenerateMipmaps = this->mustGenerateMipmaps();
        const bool mipmapsUsed = ensureMipmaps(mustGenerateMipmaps);
        const bool imageOptimalCopied = ensureSupportedSampledImage(mustGenerateMipmaps);
        if (!mipmapsUsed && !imageOptimalCopied)
            m.imageOptimalTiling.reset();

        ensureSampler();

        ensureBicubic();

        ensureVideoPipeline();

        if (m.mustUpdateFragUniform)
            fillVideoPipelineFragmentUniform();

        if (m.videoPipeline)
        {
            m.videoPipeline->prepareObjects(m.commandBuffer);
            if (m_vkHwInterop)
                m_vkHwInterop->updateInfo({m_frame});
        }

        bool osdChanged = false;

        // OSD mutexes are unlocked after rendering complete or after exception is thrown
        auto osdLockers = prepareOSD(osdChanged);

        if (osdChanged)
            m.clearedImages.clear();

        const uint32_t imageIdx = m.swapChain->acquireNextImage(&suboptimal);
        if (!suboptimal || m.swapChain->maybeSuboptimal())
        {
            auto submitInfo = m.swapChain->getSubmitInfo();
            HWInterop::SyncDataPtr syncData;
            if (m_vkHwInterop)
                syncData = m_vkHwInterop->sync({m_frame}, &submitInfo);

            canSubmitCommandBufferFromError = false;
            beginRenderPass(imageIdx);

            maybeClear(imageIdx);
            renderVideo();
            if (!osdLockers.empty())
                renderOSD();

            m.commandBuffer->endRenderPass(m.commandBuffer->dld());

            if (m.videoPipeline && m_instance->hasFiltersOnOtherQueueFamiliy())
                m.videoPipeline->finalizeObjects(m.commandBuffer, false, true);

            m.queueLocker = m.queue->lock();
            m.commandBuffer->endSubmitAndWait(false, [&] {
                m.swapChain->present(imageIdx, &suboptimal);
                vulkanInstance()->presentQueued(this);
            }, move(submitInfo));
            m.queueLocker.unlock(); // It is not unlocked in case of exception
        }
    }
    catch (const vk::OutOfDateKHRError &e)
    {
        Q_UNUSED(e)
        outOfDate = true;
    }
    catch (const vk::SurfaceLostKHRError &e)
    {
        Q_UNUSED(e)
        submitCommandBuffer();
        resetSurface();
        return;
    }
    catch (const vk::SystemError &e)
    {
        handleException(e);
        return;
    }

    if (outOfDate || (suboptimal && !m.swapChain->maybeSuboptimal()))
    {
        submitCommandBuffer();
        resetSwapChainAndGraphicsPipelines(true);
        maybeRequestUpdate();
    }
}

vector<unique_lock<mutex>> Window::prepareOSD(bool &changed)
{
    const auto osdIDs = move(m_osdIDs);

    if (m_osd.empty())
    {
        if (!osdIDs.empty())
            changed = true;
        return {};
    }

    vector<unique_lock<mutex>> lockers;
    lockers.reserve(m_osd.size());

    for (auto &&osd : std::as_const(m_osd))
    {
        lockers.push_back(osd->lock());

        const auto id = osd->id();
        if (!changed && osdIDs.count(id) == 0)
            changed = true;
        m_osdIDs.insert(id);
    }

    if (!changed && m_osdIDs.size() != osdIDs.size())
        changed = true;

    return lockers;
}

void Window::beginRenderPass(uint32_t imageIdx)
{
    vk::RenderPassBeginInfo renderPassBeginInfo;
    renderPassBeginInfo.renderPass = *m.renderPass;
    renderPassBeginInfo.framebuffer = m.swapChain->frameBuffer(imageIdx);
    renderPassBeginInfo.renderArea.extent = m.swapChain->size();

    vk::ClearValue clearValue(array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
    if (m_useRenderPassClear)
    {
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearValue;
    }

    m.commandBuffer->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline, m.commandBuffer->dld());
}

void Window::maybeClear(uint32_t imageIdx)
{
    if (m_useRenderPassClear || m_sphericalView || m.clearedImages.count(imageIdx) > 0)
        return;

    vk::ClearAttachment clearAttachment;
    clearAttachment.aspectMask = vk::ImageAspectFlagBits::eColor;
    clearAttachment.clearValue.color.float32[3] = 1.0f;

    vk::ClearRect clearRect(vk::Rect2D(vk::Offset2D(), m.swapChain->size()), 0, 1);

    m.commandBuffer->clearAttachments(clearAttachment, clearRect, m.commandBuffer->dld());

    m.clearedImages.insert(imageIdx);
}
void Window::renderVideo()
{
    if (!m.videoPipeline)
        return;

    m.videoPipeline->recordCommands(m.commandBuffer);
    m.commandBufferDrawFn();
}
void Window::renderOSD()
{
    const auto winSize = getRealWidgetSize();

    vector<MemoryObjectDescrs> osdAvMemoryObjects;
    vector<MemoryObjectDescrs> osdAssMemoryObjects;

    vector<pair<const QMPlay2OSD::Image *, const QMPlay2OSD *>> osdImages;

    for (auto &&osd : std::as_const(m_osd))
    {
        osd->iterate([&](const QMPlay2OSD::Image &image) {
            if (image.dataBufferView->buffer()->device() != m.device)
                return;

            if (image.paletteBufferView)
            {
                Q_ASSERT(osd->needsRescale());

                if (image.paletteBufferView->buffer()->device() != m.device)
                    return;

                osdAvMemoryObjects.push_back({
                    image.paletteBufferView,
                    image.dataBufferView,
                });
            }
            else
            {
                osdAssMemoryObjects.push_back({
                    image.dataBufferView,
                });
            }

            osdImages.emplace_back(&image, osd.get());
        });
    }

    auto updateOsdDescriptorPool = [this](
            shared_ptr<DescriptorPool> &osdDescriptorPool,
            const vector<MemoryObjectDescrs> &memoryObjects) {
        if (!memoryObjects.empty() && (!osdDescriptorPool || osdDescriptorPool->max() < memoryObjects.size()))
        {
            osdDescriptorPool = DescriptorPool::create(
                DescriptorSetLayout::create(m.device, memoryObjects[0].fetchDescriptorTypes()),
                memoryObjects.size()
            );
        }
    };
    updateOsdDescriptorPool(m.osdAvDescriptorPool, osdAvMemoryObjects);
    updateOsdDescriptorPool(m.osdAssDescriptorPool, osdAssMemoryObjects);

    auto createOsdPipeline = [this](const shared_ptr<ShaderModule> &shaderModule) {
        GraphicsPipeline::CreateInfo createInfo;
        createInfo.device = m.device;
        createInfo.vertexShaderModule = m.osdVertexShaderModule;
        createInfo.fragmentShaderModule = shaderModule;
        createInfo.renderPass = m.renderPass;
        createInfo.size = m.swapChain->size();
        createInfo.pushConstantsSize = sizeof(OSDPushConstants);
        return GraphicsPipeline::create(createInfo);
    };
    if (!osdAvMemoryObjects.empty() && !m.osdAvPipeline)
        m.osdAvPipeline = createOsdPipeline(m.osdAvFragmentShaderModule);
    if (!osdAssMemoryObjects.empty() && !m.osdAssPipeline)
        m.osdAssPipeline = createOsdPipeline(m.osdAssFragmentShaderModule);

    size_t osdAvMemoryObjectsIdx = 0;
    size_t osdAssMemoryObjectsIdx = 0;
    for (auto &&[img, osd] : osdImages)
    {
        QVector2D multiplier(Qt::Uninitialized);
        shared_ptr<GraphicsPipeline> osdPipeline;
        shared_ptr<DescriptorPool> osdDescriptorPool;
        MemoryObjectDescrs *memObjectDescrs = nullptr;

        if (img->paletteBufferView)
        {
            multiplier = QVector2D(
                static_cast<double>(m_scaledSize.width())  / static_cast<double>(m_imgSize.width()),
                static_cast<double>(m_scaledSize.height()) / static_cast<double>(m_imgSize.height())
            );
            osdPipeline = m.osdAvPipeline;
            osdDescriptorPool = m.osdAvDescriptorPool;
            memObjectDescrs = &osdAvMemoryObjects[osdAvMemoryObjectsIdx++];
        }
        else
        {
            multiplier = QVector2D(1.0f, 1.0f);
            osdPipeline = m.osdAssPipeline;
            osdDescriptorPool = m.osdAssDescriptorPool;
            memObjectDescrs = &osdAssMemoryObjects[osdAssMemoryObjectsIdx++];
        }

        const auto imgRect = osd->getRect(*img);

        QMatrix4x4 mat;
        mat.translate(
            (-1.0 - m_osdOffset.x()) + (m_subsRect.x() + imgRect.x() * multiplier.x()) * 2.0 / winSize.width(),
            (-1.0 - m_osdOffset.y()) + (m_subsRect.y() + imgRect.y() * multiplier.y()) * 2.0 / winSize.height()
        );
        mat.scale(
            imgRect.width()  * 2.0 * multiplier.x() / winSize.width(),
            imgRect.height() * 2.0 * multiplier.y() / winSize.height()
        );

        auto pushConstants = osdPipeline->pushConstants<OSDPushConstants>();
        pushConstants->mat = mat.toGenericMatrix<4, 4>();
        pushConstants->size = img->size;
        pushConstants->linesize = img->linesize;
        pushConstants->color = img->color; // ASS only

        osdPipeline->createDescriptorSetFromPool(osdDescriptorPool);
        osdPipeline->setMemoryObjects(*memObjectDescrs);
        osdPipeline->prepare();

        osdPipeline->recordCommands(m.commandBuffer);
        draw4Vertices();
    }
}

bool Window::ensureDevice()
{
    auto device = m_instance->device();
    if (m.device == device)
        return true;

    m = {};
    m_initResourcesTimer.start();
    return false;
}

bool Window::ensureSurfaceAndRenderPass()
{
    if (m.renderPass)
        return true;

    if (!m_canCreateSurface)
        return false;

    if (!vulkanInstance()->supportsPresent(*m_physicalDevice, m.queue->queueFamilyIndex(), this))
    {
        QMPlay2Core.logError("Vulkan :: Present is not supported");
        m_error = true;
        return false;
    }

    if (!m.surface)
        m.surface = QVulkanInstance::surfaceForWindow(this);
    if (!m.surface)
        return false;

    const auto surfaceFormats = m_physicalDevice->getSurfaceFormatsKHR(m.surface, m_physicalDevice->dld());
    if (surfaceFormats.empty())
        return false;

    auto getSurfaceFormat = [&] {
        return SwapChain::getSurfaceFormat(
            surfaceFormats,
            {
                vk::Format::eA2B10G10R10UnormPack32,
                vk::Format::eA2R10G10B10UnormPack32,
                vk::Format::eB8G8R8A8Unorm,
                vk::Format::eR8G8B8A8Unorm,
            },
            m.colorSpace
        );
    };

    const auto prevColorSpace = m.colorSpace;

    auto surfaceFormat = vk::Format::eUndefined;
    if (m_hdr)
    {
        m.colorSpace = vk::ColorSpaceKHR::eHdr10St2084EXT;
        surfaceFormat = getSurfaceFormat();
    }
    m.hasHdr10St2084 = (surfaceFormat != vk::Format::eUndefined);
    if (!m.hasHdr10St2084 || !m_frameProps->isHdr10St2084())
    {
        m.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
        surfaceFormat = getSurfaceFormat();
    }
    if (surfaceFormat == vk::Format::eUndefined)
    {
        surfaceFormat = surfaceFormats[0].format;
    }

    m.renderPass = RenderPass::create(
        m.device,
        surfaceFormat,
        vk::ImageLayout::ePresentSrcKHR,
        m_useRenderPassClear
    );

    if (m.colorSpace != prevColorSpace)
        emit QMPlay2Core.updateInformationPanel();

    return true;
}
void Window::ensureSwapChain()
{
    if (m.swapChain)
        return;

    vector<vk::PresentModeKHR> presentModes;
    presentModes.reserve(3);
    switch (m_vsync)
    {
        case Qt::Unchecked:
        {
            presentModes.push_back(vk::PresentModeKHR::eImmediate);
            presentModes.push_back(vk::PresentModeKHR::eMailbox);
            presentModes.push_back(vk::PresentModeKHR::eFifo);
            break;
        }
        case Qt::PartiallyChecked:
        {
            const auto availPresentModes = m_physicalDevice->getSurfacePresentModesKHR(m.surface, m_physicalDevice->dld());
            if (find(availPresentModes.begin(), availPresentModes.end(), vk::PresentModeKHR::eMailbox) != availPresentModes.end())
            {
                presentModes.push_back(vk::PresentModeKHR::eMailbox);
            }
            else if (m_widget->window()->property("fullScreen").toBool())
            {
                presentModes.push_back(vk::PresentModeKHR::eFifo);
            }
            else
            {
                presentModes.push_back(vk::PresentModeKHR::eImmediate);
                presentModes.push_back(vk::PresentModeKHR::eFifo);
            }
            break;
        }
        case Qt::Checked:
        {
            presentModes.push_back(vk::PresentModeKHR::eFifo);
            presentModes.push_back(vk::PresentModeKHR::eMailbox);
            presentModes.push_back(vk::PresentModeKHR::eImmediate);
            break;
        }
    }

    const auto winSize = getRealWidgetSize();

    SwapChain::CreateInfo createInfo;
    createInfo.device = m.device;
    createInfo.queue = m.queue;
    createInfo.renderPass = m.renderPass;
    createInfo.surface = m.surface;
    createInfo.fallbackSize = vk::Extent2D(winSize.width(), winSize.height());
    createInfo.presentModes = move(presentModes);
    createInfo.oldSwapChain = move(m.oldSwapChain);
    createInfo.colorSpace = m.colorSpace;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    createInfo.exclusiveFullScreen = m_widget->property("bypassCompositor").toBool()
        ? vk::FullScreenExclusiveEXT::eAllowed
        : vk::FullScreenExclusiveEXT::eDisallowed
    ;
#endif
    m.swapChain = SwapChain::create(createInfo);
}
void Window::ensureVideoPipeline()
{
    if (!m.image)
    {
        m.videoPipeline.reset();
        return;
    }

    if (!m.videoPipeline)
    {
        vector<vk::VertexInputBindingDescription> vertexBindingDescrs(2);
        vertexBindingDescrs[0].binding = 0;
        vertexBindingDescrs[0].stride = sizeof(QVector3D);
        vertexBindingDescrs[0].inputRate = vk::VertexInputRate::eVertex;
        vertexBindingDescrs[1].binding = 1;
        vertexBindingDescrs[1].stride = sizeof(QVector2D);
        vertexBindingDescrs[1].inputRate = vk::VertexInputRate::eVertex;

        vector<vk::VertexInputAttributeDescription> vertexAttrDescrs(2);
        vertexAttrDescrs[0].location = 0;
        vertexAttrDescrs[0].binding = 0;
        vertexAttrDescrs[0].format = vk::Format::eR32G32B32Sfloat;
        vertexAttrDescrs[0].offset = 0;
        vertexAttrDescrs[1].location = 1;
        vertexAttrDescrs[1].binding = 1;
        vertexAttrDescrs[1].format = vk::Format::eR32G32Sfloat;
        vertexAttrDescrs[1].offset = 0;

        vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        colorBlendAttachment.colorWriteMask =
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB
        ;

        GraphicsPipeline::CreateInfo createInfo;
        createInfo.device = m.device;
        createInfo.vertexShaderModule = m.vertexShaderModule;
        createInfo.fragmentShaderModule = m.fragmentShaderModule;
        createInfo.renderPass = m.renderPass;
        createInfo.size = m.swapChain->size();
        createInfo.pushConstantsSize = sizeof(VertPushConstants);
        createInfo.vertexBindingDescrs = move(vertexBindingDescrs);
        createInfo.vertexAttrDescrs = move(vertexAttrDescrs);
        createInfo.colorBlendAttachment = &colorBlendAttachment;
        m.videoPipeline = GraphicsPipeline::create(createInfo);
    }

    auto vkImage = m.imageOptimalTiling
        ? m.imageOptimalTiling
        : m.image
    ;

    m.videoPipeline->setCustomSpecializationDataFragment(m_videoPipelineSpecializationData);
    m.videoPipeline->setMemoryObjects({
        {m.fragUniform},
        {vkImage, m.sampler},
    });
    m.videoPipeline->prepare();

    const int vkImgW = vkImage->size().width;
    const int imgW = m_imgSize.width();

    const int vkImgH = vkImage->size().height;
    const int imgH = m_imgSize.height();

    const float subtract = m_frame.hasBorders()
        ? 0.0f
        : 1.0f
    ;

    auto pushConstants = m.videoPipeline->pushConstants<VertPushConstants>();
    pushConstants->mat = m_matrix.toGenericMatrix<4, 4>();
    pushConstants->imgSizeMultiplier.setX((imgW == vkImgW)
        ? 1.0f
        : (imgW - subtract) / vkImgW
    );
    pushConstants->imgSizeMultiplier.setY((imgH == vkImgH)
        ? 1.0f
        : (imgH - subtract) / vkImgH
    );
}

void Window::resetVerticesBuffer()
{
    m.verticesStagingBuffer.reset();
    m.verticesBuffer.reset();
    m.commandBufferDrawFn = nullptr;
}
void Window::fillVerticesBuffer()
{
    constexpr uint32_t slices = 50;
    constexpr uint32_t stacks = 50;

    uint32_t verticesSize = 0, texCoordsSize = 0, indicesSize = 0;
    uint32_t nIndices = 0;

    if (!m_sphericalView)
    {
        verticesSize  = 4 * sizeof(QVector3D);
        texCoordsSize = 4 * sizeof(QVector2D);
    }
    else
    {
        nIndices = Sphere::getSizes(slices, stacks, verticesSize, texCoordsSize, indicesSize);
    }

    if (!m.verticesBuffer || !m.verticesStagingBuffer)
    {
        m.verticesBuffer = Buffer::createVerticesWrite(
            m.device,
            verticesSize + texCoordsSize + indicesSize,
            false
        );
        if (!m.verticesBuffer->isDeviceLocal())
        {
            m.verticesStagingBuffer = move(m.verticesBuffer);
            m.verticesBuffer = Buffer::createVerticesWrite(
                m.device,
                m.verticesStagingBuffer->size(),
                true
            );
        }
    }

    auto verticesBuffer = m.verticesStagingBuffer
        ? m.verticesStagingBuffer
        : m.verticesBuffer
    ;

    auto data = verticesBuffer->map<uint8_t>();

    auto bindVertexBuffer = [=] {
        m.commandBuffer->bindVertexBuffers(0, {*m.verticesBuffer, *m.verticesBuffer}, {0, verticesSize}, m.commandBuffer->dld());
    };

    if (!m_sphericalView)
    {
        auto vertices  = reinterpret_cast<QVector3D *>(data + 0);
        auto texCoords = reinterpret_cast<QVector2D *>(data + verticesSize);

        // bottom left
        vertices[0]  = {-1.0, +1.0, 0.0f};
        texCoords[0] = {+0.0, +1.0};

        // top left
        vertices[1]  = {-1.0, -1.0, 0.0f};
        texCoords[1] = {+0.0, +0.0};

        // bottom right
        vertices[2]  = {+1.0, +1.0, 0.0f};
        texCoords[2] = {+1.0, +1.0};

        // top right
        vertices[3]  = {+1.0, -1.0, 0.0f};
        texCoords[3] = {+1.0, +0.0};

        if (m_rotate90)
        {
            swap(texCoords[0], texCoords[1]);
            swap(texCoords[0], texCoords[3]);
            swap(texCoords[0], texCoords[2]);
        }
        if (m_flip & Qt::Horizontal)
        {
            swap(texCoords[0], texCoords[2]);
            swap(texCoords[1], texCoords[3]);
        }
        if (m_flip & Qt::Vertical)
        {
            swap(texCoords[0], texCoords[1]);
            swap(texCoords[2], texCoords[3]);
        }

        m.commandBufferDrawFn = [=] {
            bindVertexBuffer();
            draw4Vertices();
        };
    }
    else
    {
        Sphere::generate(
            1.0f,
            slices,
            stacks,
            reinterpret_cast<float *>(data + 0),
            reinterpret_cast<float *>(data + verticesSize),
            reinterpret_cast<uint16_t *>(data + verticesSize + texCoordsSize)
        );

        m.commandBufferDrawFn = [=] {
            bindVertexBuffer();
            m.commandBuffer->bindIndexBuffer(*m.verticesBuffer, verticesSize + texCoordsSize, vk::IndexType::eUint16, m.commandBuffer->dld());
            m.commandBuffer->drawIndexed(nIndices, 1, 0, 0, 0, m.commandBuffer->dld());
        };
    }

    verticesBuffer->unmap();

    if (m.verticesStagingBuffer)
        m.verticesStagingBuffer->copyTo(m.verticesBuffer, m.commandBuffer);
}

bool Window::ensureHWImageMapped()
{
    if (m_vkHwInterop)
    {
        m_vkHwInterop->map(m_frame);
        if (m_vkHwInterop->hasError())
            m_frame.clear();
        else if (!m_frame.vulkanImage())
            return false; // Not ready
    }
    return true;
}

void Window::loadImage()
{
    if (!m_frameChanged && m.image)
        return;

    const auto newFormat = m_frameChanged
        ? Instance::fromFFmpegPixelFormat(m_frame.pixelFormat())
        : m_format
    ;

    m.imageFromFrame = false;

    if (auto image = m_frame.vulkanImage())
    {
        if (image->device() == m.device)
        {
            m.image = move(image);
            m.imageFromFrame = true;
        }
    }

    if (!m.imageFromFrame && m_frame.hasCPUAccess())
    {
        Q_ASSERT(newFormat != vk::Format::eUndefined);

        if (m.image)
        {
            if (m_format != newFormat)
                m.image.reset();
            else
                Q_ASSERT(m.image->isHostVisible());
        }

        if (!m.image)
        {
            m.image = Image::createLinear(
                m.device,
                vk::Extent2D(m_imgSize.width(), m_imgSize.height()),
                newFormat,
                Image::MemoryPropertyPreset::PreferHostOnly
            );
        }

        m_frame.copyToVulkanImage(m.image);
    }

    if (m_frameChanged)
    {
        m.shouldUpdateImageOptimalTiling = true;
        m_format = newFormat;
    }

    m_frameChanged = false;
}
void Window::obtainVideoPipelineSpecializationFrameProps()
{
    auto specializationData = getVideoPipelineSpecializationData();
    specializationData->numPlanes = m_frameProps->numPlanes;
    switch (specializationData->numPlanes)
    {
        case 1:
            if (m_frameProps->gray)
            {
                specializationData->idxComponent0 = 0;
                specializationData->idxComponent1 = 0;
                specializationData->idxComponent2 = 0;
            }
            else
            {
                specializationData->idxComponent0 = 0;
                specializationData->idxComponent1 = 1;
                specializationData->idxComponent2 = 2;
            }
            break;
        case 2:
            specializationData->idxPlane0 = 0;
            specializationData->idxPlane1 = 1;
            specializationData->idxComponent1 = 0;
            specializationData->idxComponent2 = 1;
            break;
        case 3:
            if (m_frameProps->rgb)
            {
                // GBRP format
                specializationData->idxPlane0 = 2;
                specializationData->idxPlane1 = 0;
                specializationData->idxPlane2 = 1;
            }
            else
            {
                specializationData->idxPlane0 = 0;
                specializationData->idxPlane1 = 1;
                specializationData->idxPlane2 = 2;
            }
            break;
    }
    specializationData->hasLuma = !m_frameProps->rgb;
    specializationData->isGray = m_frameProps->gray;

    if (
        !m_frameProps->gray &&
        !isHdr10St2084() &&
        isTrcSupported(m_frameProps->colorTrc) && (
            m_frameProps->colorTrc != AVCOL_TRC_BT709 || (
                Functions::isColorPrimariesSupported(m_frameProps->colorPrimaries) &&
                m_frameProps->colorPrimaries != AVCOL_PRI_BT709
            )
        )
    ) {
        specializationData->trc = m_frameProps->colorTrc;
    }
    else
    {
        specializationData->trc = 0;
    }

    m.mustUpdateVideoPipelineSpecialization = false;
}

void Window::fillVideoPipelineFragmentUniform()
{
    auto fragData = m.fragUniform->map<FragUniform>();

    if (m_frameProps->rgb || m_frameProps->gray)
    {
        fragData->yuvToRgbMatrix.setToIdentity();

        fragData->levels.setY(0.0f);
    }
    else
    {
        fragData->yuvToRgbMatrix = Functions::getYUVtoRGBmatrix(
            m_frameProps->colorSpace
        ).toGenericMatrix<3, 4>();

        fragData->levels.setY(128.0 / 255.0f);
    }

    if (isTrcSupported(m_frameProps->colorTrc))
    {
        fragData->colorPrimariesMatrix = Functions::getColorPrimariesTo709Matrix(
            m_frameProps->colorPrimaries
        ).toGenericMatrix<3, 4>();
    }
    else
    {
        fragData->colorPrimariesMatrix.setToIdentity();
    }

    fragData->levels.setX(m_frameProps->limited
        ? 16.0f / 255.0f
        : 0.0f
    );

    fragData->bitsMultiplier = (m_frameProps->plannar || m_frameProps->gray)
        ? 1 << m_frameProps->paddingBits
        : 1.0f
    ;

    fragData->rangeMultiplier.setX(m_frameProps->limited
        ? 255.0f / (235.0f - 16.0f)
        : 1.0f
    );
    fragData->rangeMultiplier.setY(m_frameProps->limited
        ? 255.0f / (240.0f - 16.0f)
        : 1.0f
    );

    fragData->brightness = m_brightness;
    fragData->contrast = m_contrast;
    fragData->hue = m_hue;
    fragData->saturation = m_saturation;
    fragData->sharpness = m_sharpness;

    fragData->maxLuminance = m_frameProps->maxLuminance;

    m.fragUniform->unmap();

    m.mustUpdateFragUniform = false;
}

bool Window::ensureMipmaps(bool mustGenerateMipmaps)
{
    if (!mustGenerateMipmaps || !m.image)
        return false;

    if (m.imageOptimalTiling && (m.imageOptimalTiling->format() != m.image->format() || m.imageOptimalTiling->mipLevels() <= 1))
        m.imageOptimalTiling.reset();

    if (!m.imageOptimalTiling)
    {
        m.imageOptimalTiling = Image::createOptimal(
            m.device,
            vk::Extent2D(m_imgSize.width(), m_imgSize.height()),
            m.image->format(),
            true,
            false
        );

        m.shouldUpdateImageOptimalTiling = true;
    }
    else
    {
        Q_ASSERT(m.imageOptimalTiling->size() == vk::Extent2D(m_imgSize.width(), m_imgSize.height()));
    }

    const bool mustRegenerateMipmaps = m.imageOptimalTiling->setMipLevelsLimitForSize(
        vk::Extent2D(m_scaledSize.width(), m_scaledSize.height())
    );

    if (m.shouldUpdateImageOptimalTiling)
    {
        m.image->copyTo(m.imageOptimalTiling, m.commandBuffer);
        m.shouldUpdateImageOptimalTiling = false;
    }
    else if (mustRegenerateMipmaps)
    {
        m.imageOptimalTiling->maybeGenerateMipmaps(m.commandBuffer);
    }

    return true;
}
bool Window::mustGenerateMipmaps()
{
    if (!m_hqScaleDown || m_sphericalView)
        return false;

    const auto scaledSizeF = QSizeF(m_scaledSize);
    const auto imgSizeF = QSizeF(m_imgSize);
    return (scaledSizeF.width() / imgSizeF.width() < 0.75 || scaledSizeF.height() / imgSizeF.height() < 0.75);
}

bool Window::ensureSupportedSampledImage(bool mustGenerateMipmaps)
{
    if (!m.image || m.image->isSampled())
        return false;

    if (m.imageOptimalTiling && m.imageOptimalTiling->mipLevels() > 1)
    {
        if (!mustGenerateMipmaps)
            m.imageOptimalTiling.reset();
        else
            return false;
    }

    if (m.imageOptimalTiling && m.imageOptimalTiling->format() != m.image->format())
        m.imageOptimalTiling.reset();

    if (!m.imageOptimalTiling)
    {
        m.imageOptimalTiling = Image::createOptimal(
            m.device,
            vk::Extent2D(m_imgSize.width(), m_imgSize.height()),
            m.image->format(),
            false,
            false
        );

        m.shouldUpdateImageOptimalTiling = true;
    }
    else
    {
        Q_ASSERT(m.imageOptimalTiling->size() == vk::Extent2D(m_imgSize.width(), m_imgSize.height()));
    }

    if (m.shouldUpdateImageOptimalTiling)
    {
        m.image->copyTo(m.imageOptimalTiling, m.commandBuffer);
        m.shouldUpdateImageOptimalTiling = false;
    }

    return true;
}

void Window::ensureSampler()
{
    if (m.sampler && m_nearestScaling == (m.sampler->createInfo().magFilter == vk::Filter::eNearest))
        return;

    m.sampler.reset();
    m.sampler = Sampler::createClampToEdge(
        m.device,
        m_nearestScaling
            ? vk::Filter::eNearest
            : vk::Filter::eLinear
    );
}

void Window::ensureBicubic()
{
    auto &useBicubic = getVideoPipelineSpecializationData()->useBicubic;

    if (!m_hqScaleUp || m_sphericalView)
    {
        useBicubic = false;
        return;
    }

    useBicubic = (m_scaledSize.width() > m_imgSize.width() || m_scaledSize.height() > m_imgSize.height());
}

inline void Window::draw4Vertices()
{
    m.commandBuffer->draw(4, 1, 0, 0, m.commandBuffer->dld());
}

void Window::resetSwapChainAndGraphicsPipelines(bool takeOldSwapChain) try
{
    if (!m.device)
        return;

    if (takeOldSwapChain)
    {
        if (m.swapChain)
            m.oldSwapChain = m.swapChain->take();
    }
    else
    {
        m.oldSwapChain.reset();
    }

    if (!m.queueLocker.owns_lock())
        m.queueLocker = m.queue->lock();
    m.queue->waitIdle(m.queue->dld());
    m.queueLocker.unlock();

    m.commandBuffer->resetStoredData();

    m.videoPipeline.reset();
    m.osdAvPipeline.reset();
    m.osdAssPipeline.reset();
    m.swapChain.reset();

    m.clearedImages.clear();
} catch (const vk::SystemError &e) {
    handleException(e);
}
void Window::resetSurface()
{
    resetSwapChainAndGraphicsPipelines(false);
    m.renderPass.reset();
    m.surface = nullptr; // Don't destroy the surface, because Qt manages it
}

bool Window::eventFilter(QObject *o, QEvent *e)
{
    Q_ASSERT(o == m_widget);
    switch (e->type())
    {
        case QEvent::Hide:
            if (m_isWayland)
            {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
                // QWindow is reset when hiding a widget container without "SurfaceAboutToBeDestroyed" notification (Qt bug, tested on 6.5.1)
                resetSurface();
#else
                // Destroy the surface to prevent "The Wayland connection experienced a fatal error: Invalid argument" (Qt bug, tested on 5.15.9+kde+r155)
                destroy();
#endif
            }
            break;
        default:
            break;
    }
    dispatchEvent(e, o);
    return QWindow::eventFilter(o, e);
}

bool Window::event(QEvent *e)
{
    switch (e->type())
    {
        case QEvent::Resize:
            resetSwapChainAndGraphicsPipelines(true);
            updateSizesAndMatrix();
            break;
        case QEvent::UpdateRequest:
            if (m.device && isExposed())
                render();
            break;
        case QEvent::Expose:
            maybeRequestUpdate();
            break;
        case QEvent::PlatformSurface:
            switch (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType())
            {
                case QPlatformSurfaceEvent::SurfaceCreated:
                    m_canCreateSurface = true;
                    break;
                case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
                    resetSurface();
                    m_canCreateSurface = false;
                    break;
            }
            break;
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::FocusAboutToChange:
        case QEvent::Enter:
        case QEvent::Leave:
        case QEvent::TabletMove:
        case QEvent::TabletPress:
        case QEvent::TabletRelease:
        case QEvent::TabletEnterProximity:
        case QEvent::TabletLeaveProximity:
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        case QEvent::InputMethodQuery:
        case QEvent::TouchCancel:
            if (m_passEventsToParent)
                return QCoreApplication::sendEvent(parent(), e);
        case QEvent::Wheel:
            if (m_passEventsToParent)
                return QCoreApplication::sendEvent(const_cast<QWidget *>(QMPlay2Core.getVideoDock()), e);
        default:
            break;
    }
    return QWindow::event(e);
}

}
