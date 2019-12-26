/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

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

extern "C"
{
    #include <libavutil/frame.h>
    #include <libavutil/pixdesc.h>
}

Frame Frame::createEmpty(const Frame &other)
{
    return createEmpty(other.m_frame, true);
}
Frame Frame::createEmpty(const AVFrame *other, bool allocBuffers, AVPixelFormat newPixelFormat)
{
    Frame frame;
    if (other)
    {
        frame.copyAVFrameInfo(other);
        if (newPixelFormat != AV_PIX_FMT_NONE)
            frame.m_frame->format = newPixelFormat;
        frame.obtainPixelFormat();
        if (allocBuffers)
        {
            if (newPixelFormat != AV_PIX_FMT_NONE)
            {
                av_frame_get_buffer(frame.m_frame, 0);
            }
            else
            {
                // OpenGL2 writer doesn't like when linesize changes
                for (int i = frame.numPlanes() - 1; i >= 0; --i)
                {
                    frame.m_frame->linesize[i] = other->linesize[i];
                    frame.m_frame->buf[i] = av_buffer_alloc(other->buf[i] ? other->buf[i]->size : frame.m_frame->linesize[i] * frame.height(i));
                    frame.m_frame->data[i] = frame.m_frame->buf[i]->data;
                }
                frame.m_frame->extended_data = frame.m_frame->data;
            }
        }
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
    bool isLimited)
{
    Frame frame;

    frame.m_frame->width = width;
    frame.m_frame->height = height;
    frame.m_frame->format = pixelFormat;
    if (interlaced)
        frame.setInterlaced(topFieldFirst);
    frame.m_frame->colorspace = colorSpace;
    frame.m_frame->color_range = isLimited ? AVCOL_RANGE_MPEG : AVCOL_RANGE_JPEG;

    frame.obtainPixelFormat();

    return frame;
}

Frame::Frame()
    : m_frame(av_frame_alloc())
{
}
Frame::Frame(AVFrame *avFrame)
    : Frame()
{
    if (!avFrame)
        return;

    av_frame_ref(m_frame, avFrame);
    obtainPixelFormat();
}
Frame::Frame(const Frame &other)
    : Frame()
{
    *this = other;
}
Frame::Frame(Frame &&other)
    : Frame()
{
    *this = std::move(other);
}
Frame::~Frame()
{
    av_frame_free(&m_frame);
}

bool Frame::hasNoData() const
{
    return (m_frame->data[0] == nullptr);
}
bool Frame::isEmpty() const
{
    return (hasNoData() && !isHW());
}
void Frame::clear()
{
    av_frame_unref(m_frame);
    m_pixelFormat = nullptr;
    m_customHwSurface = s_invalidHwSurface;
}

bool Frame::isInterlaced() const
{
    return static_cast<bool>(m_frame->interlaced_frame);
}
bool Frame::isTopFieldFirst() const
{
    return static_cast<bool>(m_frame->top_field_first);
}

void Frame::setInterlaced(bool topFieldFirst)
{
    m_frame->interlaced_frame = 1;
    m_frame->top_field_first = static_cast<int>(topFieldFirst);
}
void Frame::setNoInterlaced()
{
    m_frame->interlaced_frame = 0;
    m_frame->top_field_first = 0;
}

bool Frame::isHW() const
{
    if (m_customHwSurface != s_invalidHwSurface)
        return true;

    switch (m_frame->format)
    {
        case AV_PIX_FMT_DXVA2_VLD:
        case AV_PIX_FMT_VDPAU:
        case AV_PIX_FMT_VAAPI:
        case AV_PIX_FMT_VIDEOTOOLBOX:
        case AV_PIX_FMT_CUDA:
        case AV_PIX_FMT_D3D11:
        case AV_PIX_FMT_DRM_PRIME:
//        case AV_PIX_FMT_OPENCL:
            return true;
    }

    return false;
}
quintptr Frame::hwSurface() const
{
    if (m_customHwSurface != s_invalidHwSurface)
        return m_customHwSurface;

    switch (m_frame->format)
    {
        case AV_PIX_FMT_DXVA2_VLD:
        case AV_PIX_FMT_VDPAU:
        case AV_PIX_FMT_VAAPI:
        case AV_PIX_FMT_VIDEOTOOLBOX:
            return reinterpret_cast<quintptr>(m_frame->data[3]);
    }

    return s_invalidHwSurface;
}

AVPixelFormat Frame::pixelFormat() const
{
    if (isHW())
        return AV_PIX_FMT_NONE;
    return static_cast<AVPixelFormat>(m_frame->format);
}

AVColorSpace Frame::colorSpace() const
{
    if (m_frame->colorspace == AVCOL_SPC_UNSPECIFIED && m_frame->height > 576)
        return AVCOL_SPC_BT709;
    return m_frame->colorspace;
}
bool Frame::isLimited() const
{
    return (m_frame->color_range != AVCOL_RANGE_JPEG);
}

int Frame::chromaShiftW() const
{
    return m_pixelFormat ? m_pixelFormat->log2_chroma_h : 0;
}
int Frame::chromaShiftH() const
{
    return m_pixelFormat ? m_pixelFormat->log2_chroma_w : 0;
}
int Frame::numPlanes() const
{
    return m_pixelFormat ? m_pixelFormat->nb_components : 0;
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

bool Frame::setVideoData(AVBufferRef *buffer[], const int *linesize, bool ref)
{
    if (isHW())
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
        m_frame->data[i] = m_frame->buf[i]->data;
    }
    m_frame->extended_data = m_frame->data;

    return true;
}
bool Frame::setCustomHwSurface(quintptr hwSurface)
{
    if (m_customHwSurface == s_invalidHwSurface && !isEmpty())
        return false;

    m_customHwSurface = hwSurface;
    return true;
}

bool Frame::copyYV12(void *dest, qint32 linesizeLuma, qint32 linesizeChroma) const
{
    if (isHW() || (m_frame->format != AV_PIX_FMT_YUV420P && m_frame->format != AV_PIX_FMT_YUVJ420P))
        return false;

    const qint32 height = this->height(0);
    const qint32 chromaHeight = this->height(1);
    const qint32 wh = linesizeChroma * chromaHeight;
    quint8 *destData = (quint8 *)dest;
    const quint8 *srcData[3] = {
        m_frame->data[0],
        m_frame->data[1],
        m_frame->data[2],
    };
    size_t toCopy = qMin(linesizeLuma, m_frame->linesize[0]);
    for (qint32 i = 0; i < height; ++i)
    {
        memcpy(destData, srcData[0], toCopy);
        srcData[0] += m_frame->linesize[0];
        destData += linesizeLuma;
    }
    toCopy = qMin(linesizeChroma, m_frame->linesize[1]);
    for (qint32 i = 0; i < chromaHeight; ++i)
    {
        memcpy(destData + wh, srcData[1], toCopy);
        memcpy(destData, srcData[2], toCopy);
        srcData[1] += m_frame->linesize[1];
        srcData[2] += m_frame->linesize[2];
        destData += linesizeChroma;
    }

    return true;
}

Frame &Frame::operator =(const Frame &other)
{
    av_frame_unref(m_frame);
    if (!other.m_frame->buf[0] && !other.m_frame->data[0])
        copyAVFrameInfo(other.m_frame);
    else
        av_frame_ref(m_frame, other.m_frame);

    m_pixelFormat = other.m_pixelFormat;
    m_customHwSurface = other.m_customHwSurface;

    return *this;
}
Frame &Frame::operator =(Frame &&other)
{
    av_frame_unref(m_frame);
    av_frame_move_ref(m_frame, other.m_frame);

    qSwap(m_pixelFormat, other.m_pixelFormat);
    qSwap(m_customHwSurface, other.m_customHwSurface);

    return *this;
}

void Frame::copyAVFrameInfo(const AVFrame *other)
{
    m_frame->format = other->format;
    m_frame->width = other->width;
    m_frame->height = other->height;
    m_frame->channels = other->channels;
    m_frame->channel_layout = other->channel_layout;
    m_frame->nb_samples = other->nb_samples;

    av_frame_copy_props(m_frame, other);
}

inline void Frame::obtainPixelFormat()
{
    m_pixelFormat = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(m_frame->format));
}
