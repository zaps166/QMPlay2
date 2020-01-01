/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2020  Błażej Szczygieł

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

#include <FFDecHWAccel.hpp>

#include <Frame.hpp>

extern "C"
{
    #include <libavformat/avformat.h>
}

FFDecHWAccel::FFDecHWAccel() :
    m_hwAccelWriter(nullptr),
    m_hasCriticalError(false)
{}
FFDecHWAccel::~FFDecHWAccel()
{}

VideoWriter *FFDecHWAccel::HWAccel() const
{
    return m_hwAccelWriter;
}

bool FFDecHWAccel::hasHWAccel(const char *hwaccelName) const
{
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(58, 9, 100)
    const AVHWDeviceType requestedType = av_hwdevice_find_type_by_name(hwaccelName);
    if (requestedType == AV_HWDEVICE_TYPE_NONE)
        return false;
    AVHWDeviceType hwType = AV_HWDEVICE_TYPE_NONE;
    for (;;)
    {
        hwType = av_hwdevice_iterate_types(hwType);
        if (hwType == AV_HWDEVICE_TYPE_NONE)
            break;
        if (hwType == requestedType)
            return true;
    }
    return false;
#else
    AVHWAccel *hwAccel = nullptr;
    while ((hwAccel = av_hwaccel_next(hwAccel)))
        if (hwAccel->id == codec_ctx->codec_id && strstr(hwAccel->name, hwaccelName))
            break;
    return hwAccel;
#endif
}

int FFDecHWAccel::decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurryUp)
{
    Q_UNUSED(newPixFmt)
    bool frameFinished = false;

    decodeFirstStep(encodedPacket, flush);

    if (hurryUp > 1)
        codec_ctx->skip_frame = AVDISCARD_NONREF;
    else if (hurryUp == 0)
        codec_ctx->skip_frame = AVDISCARD_DEFAULT;

    const int bytesConsumed = decodeStep(frameFinished);
    m_hasCriticalError = (bytesConsumed < 0);

    if (frameFinished && ~hurryUp)
    {
        if (m_hwAccelWriter)
        {
            decoded = Frame(frame);
        }
        else
        {
            downloadVideoFrame(decoded);
        }
    }

    decodeLastStep(encodedPacket, decoded, frameFinished);

    return m_hasCriticalError ? -1 : bytesConsumed;
}
void FFDecHWAccel::downloadVideoFrame(Frame &decoded)
{
    Q_UNUSED(decoded)
}

bool FFDecHWAccel::hasCriticalError() const
{
    return m_hasCriticalError;
}
