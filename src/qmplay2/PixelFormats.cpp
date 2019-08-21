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
        case QMPlay2PixelFormat::YUVJ420P:
            return AV_PIX_FMT_YUVJ420P;
        case QMPlay2PixelFormat::YUV422P:
            return AV_PIX_FMT_YUV422P;
        case QMPlay2PixelFormat::YUVJ422P:
            return AV_PIX_FMT_YUVJ422P;
        case QMPlay2PixelFormat::YUV444P:
            return AV_PIX_FMT_YUV444P;
        case QMPlay2PixelFormat::YUVJ444P:
            return AV_PIX_FMT_YUVJ444P;
        case QMPlay2PixelFormat::YUV410P:
            return AV_PIX_FMT_YUV410P;
        case QMPlay2PixelFormat::YUV411P:
            return AV_PIX_FMT_YUV411P;
        case QMPlay2PixelFormat::YUVJ411P:
            return AV_PIX_FMT_YUVJ411P;
        case QMPlay2PixelFormat::YUV440P:
            return AV_PIX_FMT_YUV440P;
        case QMPlay2PixelFormat::YUVJ440P:
            return AV_PIX_FMT_YUVJ440P;
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
        case AV_PIX_FMT_YUVJ420P:
            return QMPlay2PixelFormat::YUVJ420P;
        case AV_PIX_FMT_YUV422P:
            return QMPlay2PixelFormat::YUV422P;
        case AV_PIX_FMT_YUVJ422P:
            return QMPlay2PixelFormat::YUVJ422P;
        case AV_PIX_FMT_YUV444P:
            return QMPlay2PixelFormat::YUV444P;
        case AV_PIX_FMT_YUVJ444P:
            return QMPlay2PixelFormat::YUVJ444P;
        case AV_PIX_FMT_YUV410P:
            return QMPlay2PixelFormat::YUV410P;
        case AV_PIX_FMT_YUV411P:
            return QMPlay2PixelFormat::YUV411P;
        case AV_PIX_FMT_YUVJ411P:
            return QMPlay2PixelFormat::YUVJ411P;
        case AV_PIX_FMT_YUV440P:
            return QMPlay2PixelFormat::YUV440P;
        case AV_PIX_FMT_YUVJ440P:
            return QMPlay2PixelFormat::YUVJ440P;
    }
    return QMPlay2PixelFormat::None;
}

QMPlay2ColorSpace fromFFmpegColorSpace(int colorSpace, int h)
{
    switch (colorSpace)
    {
        case AVCOL_SPC_UNSPECIFIED:
            if (h > 576)
                return QMPlay2ColorSpace::BT709;
            break;
        case AVCOL_SPC_BT709:
            return QMPlay2ColorSpace::BT709;
        case AVCOL_SPC_BT470BG:
            return QMPlay2ColorSpace::BT470BG;
        case AVCOL_SPC_SMPTE170M:
            return QMPlay2ColorSpace::SMPTE170M;
        case AVCOL_SPC_SMPTE240M:
            return QMPlay2ColorSpace::SMPTE240M;
        case AVCOL_SPC_BT2020_CL:
        case AVCOL_SPC_BT2020_NCL:
            return QMPlay2ColorSpace::BT2020;
    }
    return QMPlay2ColorSpace::Unknown;
}
LumaCoefficients getLumaCoeff(QMPlay2ColorSpace colorSpace)
{
    switch (colorSpace)
    {
        case QMPlay2ColorSpace::BT709:
            return {0.2126f, 0.7152f, 0.0722f};
        case QMPlay2ColorSpace::SMPTE170M:
            return {0.299f, 0.587f, 0.114f};
        case QMPlay2ColorSpace::SMPTE240M:
            return {0.212f, 0.701f, 0.087f};
        case QMPlay2ColorSpace::BT2020:
            return {0.2627f, 0.6780f, 0.0593f};
        default:
            break;
    }
    return {0.299f, 0.587f, 0.114f};
}

}
