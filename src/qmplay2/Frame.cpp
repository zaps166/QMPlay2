/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2025  Błażej Szczygieł

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

#include <Frame.hpp>

#include <QDebug>

#ifdef USE_VULKAN
#   include "../qmvk/Image.hpp"
#endif

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavutil/frame.h>
    #include <libavutil/pixdesc.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
}

#include <cmath>

using namespace std;

AVPixelFormat Frame::convert3PlaneTo2Plane(AVPixelFormat fmt)
{
    switch (fmt)
    {
        case AV_PIX_FMT_YUV420P:
        case AV_PIX_FMT_YUVJ420P:
            return AV_PIX_FMT_NV12;
        case AV_PIX_FMT_YUV420P10:
            return AV_PIX_FMT_P010;
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 2, 100)
        case AV_PIX_FMT_YUV420P12:
            return AV_PIX_FMT_P012;
#endif
        case AV_PIX_FMT_YUV420P16:
            return AV_PIX_FMT_P016;

        case AV_PIX_FMT_YUV422P:
        case AV_PIX_FMT_YUVJ422P:
            return AV_PIX_FMT_NV16;
        case AV_PIX_FMT_YUV422P10:
            return AV_PIX_FMT_NV20; // Is it OK?

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(56, 31, 100)
        case AV_PIX_FMT_YUV444P:
        case AV_PIX_FMT_YUVJ444P:
            return AV_PIX_FMT_NV24;
#endif

        default:
            break;
    }
    return AV_PIX_FMT_NONE;
}
AVPixelFormat Frame::convert2PlaneTo3Plane(AVPixelFormat fmt)
{
    switch (fmt)
    {
        case AV_PIX_FMT_NV12:
            return AV_PIX_FMT_YUV420P;
        case AV_PIX_FMT_P010:
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 2, 100)
        case AV_PIX_FMT_P012:
#endif
        case AV_PIX_FMT_P016:
            return AV_PIX_FMT_YUV420P16;

        case AV_PIX_FMT_NV16:
            return AV_PIX_FMT_YUV422P;
        case AV_PIX_FMT_NV20:
            return AV_PIX_FMT_YUV422P10;

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(56, 31, 100)
        case AV_PIX_FMT_NV24:
            return AV_PIX_FMT_YUV444P;
#endif

        default:
            break;
    }
    return AV_PIX_FMT_NONE;
}

Frame Frame::createEmpty(const Frame &other, bool allocBuffers, AVPixelFormat newPixelFormat)
{
    Frame frame = createEmpty(other.m_frame, allocBuffers, newPixelFormat);
    frame.m_timeBase = other.m_timeBase;
    return frame;
}
Frame Frame::createEmpty(const AVFrame *other, bool allocBuffers, AVPixelFormat newPixelFormat)
{
    Frame frame;

    if (!other)
        return frame;

    frame.copyAVFrameInfo(other);

    const bool hasNewPixelFormat = (newPixelFormat != AV_PIX_FMT_NONE);
    if (hasNewPixelFormat)
        frame.m_frame->format = newPixelFormat;
    frame.obtainPixelFormat(hasNewPixelFormat);

    if (!allocBuffers)
        return frame;

    if (newPixelFormat != AV_PIX_FMT_NONE)
    {
        av_frame_get_buffer(frame.m_frame, 0);
    }
    else
    {
        // OpenGL writer doesn't like when linesize changes
        for (int i = frame.numPlanes() - 1; i >= 0; --i)
        {
            frame.m_frame->linesize[i] = other->linesize[i];
            frame.m_frame->buf[i] = av_buffer_alloc(other->buf[i] ? other->buf[i]->size : frame.m_frame->linesize[i] * frame.height(i));
            frame.m_frame->data[i] = frame.m_frame->buf[i]->data;
        }
        frame.m_frame->extended_data = frame.m_frame->data;
    }

    return frame;
}
Frame Frame::createEmpty(
    int width,
    int height,
    AVPixelFormat pixelFormat,
    bool interlaced,
    bool topFieldFirst,
    AVColorSpace colorSpace,
    bool isLimited,
    AVColorPrimaries colorPrimaries,
    AVColorTransferCharacteristic colorTrc)
{
    Frame frame;

    frame.m_frame->width = width;
    frame.m_frame->height = height;
    frame.m_frame->format = pixelFormat;
    if (interlaced)
        frame.setInterlaced(topFieldFirst);
    frame.m_frame->color_primaries = colorPrimaries;
    frame.m_frame->color_trc = colorTrc;
    frame.m_frame->colorspace = colorSpace;
    frame.m_frame->color_range = isLimited ? AVCOL_RANGE_MPEG : AVCOL_RANGE_JPEG;

    frame.obtainPixelFormat(false);

    return frame;
}

