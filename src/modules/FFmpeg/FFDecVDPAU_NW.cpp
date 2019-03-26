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

#include <FFDecVDPAU_NW.hpp>
#include <FFCommon.hpp>

#include <HWAccelHelper.hpp>
#include <StreamInfo.hpp>
#include <VideoFrame.hpp>
#include <Functions.hpp>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/pixdesc.h>
    #include <libavcodec/vdpau.h>
}

#include <vdpau/vdpau_x11.h>

class VDPAU
{
public:
    VDPAU(int w, int h, const char *codec_name) :
        ok(false),
        display(nullptr),
        device(0),
        decoder(0),
        vpd_decoder_render(nullptr),
        vdp_device_destroy(nullptr)
    {
        memset(surfaces, 0, sizeof surfaces);
        display = XOpenDisplay(nullptr);
        if (display && vdp_device_create_x11(display, 0, &device, &vdp_get_proc_address) == VDP_STATUS_OK)
        {
            if
            (
                vdp_get_proc_address(device, VDP_FUNC_ID_DECODER_CREATE, (void **)&vdp_decoder_create) == VDP_STATUS_OK &&
                vdp_get_proc_address(device, VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR, (void **)&vdp_surface_get_bits) == VDP_STATUS_OK &&
                vdp_get_proc_address(device, VDP_FUNC_ID_VIDEO_SURFACE_CREATE, (void **)&vdp_video_surface_create) == VDP_STATUS_OK &&
                vdp_get_proc_address(device, VDP_FUNC_ID_VIDEO_SURFACE_DESTROY, (void **)&vdp_video_surface_destroy) == VDP_STATUS_OK &&
                vdp_get_proc_address(device, VDP_FUNC_ID_DEVICE_DESTROY, (void **)&vdp_device_destroy) == VDP_STATUS_OK &&
                vdp_get_proc_address(device, VDP_FUNC_ID_DECODER_DESTROY, (void **)&vdp_decoder_destroy) == VDP_STATUS_OK &&
                vdp_get_proc_address(device, VDP_FUNC_ID_DECODER_RENDER, (void **)&vpd_decoder_render) == VDP_STATUS_OK &&
                vdp_get_proc_address(device, VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES, (void **)&vdp_decoder_query_capabilities) == VDP_STATUS_OK
            )
            {
                quint32 out[4];
                VdpBool isSupported;
                QList<VdpDecoderProfile> profileList = QList<VdpDecoderProfile>()
                    << VDP_DECODER_PROFILE_H264_HIGH << VDP_DECODER_PROFILE_H264_MAIN << VDP_DECODER_PROFILE_H264_BASELINE
#ifdef VDP_DECODER_PROFILE_HEVC_MAIN
                    << VDP_DECODER_PROFILE_HEVC_MAIN
#endif
                    << VDP_DECODER_PROFILE_MPEG2_MAIN << VDP_DECODER_PROFILE_MPEG2_SIMPLE
                    << VDP_DECODER_PROFILE_MPEG4_PART2_ASP << VDP_DECODER_PROFILE_MPEG4_PART2_SP
                    << VDP_DECODER_PROFILE_VC1_ADVANCED << VDP_DECODER_PROFILE_VC1_MAIN << VDP_DECODER_PROFILE_VC1_SIMPLE
                    << VDP_DECODER_PROFILE_MPEG1
                ;
                for (int i = profileList.count() - 1 ; i >= 0 ; --i)
                {
                    if (vdp_decoder_query_capabilities(device, profileList[i], &isSupported, out + 0, out + 1, out + 2, out + 3) != VDP_STATUS_OK || !isSupported)
                        profileList.removeAt(i);
                }
                if (!profileList.isEmpty())
                {
                    VdpDecoderProfile p = -1;
                    if (!qstrcmp(codec_name, "h264"))
                    {
                        if (profileList.contains(VDP_DECODER_PROFILE_H264_HIGH))
                            p = VDP_DECODER_PROFILE_H264_HIGH;
                        else if (profileList.contains(VDP_DECODER_PROFILE_H264_MAIN))
                            p = VDP_DECODER_PROFILE_H264_MAIN;
                        else if (profileList.contains(VDP_DECODER_PROFILE_H264_BASELINE))
                            p = VDP_DECODER_PROFILE_H264_BASELINE;
                    }
#ifdef VDP_DECODER_PROFILE_HEVC_MAIN
                    else if (!qstrcmp(codec_name, "hevc"))
                    {
                        if (profileList.contains(VDP_DECODER_PROFILE_HEVC_MAIN))
                            p = VDP_DECODER_PROFILE_HEVC_MAIN;
                    }
#endif
                    else if (!qstrcmp(codec_name, "mpeg2video"))
                    {
                        if (profileList.contains(VDP_DECODER_PROFILE_MPEG2_MAIN))
                            p = VDP_DECODER_PROFILE_MPEG2_MAIN;
                        else if (profileList.contains(VDP_DECODER_PROFILE_MPEG2_SIMPLE))
                            p = VDP_DECODER_PROFILE_MPEG2_SIMPLE;
                    }
                    else if (!qstrcmp(codec_name, "mpeg4"))
                    {
                        if (profileList.contains(VDP_DECODER_PROFILE_MPEG4_PART2_ASP))
                            p = VDP_DECODER_PROFILE_MPEG4_PART2_ASP;
                        else if (profileList.contains(VDP_DECODER_PROFILE_MPEG4_PART2_SP))
                            p = VDP_DECODER_PROFILE_MPEG4_PART2_SP;
                    }
                    else if (!qstrcmp(codec_name, "vc1"))
                    {
                        if (profileList.contains(VDP_DECODER_PROFILE_VC1_ADVANCED))
                            p = VDP_DECODER_PROFILE_VC1_ADVANCED;
                        else if (profileList.contains(VDP_DECODER_PROFILE_VC1_MAIN))
                            p = VDP_DECODER_PROFILE_VC1_MAIN;
                        else if (profileList.contains(VDP_DECODER_PROFILE_VC1_SIMPLE))
                            p = VDP_DECODER_PROFILE_VC1_SIMPLE;
                    }
                    else if (!qstrcmp(codec_name, "mpeg1video"))
                    {
                        if (profileList.contains(VDP_DECODER_PROFILE_MPEG1))
                            p = VDP_DECODER_PROFILE_MPEG1;
                    }

                    if (vdp_decoder_create(device, p, w, h, 16, &decoder) == VDP_STATUS_OK)
                    {
                        ok = true;
                        for (int i = 0 ; i < surfacesCount ; ++i)
                            if (vdp_video_surface_create(device, VDP_CHROMA_TYPE_420, w, h, &surfaces[i]) != VDP_STATUS_OK)
                            {
                                ok = false;
                                break;
                            }
                    }

                }
            }
        }
    }
    ~VDPAU()
    {
        if (device)
        {
            if (decoder)
            {
                for (int i = 0 ; i < surfacesCount ; ++i)
                    vdp_video_surface_destroy(surfaces[i]);
                vdp_decoder_destroy(decoder);
            }
            if (vdp_device_destroy)
                vdp_device_destroy(device);
        }
        if (display)
            XCloseDisplay(display);
    }

