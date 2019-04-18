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

#include <PixelFormats.hpp>

extern "C"
{
    #include <libavutil/pixfmt.h>
}

namespace QMPlay2PixelFormatConvert {

int toFFmpeg(QMPlay2PixelFormat pixFmt)
{
    switch (pixFmt)
    {
        case QMPlay2PixelFormat::YUV420P:
            return AV_PIX_FMT_YUV420P;
        case QMPlay2PixelFormat::YUV422P:
            return AV_PIX_FMT_YUV422P;
        case QMPlay2PixelFormat::YUV444P:
            return AV_PIX_FMT_YUV444P;
        case QMPlay2PixelFormat::YUV410P:
            return AV_PIX_FMT_YUV410P;
        case QMPlay2PixelFormat::YUV411P:
            return AV_PIX_FMT_YUV411P;
        case QMPlay2PixelFormat::YUV440P:
            return AV_PIX_FMT_YUV440P;
        default:
            break;
    }
    return AV_PIX_FMT_NONE;
}
QMPlay2PixelFormat fromFFmpeg(int pixFmt)
{
    switch (pixFmt)
    {
        case AV_PIX_FMT_YUV420P:
            return QMPlay2PixelFormat::YUV420P;
        case AV_PIX_FMT_YUV422P:
            return QMPlay2PixelFormat::YUV422P;
        case AV_PIX_FMT_YUV444P:
            return QMPlay2PixelFormat::YUV444P;
        case AV_PIX_FMT_YUV410P:
            return QMPlay2PixelFormat::YUV410P;
        case AV_PIX_FMT_YUV411P:
            return QMPlay2PixelFormat::YUV411P;
        case AV_PIX_FMT_YUV440P:
            return QMPlay2PixelFormat::YUV440P;
    }
    return QMPlay2PixelFormat::None;
}

}