Frame::Frame()
    : m_frame(av_frame_alloc())
{
}
Frame::Frame(AVFrame *avFrame, AVPixelFormat newPixelFormat)
    : Frame()
{
    if (!avFrame)
        return;

    av_frame_ref(m_frame, avFrame);

    const bool hasNewPixelFormat = (newPixelFormat != AV_PIX_FMT_NONE);
    if (hasNewPixelFormat)
        m_pixelFormat = newPixelFormat;
    obtainPixelFormat(hasNewPixelFormat);
}
Frame::Frame(const Frame &other)
    : Frame()
{
    *this = other;
}
Frame::Frame(Frame &&other)
    : Frame()
{
    *this = move(other);
}
Frame::~Frame()
{
    av_frame_free(&m_frame);
}

bool Frame::isEmpty() const
{
    if (m_frame->data[0] == nullptr && !isHW() && !hasCustomData())
    {
#ifdef USE_VULKAN
        if (m_vkImage)
            return false;
#endif
        return true;
    }
    return false;
}
void Frame::clear()
{
    av_frame_unref(m_frame);

    m_timeBase = {};

    m_customData = s_invalidCustomData;
    m_onDestroyFn.reset();

    m_pixelFormat = AV_PIX_FMT_NONE;
    m_pixelFmtDescriptor = nullptr;
    m_isSecondField = false;
    m_isCustomHW = false;
    m_hasBorders = false;
#ifdef USE_VULKAN
    m_vkImage.reset();
#endif
}

void Frame::setTimeBase(const AVRational &timeBase)
{
    m_timeBase = timeBase;
}
AVRational Frame::timeBase() const
{
    return m_timeBase;
}

bool Frame::isTsValid() const
{
    return (m_frame->best_effort_timestamp != AV_NOPTS_VALUE);
}

double Frame::ts() const
{
    return m_frame->best_effort_timestamp * av_q2d(m_timeBase);
}
qint64 Frame::tsInt() const
{
    return m_frame->best_effort_timestamp;
}

void Frame::setTS(double ts)
{
    m_timeBase = {1, 10000};
    m_frame->best_effort_timestamp = round(ts / av_q2d(m_timeBase));
}
void Frame::setTSInt(qint64 ts)
{
    m_frame->best_effort_timestamp = ts;
}

bool Frame::isInterlaced() const
{
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 29, 100)
    return (m_frame->flags & AV_FRAME_FLAG_INTERLACED);
#else
    return static_cast<bool>(m_frame->interlaced_frame);
#endif
}
bool Frame::isTopFieldFirst() const
{
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 29, 100)
    return (m_frame->flags & AV_FRAME_FLAG_TOP_FIELD_FIRST);
#else
    return static_cast<bool>(m_frame->top_field_first);
#endif
}
bool Frame::isSecondField() const
{
    return m_isSecondField;
}

void Frame::setInterlaced(bool topFieldFirst)
{
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 29, 100)
    m_frame->flags |= AV_FRAME_FLAG_INTERLACED;
    if (topFieldFirst)
        m_frame->flags |= AV_FRAME_FLAG_TOP_FIELD_FIRST;
    else
        m_frame->flags &= ~AV_FRAME_FLAG_TOP_FIELD_FIRST;
#else
    m_frame->interlaced_frame = 1;
    m_frame->top_field_first = static_cast<int>(topFieldFirst);
#endif
}
void Frame::setNoInterlaced()
{
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 29, 100)
    m_frame->flags &= ~(AV_FRAME_FLAG_INTERLACED | AV_FRAME_FLAG_TOP_FIELD_FIRST);
#else
    m_frame->interlaced_frame = 0;
    m_frame->top_field_first = 0;
#endif
}
void Frame::setIsSecondField(bool secondField)
{
    m_isSecondField = secondField;
}

bool Frame::hasCPUAccess() const
{
    return (m_frame->data[0] && !isHW());
}

