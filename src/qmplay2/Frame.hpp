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

#include <QVector>

#include <functional>
#include <memory>

extern "C" {
    #include <libavutil/pixfmt.h>
    #include <libavutil/rational.h>
}

#ifdef USE_VULKAN
namespace QmVk {
class Image;
}
#endif

struct AVPixFmtDescriptor;
struct AVBufferRef;
struct AVFrame;

using AVPixelFormats = QVector<AVPixelFormat>;

class QMPLAY2SHAREDLIB_EXPORT Frame
{
public:
    static constexpr quintptr s_invalidCustomData = ~static_cast<quintptr>(0);
    using OnDestroyFn = std::function<void()>;

public:
    static AVPixelFormat convert3PlaneTo2Plane(AVPixelFormat fmt);
    static AVPixelFormat convert2PlaneTo3Plane(AVPixelFormat fmt);

public:
    static Frame createEmpty(
        const Frame &other,
        bool allocBuffers,
        AVPixelFormat newPixelFormat = AV_PIX_FMT_NONE
    );
    static Frame createEmpty(
        const AVFrame *other,
        bool allocBuffers,
        AVPixelFormat newPixelFormat = AV_PIX_FMT_NONE
    );
    static Frame createEmpty(
        int width,
        int height,
        AVPixelFormat pixelFormat,
        bool interlaced,
        bool topFieldFirst,
        AVColorSpace colorSpace,
        bool isLimited
    );

public:
    Frame();
    explicit Frame(AVFrame *avFrame, AVPixelFormat newPixelFormat = AV_PIX_FMT_NONE);
    Frame(const Frame &other);
    Frame(Frame &&other);
    ~Frame();

    bool isEmpty() const;
    void clear();

    void setTimeBase(const AVRational &timeBase);
    AVRational timeBase() const;

    bool isTsValid() const;

    double ts() const;
    qint64 tsInt() const;

    void setTS(double ts);
    void setTSInt(qint64 ts);

public: // Video
    bool isInterlaced() const;
    bool isTopFieldFirst() const;
    bool isSecondField() const;

    void setInterlaced(bool topFieldFirst);
    void setNoInterlaced();
    void setIsSecondField(bool secondField);

    bool hasCPUAccess() const;

    bool isHW() const;
    quintptr hwData(int idx = 3) const;

    bool hasCustomData() const;
    quintptr customData() const;
    void setCustomData(quintptr customData);

    AVPixelFormat pixelFormat() const;

    AVColorSpace colorSpace() const;
    bool isLimited() const;

    bool isGray() const;
    bool isPlannar() const;
    bool isRGB() const;

    int chromaShiftW() const;
    int chromaShiftH() const;
    int numPlanes() const;

    int paddingBits() const;

    int *linesize() const;
    int linesize(int plane) const;
    int width(int plane = 0) const;
    int height(int plane = 0) const;

    AVRational sampleAspectRatio() const;

    const quint8 *constData(int plane = 0) const;
    quint8 *data(int plane = 0);

    bool setVideoData(
        AVBufferRef *buffer[],
        const int *linesize,
        uint8_t *data[] = nullptr,
        bool ref = false
    );

    void setOnDestroyFn(const OnDestroyFn &onDestroyFn);

    template<typename D, typename L>
    inline bool copyData(D *dest[4], L linesize[4]) const;

    bool copyYV12(void *dest, qint32 linesizeLuma, qint32 linesizeChroma) const;

#ifdef USE_VULKAN
    inline std::shared_ptr<QmVk::Image> vulkanImage() const;
    inline void setVulkanImage(const std::shared_ptr<QmVk::Image> &image);

    bool copyToVulkanImage(const std::shared_ptr<QmVk::Image> &image) const;
#endif

public: // Operators
    Frame &operator =(const Frame &other);
    Frame &operator =(Frame &&other);

private:
    bool copyDataInternal(void *dest[4], int linesize[4]) const;

    void copyAVFrameInfo(const AVFrame *other);

    void obtainPixelFormat(bool checkForYUVJ);

private:
    AVFrame *m_frame = nullptr;
    AVRational m_timeBase = {};

    quintptr m_customData = s_invalidCustomData;
    std::shared_ptr<OnDestroyFn> m_onDestroyFn;

    // Video only
    AVPixelFormat m_pixelFormat = AV_PIX_FMT_NONE;
    const AVPixFmtDescriptor *m_pixelFmtDescriptor = nullptr;
    bool m_isSecondField = false;
#ifdef USE_VULKAN
    std::shared_ptr<QmVk::Image> m_vkImage;
#endif
};

/* Inline implementation */

template<typename D, typename L>
bool Frame::copyData(D *dest[4], L linesize[4]) const
{
    static_assert(sizeof(L) == sizeof(int), "Linesize type size missmatch");
    return copyDataInternal(reinterpret_cast<void **>(dest), reinterpret_cast<int *>(linesize));
}

#ifdef USE_VULKAN
std::shared_ptr<QmVk::Image> Frame::vulkanImage() const
{
    return m_vkImage;
}
void Frame::setVulkanImage(const std::shared_ptr<QmVk::Image> &image)
{
    m_vkImage = image;
}
#endif
