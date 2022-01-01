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

#include <QMPlay2Lib.hpp>

#include <vulkan/vulkan.hpp>

#include <mutex>

extern "C" {
#include <libavutil/pixfmt.h>
}

struct AVBufferRef;
struct AVFrame;
class Frame;

namespace QmVk {

using namespace std;

class Instance;
class Image;

class QMPLAY2SHAREDLIB_EXPORT ImagePool : public enable_shared_from_this<ImagePool>
{
    class Config;

public:
    ImagePool(const shared_ptr<Instance> &instance);
    ~ImagePool();

public:
    inline shared_ptr<Instance> instance() const;

    bool takeToAVFrame(
        const vk::Extent2D &size,
        AVFrame *avFrame,
        uint32_t paddingHeight = 0
    );

    Frame takeToFrame(
        const vk::Extent2D &size,
        AVFrame *avFrame,
        AVPixelFormat newPixelFormat = AV_PIX_FMT_NONE,
        uint32_t paddingHeight = 0
    );
    Frame takeToFrame(
        const Frame &other,
        AVPixelFormat newPixelFormat = AV_PIX_FMT_NONE
    );

    Frame takeOptimalToFrame(
        const Frame &other,
        AVPixelFormat newPixelFormat = AV_PIX_FMT_NONE
    );

    shared_ptr<Image> assignLinearDeviceLocalExport(
        Frame &frame,
        vk::ExternalMemoryHandleTypeFlags exportMemoryTypes
    );

    void put(const shared_ptr<Image> &image);

private:
    template<typename TFrame>
    Frame takeToFrameCommon(
        const vk::Extent2D &size,
        TFrame otherFrame,
        AVPixelFormat newPixelFormat,
        uint32_t paddingHeight
    );

    shared_ptr<Image> takeCommon(Config &config);

    AVBufferRef *createAVBuffer(const shared_ptr<Image> &image);

    vector<shared_ptr<Image>> takeImagesToClear(const Config &config);

    void setFrameVulkanImage(
        Frame &frame,
        shared_ptr<Image> &image,
        bool setOnDestroy
    );

private:
    const shared_ptr<Instance> m_instance;

    vector<shared_ptr<Image>> m_imagesLinear;
    vector<shared_ptr<Image>> m_imagesOptimal;

    mutex m_mutex;
};

/* Inline implementation */

shared_ptr<Instance> ImagePool::instance() const
{
    return m_instance;
}

}