bool Frame::isHW() const
{
    switch (m_frame->format)
    {
        case AV_PIX_FMT_DXVA2_VLD:
        case AV_PIX_FMT_VAAPI:
        case AV_PIX_FMT_VIDEOTOOLBOX:
        case AV_PIX_FMT_D3D11:
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 29, 100)
        case AV_PIX_FMT_VULKAN:
#endif
            return true;
    }
    return m_isCustomHW;
}
quintptr Frame::hwData(int idx) const
{
    if (!m_isCustomHW && isHW())
        return reinterpret_cast<quintptr>(m_frame->data[idx]);
    return s_invalidCustomData;
}

bool Frame::hasCustomData() const
{
    return (m_customData != s_invalidCustomData);
}
quintptr Frame::customData() const
{
    return m_customData;
}
void Frame::setCustomData(quintptr customData, bool isCustomHW)
{
    m_customData = customData;
    m_isCustomHW = (isCustomHW && hasCustomData() && !hasCPUAccess());
}

AVPixelFormat Frame::pixelFormat() const
{
    return m_pixelFormat;
}

AVColorPrimaries Frame::colorPrimaries() const
{
    return m_frame->color_primaries;
}
AVColorTransferCharacteristic Frame::colorTrc() const
{
    return m_frame->color_trc;
}
AVColorSpace Frame::colorSpace() const
{
    return m_frame->colorspace;
}
bool Frame::isLimited() const
{
    return (m_frame->color_range != AVCOL_RANGE_JPEG && !isRGB() && !isGray());
}

const AVMasteringDisplayMetadata *Frame::masteringDisplayMetadata() const
{
    if (AVFrameSideData *sideData = av_frame_get_side_data(m_frame, AV_FRAME_DATA_MASTERING_DISPLAY_METADATA))
        return reinterpret_cast<const AVMasteringDisplayMetadata *>(sideData->data);
    return nullptr;
}

bool Frame::isGray() const
{
    if (m_pixelFmtDescriptor)
        return (m_pixelFmtDescriptor->nb_components == 1);
    return false;
}
bool Frame::isPlannar() const
{
    if (m_pixelFmtDescriptor)
        return (m_pixelFmtDescriptor->flags & AV_PIX_FMT_FLAG_PLANAR);
    return false;
}
bool Frame::isRGB() const
{
    if (m_pixelFmtDescriptor)
        return (m_pixelFmtDescriptor->flags & AV_PIX_FMT_FLAG_RGB) || (m_frame->colorspace == AVCOL_SPC_RGB);
    return false;
}

int Frame::chromaShiftW() const
{
    return m_pixelFmtDescriptor ? m_pixelFmtDescriptor->log2_chroma_w : 0;
}
int Frame::chromaShiftH() const
{
    return m_pixelFmtDescriptor ? m_pixelFmtDescriptor->log2_chroma_h : 0;
}
int Frame::numPlanes() const
{
    return m_pixelFmtDescriptor ? av_pix_fmt_count_planes(m_pixelFormat) : 0;
}

int Frame::depth() const
{
    if (m_pixelFmtDescriptor)
        return m_pixelFmtDescriptor->comp[0].depth;
    return 0;
}
int Frame::paddingBits() const
{
    if (m_pixelFmtDescriptor)
        return (m_pixelFmtDescriptor->comp[0].step << 3) - m_pixelFmtDescriptor->comp[0].depth - m_pixelFmtDescriptor->comp[0].shift;
    return 0;
}

int *Frame::linesize() const
{
    return m_frame->linesize;
}
int Frame::linesize(int plane) const
{
    return m_frame->linesize[plane];
}
int Frame::width(int plane) const
{
    if (plane == 0)
        return m_frame->width;
    return FF_CEIL_RSHIFT(m_frame->width, chromaShiftW());
}
int Frame::height(int plane) const
{
    if (plane == 0)
        return m_frame->height;
    return FF_CEIL_RSHIFT(m_frame->height, chromaShiftH());
}

AVRational Frame::sampleAspectRatio() const
{
    return m_frame->sample_aspect_ratio;
}

const quint8 *Frame::constData(int plane) const
{
    return m_frame->data[plane];
}
quint8 *Frame::data(int plane)
{
    if (m_frame->buf[plane])
    {
        av_buffer_make_writable(&m_frame->buf[plane]);
        m_frame->data[plane] = m_frame->buf[plane]->data;
    }
    return m_frame->data[plane];
}
quint8 **Frame::dataArr()
{
    return m_frame->data;
}

