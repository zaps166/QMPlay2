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

enum class QMPlay2PixelFormat
{
    None = -1,

    YUV420P,
    YUVJ420P,
    YUV422P,
    YUVJ422P,
    YUV444P,
    YUVJ444P,

    YUV410P,
    YUV411P,
    YUVJ411P,
    YUV440P,
    YUVJ440P,

    Count,
};
using QMPlay2PixelFormats = QVector<QMPlay2PixelFormat>;

enum class QMPlay2ColorSpace
{
    Unknown = -1,

    BT709,
    BT470BG,
    SMPTE170M,
    SMPTE240M,
    BT2020,
};
struct LumaCoefficients
{
    float cR, cG, cB;
};

namespace QMPlay2PixelFormatConvert {

QMPLAY2SHAREDLIB_EXPORT int toFFmpeg(QMPlay2PixelFormat pixFmt);
QMPLAY2SHAREDLIB_EXPORT QMPlay2PixelFormat fromFFmpeg(int pixFmt);

QMPLAY2SHAREDLIB_EXPORT QMPlay2ColorSpace fromFFmpegColorSpace(int colorSpace, int h);
QMPLAY2SHAREDLIB_EXPORT LumaCoefficients getLumaCoeff(QMPlay2ColorSpace colorSpace);

}
