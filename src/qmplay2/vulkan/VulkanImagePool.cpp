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

#include "../../qmvk/Device.hpp"
#include "../../qmvk/Image.hpp"

#include "VulkanImagePool.hpp"
#include "VulkanInstance.hpp"

#include <Frame.hpp>

extern "C" {
#include <libavutil/buffer.h>
#include <libavutil/frame.h>
}

namespace QmVk {

struct FrameImage
{
    shared_ptr<Image> image;
    weak_ptr<ImagePool> imagePoolWeak;
};

static void freeImageBuffer(void *opaque, uint8_t *data)
{
    Q_UNUSED(data)
    auto frameImage = reinterpret_cast<FrameImage *>(opaque);
    if (auto imagePool = frameImage->imagePoolWeak.lock())
        imagePool->put(frameImage->image);
    delete frameImage;
}

/**/

class ImagePool::Config
{
public:
    inline bool isLinear() const
    {
        return (paddingHeight != ~0u);
    }

public:
    shared_ptr<Device> device;

    vk::Extent2D size;
    vk::Format format = vk::Format::eUndefined;
    vk::ExternalMemoryHandleTypeFlags exportMemoryTypes;

    // Linear only
    uint32_t paddingHeight = ~0u;
    bool deviceLocal = false;
};

/**/

ImagePool::ImagePool(const shared_ptr<Instance> &instance)
    : m_instance(instance)
{
}
ImagePool::~ImagePool()
{
}

bool ImagePool::takeToAVFrame(
    const vk::Extent2D &size,
    AVFrame *avFrame,
    uint32_t paddingHeight)
{
    Config config;
    config.size = size;
    config.format = Instance::fromFFmpegPixelFormat(avFrame->format);
    config.paddingHeight = paddingHeight;

    auto image = takeCommon(config);
    if (!image)
        return false;

    avFrame->buf[0] = createAVBuffer(image);
    avFrame->opaque = image.get();

    const uint32_t numPlanes = Image::getNumPlanes(config.format);
    for (uint32_t i = 0; i < numPlanes; ++i)
    {
        avFrame->data[i] = image->map<uint8_t>(i);
        avFrame->linesize[i] = image->linesize(i);
    }
    avFrame->extended_data = avFrame->data;

    return true;
}

Frame ImagePool::takeToFrame(
    const vk::Extent2D &size,
    AVFrame *avFrame,
    AVPixelFormat newPixelFormat,
    uint32_t paddingHeight)
{
    return takeToFrameCommon(
        size,
        avFrame,
        (newPixelFormat == AV_PIX_FMT_NONE) ? static_cast<AVPixelFormat>(avFrame->format) : newPixelFormat,
        paddingHeight
    );
}
Frame ImagePool::takeToFrame(
    const Frame &other,
    AVPixelFormat newPixelFormat)
{
    return takeToFrameCommon(
        vk::Extent2D(other.width(), other.height()),
        other,
        (newPixelFormat == AV_PIX_FMT_NONE) ? other.pixelFormat() : newPixelFormat,
        0
    );
}

Frame ImagePool::takeOptimalToFrame(
    const Frame &other,
    AVPixelFormat newPixelFormat)
{
    Config config;
    config.size = vk::Extent2D(other.width(), other.height());
    config.format = Instance::fromFFmpegPixelFormat((newPixelFormat == AV_PIX_FMT_NONE) ? other.pixelFormat() : newPixelFormat);

    auto image = takeCommon(config);
    if (!image)
        return Frame();

    auto frame = Frame::createEmpty(other, false, newPixelFormat);
    setFrameVulkanImage(frame, image, true);
    return frame;
}

shared_ptr<Image> ImagePool::assignLinearDeviceLocalExport(
    Frame &frame,
    vk::ExternalMemoryHandleTypeFlags exportMemoryTypes)
{
    Config config;
    config.size = vk::Extent2D(frame.width(), frame.height());
    config.format = Instance::fromFFmpegPixelFormat(frame.pixelFormat());
    config.paddingHeight = 0;
    config.deviceLocal = true;
    config.exportMemoryTypes = exportMemoryTypes;

    auto image = takeCommon(config);
    if (!image)
        return nullptr;

    setFrameVulkanImage(frame, image, true);
    return image;
}

void ImagePool::put(const shared_ptr<Image> &image)
{
    Config config;
    config.device = image->device();
    config.size = image->size();
    config.format = image->format();
    config.exportMemoryTypes = image->exportMemoryTypes();
    if (image->isLinear())
    {
        config.paddingHeight = image->paddingHeight();
        config.deviceLocal = image->isDeviceLocal();
    }
    else
    {
        Q_ASSERT(!image->useMipmaps());
        Q_ASSERT(image->isStorage());
    }

    auto &images = config.isLinear()
        ? m_imagesLinear
        : m_imagesOptimal
    ;

    vector<shared_ptr<Image>> imagesToClear; // Declare before "lock_guard"
    lock_guard<mutex> locker(m_mutex);
    imagesToClear = takeImagesToClear(config);
    images.push_back(move(image));
}

template<typename TFrame>
Frame ImagePool::takeToFrameCommon(
    const vk::Extent2D &size,
    TFrame otherFrame,
    AVPixelFormat pixelFormat,
    uint32_t paddingHeight)
{
    Config config;
    config.size = size;
    config.format = Instance::fromFFmpegPixelFormat(pixelFormat);
    config.paddingHeight = paddingHeight;

    auto image = takeCommon(config);
    if (!image)
        return Frame();

    AVBufferRef *avBuffers[AV_NUM_DATA_POINTERS] = {
        createAVBuffer(image),
    };

    auto frame = Frame::createEmpty(otherFrame, false, pixelFormat);
    setFrameVulkanImage(frame, image, false);

    uint8_t *data[AV_NUM_DATA_POINTERS] = {};
    int linesizes[AV_NUM_DATA_POINTERS] = {};
    for (int i = frame.numPlanes() - 1; i >= 0; --i)
    {
        data[i] = image->map<uint8_t>(i);
        linesizes[i] = image->linesize(i);
    }
    frame.setVideoData(avBuffers, linesizes, data);

    return frame;
}

shared_ptr<Image> ImagePool::takeCommon(Config &config)
{
    Q_ASSERT(config.format != vk::Format::eUndefined);
    config.device = m_instance->device();

    vector<shared_ptr<Image>> imagesToClear; // Declare before "unique_lock"
    unique_lock<mutex> locker(m_mutex);

    imagesToClear = takeImagesToClear(config);

    if (!config.device)
        return nullptr;

    auto &images = config.isLinear()
        ? m_imagesLinear
        : m_imagesOptimal
    ;

    if (!images.empty())
    {
        auto image = move(images.back());
        images.pop_back();
        return image;
    }

    locker.unlock();
    imagesToClear.clear();

    try
    {
        if (config.isLinear())
        {
            return Image::createLinear(
                config.device,
                config.size,
                config.format,
                config.deviceLocal
                    ? Image::MemoryPropertyPreset::PreferNoHostAccess
                    : Image::MemoryPropertyPreset::PreferCachedHostOnly,
                config.paddingHeight,
                false,
                false,
                config.exportMemoryTypes
            );
        }
        else
        {
            return Image::createOptimal(
                config.device,
                config.size,
                config.format,
                false,
                true,
                config.exportMemoryTypes
            );
        }
    }
    catch (const vk::SystemError &e)
    {
        Q_UNUSED(e)
    }

    return {};
}

AVBufferRef *ImagePool::createAVBuffer(const shared_ptr<Image> &image)
{
    auto frameImage = new FrameImage;
    frameImage->image = image;
    frameImage->imagePoolWeak = shared_from_this();

    auto buffer = av_buffer_create(
        image->map<uint8_t>(),
        image->memorySize(),
        freeImageBuffer,
        frameImage,
        0
    );

    return buffer;
}

vector<shared_ptr<Image>> ImagePool::takeImagesToClear(const Config &config)
{
    // Always clear images when "m_mutex" is unlocked to avoid potential deadlock,
    // because images can rely on it in destructor!

    auto &images = config.isLinear()
        ? m_imagesLinear
        : m_imagesOptimal
    ;

    if (images.empty())
        return {};

    if (
        images[0]->device() != config.device ||
        images[0]->size() != config.size ||
        images[0]->format() != config.format ||
        images[0]->exportMemoryTypes() != config.exportMemoryTypes
    ) {
        return move(images);
    }

    if (config.isLinear() && (
        images[0]->paddingHeight() != config.paddingHeight ||
        (config.deviceLocal && !images[0]->isDeviceLocal())
    )) {
        return move(images);
    }

    return {};
}

void ImagePool::setFrameVulkanImage(
    Frame &frame,
    shared_ptr<Image> &image,
    bool setOnDestroy)
{
    frame.setVulkanImage(image);

    if (!setOnDestroy)
        return;

    frame.setOnDestroyFn([image, imagePoolWeak = weak_ptr<ImagePool>(shared_from_this())] {
        if (auto imagePool = imagePoolWeak.lock())
            imagePool->put(image);
    });
}

}