bool Frame::setVideoData(
    AVBufferRef *buffer[],
    const int *linesize,
    uint8_t *data[],
    bool ref)
{
    if (isHW() || (ref && data))
        return false;

    for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i)
    {
        m_frame->data[i] = nullptr;
        av_buffer_unref(&m_frame->buf[i]);
        m_frame->linesize[i] = 0;
    }

    for (int i = numPlanes() - 1; i >= 0; --i)
    {
        m_frame->linesize[i] = linesize[i];
        m_frame->buf[i] = ref ? av_buffer_ref(buffer[i]) : buffer[i];
        m_frame->data[i] = data ? data[i] : m_frame->buf[i]->data;
    }
    m_frame->extended_data = m_frame->data;

    return true;
}

void Frame::setOnDestroyFn(const Frame::OnDestroyFn &onDestroyFn)
{
    if (onDestroyFn && !m_onDestroyFn)
    {
        m_onDestroyFn = shared_ptr<OnDestroyFn>(new OnDestroyFn(onDestroyFn), [](OnDestroyFn *ptr) {
            if (*ptr)
                (*ptr)();
            delete ptr;
        });
    }
    else if (m_onDestroyFn)
    {
        *m_onDestroyFn = onDestroyFn;
    }
}

bool Frame::copyYV12(void *dest, qint32 linesizeLuma, qint32 linesizeChroma) const
{
    if (m_pixelFormat != AV_PIX_FMT_YUV420P && m_pixelFormat != AV_PIX_FMT_YUVJ420P)
        return false;

    uint8_t *destData[4];
    destData[0] = reinterpret_cast<uint8_t *>(dest);
    destData[2] = destData[0] + linesizeLuma * height(0);
    destData[1] = destData[2] + linesizeChroma * height(1);
    destData[3] = nullptr;

    int destLinesize[4] {
        linesizeLuma,
        linesizeChroma,
        linesizeChroma,
        0
    };

    return copyData(destData, destLinesize);
}

Frame Frame::downloadHwData(SwsContext **swsCtx, const AVPixelFormats &supportedPixelFormats) const
{
    Frame downloaded;
    if (isHW() && m_frame->hw_frames_ctx)
    {
        auto dstFrame = av_frame_alloc();
        dstFrame->format = convert2PlaneTo3Plane(m_pixelFormat);

        bool found = false;
        AVPixelFormat *formats = nullptr;
        if (av_hwframe_transfer_get_formats(m_frame->hw_frames_ctx, AV_HWFRAME_TRANSFER_DIRECTION_FROM, &formats, 0) == 0)
        {
            for (int i = 0; formats[i] != AV_PIX_FMT_NONE; ++i)
            {
                if (formats[i] == dstFrame->format)
                {
                    found = true;
                    break;
                }
            }
            av_free(formats);
        }
        if (!found)
        {
            dstFrame->format = AV_PIX_FMT_NONE;
        }

        const bool convert =
            swsCtx &&
            !supportedPixelFormats.isEmpty() &&
            !supportedPixelFormats.contains(found ? static_cast<AVPixelFormat>(dstFrame->format) : m_pixelFormat)
        ;

        if (av_hwframe_transfer_data(dstFrame, m_frame, 0) == 0)
        {
            AVFrame *dstFrame2 = nullptr;

            if (convert)
            {
                dstFrame2 = av_frame_alloc();
                dstFrame2->width = dstFrame->width;
                dstFrame2->height = dstFrame->height;
                dstFrame2->format = AV_PIX_FMT_YUV420P;
                if (av_frame_get_buffer(dstFrame2, 0) == 0)
                {
                    *swsCtx = sws_getCachedContext(
                        *swsCtx,
                        dstFrame->width,
                        dstFrame->height,
                        m_pixelFormat,
                        dstFrame2->width,
                        dstFrame2->height,
                        static_cast<AVPixelFormat>(dstFrame2->format),
                        SWS_POINT,
                        nullptr,
                        nullptr,
                        nullptr
                    );
                    if (*swsCtx)
                    {
                        sws_scale(
                            *swsCtx,
                            dstFrame->data,
                            dstFrame->linesize,
                            0,
                            dstFrame->height,
                            dstFrame2->data,
                            dstFrame2->linesize
                        );
                    }
                    downloaded = Frame(dstFrame2);
                }
            }

            if (downloaded.isEmpty())
                downloaded = Frame(dstFrame);
            av_frame_copy_props(downloaded.m_frame, m_frame);
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 29, 100)
            downloaded.m_frame->flags = m_frame->flags;
#else
            downloaded.m_frame->key_frame = m_frame->key_frame;
#endif
            downloaded.setTimeBase(m_timeBase);

            if (dstFrame2)
                av_frame_free(&dstFrame2);
        }

        av_frame_free(&dstFrame);
    }
    return downloaded;
}

