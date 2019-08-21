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

#pragma once

#include <Buffer.hpp>
#include <PixelFormats.hpp>

class QMPLAY2SHAREDLIB_EXPORT VideoFrameSize
{
public:
    inline VideoFrameSize(qint32 width, qint32 height, quint8 chromaShiftW = 1, quint8 chromaShiftH = 1) :
        width(width), height(height),
        chromaShiftW(chromaShiftW), chromaShiftH(chromaShiftH)
    {}
    inline VideoFrameSize()
    {
        clear();
    }

    qint32 chromaWidth() const;
    qint32 chromaHeight() const;

    inline qint32 getWidth(qint32 plane) const
    {
        return plane ? chromaWidth() : width;
    }
    inline qint32 getHeight(qint32 plane) const
    {
        return plane ? chromaHeight() : height;
    }

    void clear();

    qint32 width, height;
    quint8 chromaShiftW, chromaShiftH;
};

/**/

struct AVFrame;

class QMPLAY2SHAREDLIB_EXPORT AVFrameHolder
{
public:
    AVFrameHolder() = default;
    AVFrameHolder(std::nullptr_t) {}
    AVFrameHolder(AVFrame *frame);
    AVFrameHolder(AVFrameHolder &&other);
    AVFrameHolder(const AVFrameHolder &other);
    ~AVFrameHolder();

    AVFrameHolder &operator =(AVFrameHolder &&other);
    AVFrameHolder &operator =(const AVFrameHolder &other);

private:
    AVFrame *m_frame = nullptr;
};

/**/

class QMPLAY2SHAREDLIB_EXPORT VideoFrame
{
public:
    VideoFrame(const VideoFrameSize &size, AVBufferRef *bufferRef[], const qint32 newLinesize[], bool interlaced, bool tff);
    VideoFrame(const VideoFrameSize &size, const qint32 newLinesize[], bool interlaced = false, bool tff = false);
    VideoFrame(const VideoFrameSize &size, quintptr surfaceId, bool interlaced, bool tff);
    VideoFrame();
    ~VideoFrame();

    inline bool hasNoData() const
    {
        return buffer[0].isEmpty();
    }
    inline bool isEmpty() const
    {
        return hasNoData() && surfaceId == 0;
    }

    inline void setNoInterlaced()
    {
        interlaced = tff = false;
    }

    QMPlay2PixelFormat getFormat() const;

    void setAVFrame(AVFrame *frame);

    void clear();

    void copy(void *dest, qint32 linesizeLuma, qint32 linesizeChroma) const;

    VideoFrameSize size;
    Buffer buffer[3];
    qint32 linesize[3] = {};
    bool interlaced = false;
    bool tff = false;
    bool limited = false;
    QMPlay2ColorSpace colorSpace = QMPlay2ColorSpace::Unknown;
    quintptr surfaceId = 0;

private:
    AVFrameHolder m_frameRef;
};
