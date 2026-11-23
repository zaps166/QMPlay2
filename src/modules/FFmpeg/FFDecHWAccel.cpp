// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <FFDecHWAccel.hpp>

#include <StreamInfo.hpp>
#include <Frame.hpp>

FFDecHWAccel::FFDecHWAccel()
{}
FFDecHWAccel::~FFDecHWAccel()
{
    sws_freeContext(m_swsCtx);
}

bool FFDecHWAccel::hasHWAccel(const char *hwaccelName)
{
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
}

AVCodec *FFDecHWAccel::init(StreamInfo &streamInfo)
{
    const QByteArray codecName = avcodec_get_name(streamInfo.params->codec_id);
    if (streamInfo.codec_name != codecName)
    {
        streamInfo.codec_name_backup = streamInfo.codec_name;
        streamInfo.codec_name = codecName;
    }
    return FFDec::init(streamInfo);
}

bool FFDecHWAccel::hasHWDecContext() const
{
    return m_hasHWDecContext;
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
        decoded = Frame(frame, Frame::convert3PlaneTo2Plane(codec_ctx->sw_pix_fmt));
        if (!m_hasHWDecContext)
            decoded = decoded.downloadHwData(&m_swsCtx, m_supportedPixelFormats);
    }

    decodeLastStep(encodedPacket, decoded, frameFinished);

    return m_hasCriticalError ? -1 : bytesConsumed;
}

bool FFDecHWAccel::hasCriticalError() const
{
    return m_hasCriticalError;
}
