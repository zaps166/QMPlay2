/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <FFDec.hpp>
#include <FFCommon.hpp>

#include <StreamInfo.hpp>

#ifdef USE_VULKAN
#   include <vulkan/VulkanInstance.hpp>
#endif

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/pixdesc.h>
}

FFDec::FFDec() :
    codec_ctx(nullptr),
    packet(nullptr),
    frame(nullptr),
    codecIsOpen(false)
{}
FFDec::~FFDec()
{
    destroyDecoder();
}

int FFDec::pendingFrames() const
{
    return m_frames.count();
}

void FFDec::destroyDecoder()
{
    clearFrames();
    av_frame_free(&frame);
    av_packet_free(&packet);
    if (codecIsOpen)
    {
        avcodec_close(codec_ctx);
        codecIsOpen = false;
    }
    av_freep(&codec_ctx);
}

void FFDec::clearFrames()
{
    for (AVFrame *&frame : m_frames)
        av_frame_free(&frame);
    m_frames.clear();
}


AVCodec *FFDec::init(StreamInfo &streamInfo)
{
    AVCodec *codec = avcodec_find_decoder_by_name(streamInfo.codec_name);
    if (codec)
    {
        codec_ctx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codec_ctx, &streamInfo);
//        codec_ctx->debug_mv = FF_DEBUG_VIS_MV_P_FOR | FF_DEBUG_VIS_MV_B_FOR | FF_DEBUG_VIS_MV_B_BACK;
    }
    return codec;
}
bool FFDec::openCodec(AVCodec *codec)
{
    if (avcodec_open2(codec_ctx, codec, nullptr))
        return false;
    packet = av_packet_alloc();
    switch (codec_ctx->codec_type)
    {
        case AVMEDIA_TYPE_VIDEO:
#ifdef USE_VULKAN
            if (QMPlay2Core.isVulkanRenderer())
                m_vkImagePool = std::static_pointer_cast<QmVk::Instance>(QMPlay2Core.gpuInstance())->createImagePool();
#endif
            // fallthrough
        case AVMEDIA_TYPE_AUDIO:
            frame = av_frame_alloc();
            break;
        default:
            break;
    }
    return (codecIsOpen = true);
}

void FFDec::decodeFirstStep(const Packet &encodedPacket, bool flush)
{
    av_packet_copy_props(packet, encodedPacket);
    packet->data = (quint8 *)encodedPacket.data();
    packet->size = encodedPacket.size();
    if (flush)
    {
        avcodec_flush_buffers(codec_ctx);
        clearFrames();
    }
}
int FFDec::decodeStep(bool &frameFinished)
{
    int bytesConsumed = 0;
    bool sendOk = false;

    int sendErr = avcodec_send_packet(codec_ctx, packet);
    if (sendErr == 0 || sendErr == AVERROR(EAGAIN))
    {
        bytesConsumed = packet->size;
        sendOk = true;
    }

    for (;;)
    {
        int recvErr = avcodec_receive_frame(codec_ctx, frame);
        if (recvErr == 0)
        {
            m_frames.push_back(frame);
            frame = av_frame_alloc();
        }
        else
        {
            if ((recvErr != AVERROR_EOF && recvErr != AVERROR(EAGAIN)) || (!sendOk && sendErr != AVERROR_EOF))
            {
                bytesConsumed = -1;
                clearFrames();
            }
            break;
        }
    }

    frameFinished = maybeTakeFrame();

    return bytesConsumed;
}
void FFDec::decodeLastStep(const Packet &encodedPacket, Frame &decoded, bool frameFinished)
{
    decoded.setTimeBase(m_timeBase);
    if (frameFinished && !decoded.isTsValid())
    {
        if (frame->best_effort_timestamp != AV_NOPTS_VALUE)
            decoded.setTSInt(frame->best_effort_timestamp);
        else
            decoded.setTS(encodedPacket.ts());
    }
}

bool FFDec::maybeTakeFrame()
{
    if (!m_frames.isEmpty())
    {
        av_frame_free(&frame);
        frame = m_frames.takeFirst();
        return true;
    }
    return false;
}
