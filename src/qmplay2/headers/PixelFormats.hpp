/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2018  Błażej Szczygieł

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
    YUV422P,
    YUV444P,

    YUV410P,
    YUV411P,
    YUV440P,

    Count,
};
using QMPlay2PixelFormats = QVector<QMPlay2PixelFormat>;

namespace QMPlay2PixelFormatConvert {

QMPLAY2SHAREDLIB_EXPORT int toFFmpeg(QMPlay2PixelFormat pixFmt);
QMPLAY2SHAREDLIB_EXPORT QMPlay2PixelFormat fromFFmpeg(int pixFmt);

}