#ifdef USE_VULKAN
bool Frame::copyToVulkanImage(const shared_ptr<QmVk::Image> &image) const
{
    if (!image->isLinear() || !image->isHostVisible())
        return false;

    const int vkNumPlanes = image->numPlanes();
    if (numPlanes() != vkNumPlanes)
        return false;

    uint8_t *dstData[4] = {};
    int dstLinesize[4] = {};
    for (int i = 0; i < vkNumPlanes; ++i)
    {
        dstData[i] = image->map<uint8_t>(i);
        dstLinesize[i] = image->linesize(i);
    }
    copyData(dstData, dstLinesize);

    return true;
}
#endif

Frame &Frame::operator =(const Frame &other)
{
    av_frame_unref(m_frame);
    if (!other.m_frame->buf[0] && !other.m_frame->data[0])
    {
        copyAVFrameInfo(other.m_frame);
        memcpy(m_frame->linesize, other.m_frame->linesize, sizeof(AVFrame::linesize));
    }
    else
    {
        av_frame_ref(m_frame, other.m_frame);
    }

    m_timeBase = other.m_timeBase;

    m_customData = other.m_customData;
    m_onDestroyFn = other.m_onDestroyFn;

    m_pixelFormat = other.m_pixelFormat;
    m_pixelFmtDescriptor = other.m_pixelFmtDescriptor;
    m_isSecondField = other.m_isSecondField;
    m_isCustomHW = other.m_isCustomHW;
    m_hasBorders = other.m_hasBorders;
#ifdef USE_VULKAN
    m_vkImage = other.m_vkImage;
#endif

    return *this;
}
Frame &Frame::operator =(Frame &&other)
{
    av_frame_unref(m_frame);
    av_frame_move_ref(m_frame, other.m_frame);

    qSwap(m_timeBase, other.m_timeBase);

    qSwap(m_customData, other.m_customData);
    m_onDestroyFn = move(other.m_onDestroyFn);

    qSwap(m_pixelFormat, other.m_pixelFormat);
    qSwap(m_pixelFmtDescriptor, other.m_pixelFmtDescriptor);
    qSwap(m_isSecondField, other.m_isSecondField);
    qSwap(m_isCustomHW, other.m_isCustomHW);
    qSwap(m_hasBorders, other.m_hasBorders);
#ifdef USE_VULKAN
    qSwap(m_vkImage, other.m_vkImage);
#endif

    return *this;
}

bool Frame::copyDataInternal(void *dest[4], int linesize[4]) const
{
    if (!hasCPUAccess())
        return false;

    av_image_copy(
        reinterpret_cast<uint8_t **>(dest),
        linesize,
        const_cast<const uint8_t **>(m_frame->data),
        m_frame->linesize,
        m_pixelFormat,
        m_frame->width,
        m_frame->height
    );

    return true;
}

void Frame::copyAVFrameInfo(const AVFrame *other)
{
    m_frame->format = other->format;
    m_frame->width = other->width;
    m_frame->height = other->height;
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 28, 100)
    m_frame->ch_layout = other->ch_layout;
#else
    m_frame->channels = other->channels;
    m_frame->channel_layout = other->channel_layout;
#endif
    m_frame->nb_samples = other->nb_samples;

    av_frame_copy_props(m_frame, other);
}

void Frame::obtainPixelFormat(bool checkForYUVJ)
{
    if (m_pixelFormat == AV_PIX_FMT_NONE)
        m_pixelFormat = static_cast<AVPixelFormat>(m_frame->format);
    m_pixelFmtDescriptor = av_pix_fmt_desc_get(m_pixelFormat);

    if (checkForYUVJ && (m_pixelFmtDescriptor->flags & AV_PIX_FMT_FLAG_PLANAR) && strstr(m_pixelFmtDescriptor->name, "yuvj") != nullptr)
        m_frame->color_range = AVCOL_RANGE_JPEG;
}
