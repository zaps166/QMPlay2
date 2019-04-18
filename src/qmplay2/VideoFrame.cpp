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

#include <VideoFrame.hpp>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavutil/common.h>
}

qint32 VideoFrameSize::chromaWidth() const
{
    return FF_CEIL_RSHIFT(width, chromaShiftW);
}
qint32 VideoFrameSize::chromaHeight() const
{
    return FF_CEIL_RSHIFT(height, chromaShiftH);
}

QMPlay2PixelFormat VideoFrameSize::getFormat() const
{
    switch ((chromaShiftW << 8) | chromaShiftH)
    {
        case 0x0000:
            return QMPlay2PixelFormat::YUV444P;
        case 0x0001:
            return QMPlay2PixelFormat::YUV440P;
        case 0x0100:
            return QMPlay2PixelFormat::YUV422P;
        case 0x0200:
            return QMPlay2PixelFormat::YUV411P;
        case 0x0202:
            return QMPlay2PixelFormat::YUV410P;
    }
    return QMPlay2PixelFormat::YUV420P;
}

void VideoFrameSize::clear()
{
    width = height = 0;
    chromaShiftW = chromaShiftH = 0;
}

/**/

AVFrameHolder::AVFrameHolder(AVFrame *frame)
    : m_frame(av_frame_alloc())
{
    av_frame_ref(m_frame, frame);
}
AVFrameHolder::AVFrameHolder(AVFrameHolder &&other)
{
    *this = std::move(other);
}
AVFrameHolder::AVFrameHolder(const AVFrameHolder &other)
{
    *this = other;
}
AVFrameHolder::~AVFrameHolder()
{
    av_frame_free(&m_frame);
}

AVFrameHolder &AVFrameHolder::operator =(AVFrameHolder &&other)
{
    qSwap(m_frame, other.m_frame);
    return *this;
}
AVFrameHolder &AVFrameHolder::operator =(const AVFrameHolder &other)
{
    if (other.m_frame)
        *this = other.m_frame;
    return *this;
}

/**/

VideoFrame::VideoFrame(const VideoFrameSize &size, AVBufferRef *bufferRef[], const qint32 newLinesize[], bool interlaced, bool tff) :
    size(size),
    interlaced(interlaced),
    tff(tff)
{
    for (qint32 p = 0; p < 3 && bufferRef[p]; ++p)
    {
        linesize[p] = newLinesize[p];
        buffer[p].assign(bufferRef[p], linesize[p] * size.getHeight(p));
        bufferRef[p] = nullptr;
    }
}
VideoFrame::VideoFrame(const VideoFrameSize &size, const qint32 newLinesize[], bool interlaced, bool tff) :
    size(size),
    interlaced(interlaced),
    tff(tff)
{
    for (qint32 p = 0; p < 3; ++p)
    {
        linesize[p] = newLinesize[p];
        buffer[p].resize(linesize[p] * size.getHeight(p));
    }
}
VideoFrame::VideoFrame(const VideoFrameSize &size, quintptr surfaceId, bool interlaced, bool tff) :
    size(size),
    interlaced(interlaced),
    tff(tff),
    surfaceId(surfaceId)
{}
VideoFrame::VideoFrame()
{}
VideoFrame::~VideoFrame()
{}

void VideoFrame::setAVFrame(AVFrame *frame)
{
    m_frameRef = frame;
}

void VideoFrame::clear()
{
    for (qint32 i = 0; i < 3; ++i)
    {
        buffer[i].clear();
        linesize[i] = 0;
    }
    setNoInterlaced();
    surfaceId = 0;
    size.clear();
    m_frameRef = nullptr;
}

void VideoFrame::copy(void *dest, qint32 linesizeLuma, qint32 linesizeChroma) const
{
    const qint32 chromaHeight = size.chromaHeight();
    const qint32 wh = linesizeChroma * chromaHeight;
    quint8 *destData = (quint8 *)dest;
    const quint8 *srcData[3] = {
        buffer[0].data(),
        buffer[1].data(),
        buffer[2].data()
    };
    size_t toCopy = qMin(linesizeLuma, linesize[0]);
    for (qint32 i = 0; i < size.height; ++i)
    {
        memcpy(destData, srcData[0], toCopy);
        srcData[0] += linesize[0];
        destData += linesizeLuma;
    }
    toCopy = qMin(linesizeChroma, linesize[1]);
    for (qint32 i = 0; i < chromaHeight; ++i)
    {
        memcpy(destData + wh, srcData[1], toCopy);
        memcpy(destData, srcData[2], toCopy);
        srcData[1] += linesize[1];
        srcData[2] += linesize[2];
        destData += linesizeChroma;
    }
}
