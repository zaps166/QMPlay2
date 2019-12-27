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

#include <QMPlay2Lib.hpp>

#include <QVector>

extern "C" {
    #include <libavutil/pixfmt.h>
    #include <libavutil/rational.h>
}

struct AVPixFmtDescriptor;
struct AVBufferRef;
struct AVFrame;

using AVPixelFormats = QVector<AVPixelFormat>;

class QMPLAY2SHAREDLIB_EXPORT Frame
{
public:
    static constexpr quintptr s_invalidHwSurface = ~static_cast<quintptr>(0);

public:
    static Frame createEmpty(const Frame &other);
    static Frame createEmpty(const AVFrame *other, bool allocBuffers, AVPixelFormat newPixelFormat = AV_PIX_FMT_NONE);
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
    explicit Frame(AVFrame *avFrame);
    Frame(const Frame &other);
    Frame(Frame &&other);
    ~Frame();

    bool isEmpty() const;
    void clear();

    void setTimeBase(double timeBase);

    bool isTsValid() const;
    double ts() const;
    void setTS(double ts);

public: // Video
    bool isInterlaced() const;
    bool isTopFieldFirst() const;

    void setInterlaced(bool topFieldFirst);
    void setNoInterlaced();

    bool isHW() const;
    quintptr hwSurface() const;

    AVPixelFormat pixelFormat() const;

    AVColorSpace colorSpace() const;
    bool isLimited() const;

    int chromaShiftW() const;
    int chromaShiftH() const;
    int numPlanes() const;

    int *linesize() const;
    int linesize(int plane) const;
    int width(int plane = 0) const;
    int height(int plane = 0) const;

    AVRational sampleAspectRatio() const;

    const quint8 *constData(int plane = 0) const;
    quint8 *data(int plane = 0);

    bool setVideoData(AVBufferRef *buffer[], const int *linesize, bool ref);
    bool setCustomHwSurface(quintptr hwSurface);

    bool copyYV12(void *dest, qint32 linesizeLuma, qint32 linesizeChroma) const;

public: // Operators
    Frame &operator =(const Frame &other);
    Frame &operator =(Frame &&other);

private:
    void copyAVFrameInfo(const AVFrame *other);

    inline void obtainPixelFormat();

private:
    AVFrame *m_frame = nullptr;
    double m_timeBase = qQNaN();

    // Video only
    const AVPixFmtDescriptor *m_pixelFormat = nullptr;
    quintptr m_customHwSurface = s_invalidHwSurface;
};