    inline SurfacesQueue getSurfacesQueue() const
    {
        SurfacesQueue surfacesQueue;
        for (int i = 0; i < surfacesCount; ++i)
            surfacesQueue.enqueue((QMPlay2SurfaceID)surfaces[i]);
        return surfacesQueue;
    }

    static constexpr int surfacesCount = 20;
    VdpVideoSurface surfaces[surfacesCount];

    bool ok;

    Display *display;
    VdpDevice device;
    VdpDecoder decoder;
    VdpDecoderRender *vpd_decoder_render;
    VdpDecoderCreate *vdp_decoder_create;
    VdpDeviceDestroy *vdp_device_destroy;
    VdpDecoderDestroy *vdp_decoder_destroy;
    VdpGetProcAddress *vdp_get_proc_address;
    VdpVideoSurfaceCreate *vdp_video_surface_create;
    VdpVideoSurfaceGetBitsYCbCr *vdp_surface_get_bits;
    VdpVideoSurfaceDestroy *vdp_video_surface_destroy;
    VdpDecoderQueryCapabilities *vdp_decoder_query_capabilities;
};

/**/

FFDecVDPAU_NW::FFDecVDPAU_NW(Module &module) :
    vdpau(nullptr)
{
    SetModule(module);
}
FFDecVDPAU_NW::~FFDecVDPAU_NW()
{
    if (codecIsOpen)
        avcodec_flush_buffers(codec_ctx);
    delete vdpau;
}

bool FFDecVDPAU_NW::set()
{
    return sets().getBool("DecoderVDPAU_NWEnabled");
}

QString FFDecVDPAU_NW::name() const
{
    return "FFmpeg/" VDPAUWriterName;
}

void FFDecVDPAU_NW::downloadVideoFrame(VideoFrame &decoded)
{
    const int aligned8W = Functions::aligned(frame->width, 8);
    const int linesize[] = {
        aligned8W,
        aligned8W >> 1,
        aligned8W >> 1
    };
    const VideoFrameSize aligned4HFrameSize(frame->width, Functions::aligned(frame->height, 4));
    const VideoFrameSize realFrameSize(frame->width, frame->height);
    decoded = VideoFrame(aligned4HFrameSize, linesize, frame->interlaced_frame, frame->top_field_first);
    decoded.size = realFrameSize;
    void *data[] = {decoded.buffer[0].data(), decoded.buffer[2].data(), decoded.buffer[1].data()};
    if (vdpau->vdp_surface_get_bits((quintptr)frame->data[3], VDP_YCBCR_FORMAT_YV12, data, (quint32 *)decoded.linesize) != VDP_STATUS_OK)
        decoded.clear();
}

bool FFDecVDPAU_NW::open(StreamInfo &streamInfo, VideoWriter *)
{
    if (av_get_pix_fmt(streamInfo.format) == AV_PIX_FMT_YUV420P) //Read comment in FFDecVDPAU::open()
    {
        AVCodec *codec = init(streamInfo);
        if (codec && hasHWAccel("vdpau"))
        {
            vdpau = new VDPAU(codec_ctx->width, codec_ctx->height, avcodec_get_name(codec_ctx->codec_id));
            if (vdpau->ok)
            {
                AVVDPAUContext *vdpauCtx = FFCommon::allocAVVDPAUContext(codec_ctx);
                if (!vdpauCtx)
                    return false;
                vdpauCtx->decoder = vdpau->decoder;
                vdpauCtx->render  = vdpau->vpd_decoder_render;

                new HWAccelHelper(codec_ctx, AV_PIX_FMT_VDPAU, vdpauCtx, vdpau->getSurfacesQueue());

                if (openCodec(codec))
                {
                    time_base = streamInfo.getTimeBase();
                    return true;
                }
            }
        }
    }
    return false;
}
