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

#include <VDPAUWriter.hpp>
#include <FFCommon.hpp>

#include <QMPlay2OSD.hpp>
#include <VideoFrame.hpp>
#include <Functions.hpp>
#include <ImgScaler.hpp>

#include <QDesktopWidget>
#include <QApplication>
#include <QDockWidget>
#include <QPainter>

#include <vdpau/vdpau_x11.h>

VDPAUWriter::VDPAUWriter(Module &module) :
    ok(false),
    mustRestartPlaying(false),
    canDrawOSD(false),
    queueTarget(0),
    presentationQueue(0),
    profile(-1),
    device(0),
    display(nullptr),
    vpd_decoder_render(nullptr),
    vdp_device_destroy(nullptr),
    sharpness_lvl(0.0f),
    lastWinId(0),
    aspect_ratio(0.0), zoom(0.0),
    flip(0), outW(0), outH(0), Hue(0), Saturation(0), Brightness(0), Contrast(0)
{
    setAttribute(Qt::WA_PaintOnScreen);
    grabGesture(Qt::PinchGesture);
    setMouseTracking(true);

    features[0] = VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL;
    features[1] = VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL;
    features[2] = VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION;
    features[3] = VDP_VIDEO_MIXER_FEATURE_SHARPNESS;
    for (int i = 0; i < scalingLevelsCount; ++i)
        features[i + scalingLevelsIdx] = VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1 + i;

    connect(&QMPlay2Core, SIGNAL(videoDockVisible(bool)), this, SLOT(videoVisible(bool)));
    connect(&visibleTim, SIGNAL(timeout()), this, SLOT(doVideoVisible()));
    connect(&drawTim, SIGNAL(timeout()), this, SLOT(draw()));
    visibleTim.setSingleShot(true);
    drawTim.setSingleShot(true);

    SetModule(module);
}
VDPAUWriter::~VDPAUWriter()
{
    clr();
    if (device)
    {
        if (presentationQueue)
            vdp_presentation_queue_destroy(presentationQueue);
        if (queueTarget)
            vdp_presentation_queue_target_destroy(queueTarget);
        if (vdp_device_destroy)
            vdp_device_destroy(device);
    }
    if (display)
        XCloseDisplay(display);
}

bool VDPAUWriter::set()
{
    switch (sets().getInt("VDPAUDeintMethod"))
    {
        case 0:
            featureEnables[0] = featureEnables[1] = false;
            break;
        case 2:
            featureEnables[0] = false;
            featureEnables[1] = true;
            break;
        default:
            featureEnables[0] = true;
            featureEnables[1] = false;
    }
    featureEnables[2] = sets().getBool("VDPAUNoiseReductionEnabled");
    noisereduction_lvl = sets().getDouble("VDPAUNoiseReductionLvl");
    if (noisereduction_lvl < 0.0f || noisereduction_lvl > 1.0f)
        noisereduction_lvl = 0.0f;
    quint32 scalingLvl = sets().getUInt("VDPAUHQScaling");
    if (scalingLvl > scalingLevelsCount)
        scalingLvl = 0;
    for (int i = 0; i < scalingLevelsCount; ++i)
        featureEnables[i + scalingLevelsIdx] = (int)scalingLvl > i;
    if (ok)
    {
        setFeatures();
        if (!paused)
        {
            if (!drawTim.isActive())
                drawTim.start(drawTimeout);
        }
        else //XXX: Is it really necessary to call 2 functions?
        {
            draw();
            vdpau_display();
            drawTim.stop();
        }
    }
    return true;
}

bool VDPAUWriter::readyWrite() const
{
    return ok;
}

bool VDPAUWriter::processParams(bool *)
{
    zoom = getParam("Zoom").toDouble();
    deinterlace = getParam("Deinterlace").toInt();
    aspect_ratio = getParam("AspectRatio").toDouble();
    flip = getParam("Flip").toInt();

    const int _Hue = getParam("Hue").toInt();
    const int _Saturation = getParam("Saturation").toInt();
    const int _Brightness = getParam("Brightness").toInt();
    const int _Contrast = getParam("Contrast").toInt();
    if (_Hue != Hue || _Saturation != Saturation || _Brightness != Brightness || _Contrast != Contrast)
    {
        Hue = _Hue;
        Saturation = _Saturation;
        Brightness = _Brightness;
        Contrast = _Contrast;

        VdpCSCMatrix matrix;
        VdpProcamp procamp = {VDP_PROCAMP_VERSION, Brightness / 100.0f, (Contrast / 100.0f) + 1.0f, (Saturation / 100.0f) + 1.0f, Hue / 31.830989f};
        if (vdp_generate_csc_matrix(&procamp, (outW >= 1280 || outH > 576) ? VDP_COLOR_STANDARD_ITUR_BT_709 : VDP_COLOR_STANDARD_ITUR_BT_601, &matrix) == VDP_STATUS_OK)
        {
            static constexpr VdpVideoMixerAttribute attributes[] = {VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX};
            const void *attributeValues[] = {&matrix};
            vdp_video_mixer_set_attribute_values(mixer, 1, attributes, attributeValues);
        }
    }

    const float _sharpness_lvl = getParam("Sharpness").toInt() / 100.0f;
    if (_sharpness_lvl != sharpness_lvl)
    {
        sharpness_lvl = _sharpness_lvl;
        featureEnables[3] = (sharpness_lvl != 0.0f);
        setFeatures();
    }

    if (!isVisible())
        emit QMPlay2Core.dockVideo(this);
    else
    {
        resizeEvent(nullptr);
        if (!paused)
        {
            if (!drawTim.isActive())
                drawTim.start(drawTimeout);
        }
        else //XXX: Is it really necessary to call 2 functions?
        {
            draw();
            vdpau_display();
            drawTim.stop();
        }
    }

    return readyWrite();
}
void VDPAUWriter::writeVideo(const VideoFrame &videoFrame)
{
    field = (VdpVideoMixerPictureStructure)Functions::getField(videoFrame, deinterlace, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD);
    draw(videoFrame.surfaceId);
    paused = false;
}
void VDPAUWriter::writeOSD(const QList<const QMPlay2OSD *> &osds)
{
    if (canDrawOSD)
    {
        osd_mutex.lock();
        osd_list = osds;
        osd_mutex.unlock();
    }
}
void VDPAUWriter::pause()
{
    paused = true;
    draw();
    vdpau_display();
}

bool VDPAUWriter::hwAccelGetImg(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) const
{
    if (dest)
    {
        const quint32 linesize[2] = {
            (quint32)outW,
            (quint32)outW
        };
        void *data[2] = {
            new quint8[linesize[0] * outH],
            new quint8[linesize[1] * ((outH + 1) >> 1)],
        };
        const bool ok = (vdp_surface_get_bits(videoFrame.surfaceId, VDP_YCBCR_FORMAT_NV12, data, linesize) == VDP_STATUS_OK);
        if (ok)
            nv12ToRGB32->scale((const void **)data, (const int *)linesize, dest);
        for (int p = 0; p < 2; ++p)
            delete[] (quint8 *)data[p];
        return ok;
    }
    return false;
}

QString VDPAUWriter::name() const
{
    return VDPAUWriterName;
}

bool VDPAUWriter::open()
{
    addParam("Zoom");
    addParam("AspectRatio");
    addParam("Deinterlace");
    addParam("PrepareForHWBobDeint", true);
    addParam("Flip");
    addParam("Hue");
    addParam("Saturation");
    addParam("Brightness");
    addParam("Contrast");
    addParam("Sharpness");

    clr();

    display = XOpenDisplay(nullptr);
    if (display && vdp_device_create_x11(display, 0, &device, &vdp_get_proc_address) == VDP_STATUS_OK)
    {
        bool getProcAddressOK = true;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_DECODER_CREATE, (void **)&vdp_decoder_create) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_DECODER_RENDER, (void **)&vpd_decoder_render) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_VIDEO_SURFACE_CREATE, (void **)&vdp_video_surface_create) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_VIDEO_SURFACE_DESTROY, (void **)&vdp_video_surface_destroy) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_OUTPUT_SURFACE_CREATE, (void **)&vdp_output_surface_create) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY, (void **)&vdp_output_surface_destroy) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES, (void **)&vdp_bitmap_surface_query_capabilities) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_BITMAP_SURFACE_CREATE, (void **)&vdp_bitmap_surface_create) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_BITMAP_SURFACE_DESTROY, (void **)&vdp_bitmap_surface_destroy) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE, (void **)&vdp_bitmap_surface_put_bits_native) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE, (void **)&vdp_output_surface_render_bitmap_surface) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE, (void **)&vdp_presentation_queue_create) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR, (void **)&vdp_presentation_queue_set_background_color) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY, (void **)&vdp_presentation_queue_destroy) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY, (void **)&vdp_presentation_queue_target_destroy) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_VIDEO_MIXER_CREATE, (void **)&vdp_video_mixer_create) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES, (void **)&vdp_video_mixer_set_feature_enables) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_VIDEO_MIXER_DESTROY, (void **)&vdp_video_mixer_destroy) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE, (void **)&vdp_presentation_queue_block_until_surface_idle) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_VIDEO_MIXER_RENDER, (void **)&vdp_video_mixer_render) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES, (void **)&vdp_video_mixer_set_attribute_values) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR, (void **)&vdp_surface_get_bits) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY, (void **)&vdp_presentation_queue_display) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_DEVICE_DESTROY, (void **)&vdp_device_destroy) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_DECODER_DESTROY, (void **)&vdp_decoder_destroy) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_GENERATE_CSC_MATRIX, (void **)&vdp_generate_csc_matrix) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES, (void **)&vdp_decoder_query_capabilities) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER, (void **)&vdp_preemption_callback_register) == VDP_STATUS_OK;
        getProcAddressOK &= vdp_get_proc_address(device, VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT, (void **)&vdp_video_mixer_query_feature_support) == VDP_STATUS_OK;
        if (getProcAddressOK)
        {
            VdpBool isSupported;
            quint32 out[4];
            profileList = QList<VdpDecoderProfile>()
                << VDP_DECODER_PROFILE_H264_HIGH << VDP_DECODER_PROFILE_H264_MAIN << VDP_DECODER_PROFILE_H264_BASELINE
#ifdef VDP_DECODER_PROFILE_HEVC_MAIN
                << VDP_DECODER_PROFILE_HEVC_MAIN
#endif
                << VDP_DECODER_PROFILE_MPEG2_MAIN << VDP_DECODER_PROFILE_MPEG2_SIMPLE
                << VDP_DECODER_PROFILE_MPEG4_PART2_ASP << VDP_DECODER_PROFILE_MPEG4_PART2_SP
                << VDP_DECODER_PROFILE_VC1_ADVANCED << VDP_DECODER_PROFILE_VC1_MAIN << VDP_DECODER_PROFILE_VC1_SIMPLE
                << VDP_DECODER_PROFILE_MPEG1
            ;
            for (int i = profileList.count() - 1; i >= 0; --i)
            {
                if (vdp_decoder_query_capabilities(device, profileList[i], &isSupported, out + 0, out + 1, out + 2, out + 3) != VDP_STATUS_OK || !isSupported)
                    profileList.removeAt(i);
            }
            if (!profileList.isEmpty())
            {
                vdp_preemption_callback_register(device, preemption_callback, this);
                if (vdp_bitmap_surface_query_capabilities(device, VDP_RGBA_FORMAT_B8G8R8A8, &isSupported, out + 0, out + 1) == VDP_STATUS_OK && isSupported)
                    canDrawOSD = true;
                return true;
            }
        }
    }

    return false;
}

SurfacesQueue VDPAUWriter::getSurfacesQueue() const
{
    SurfacesQueue surfacesQueue;
    for (int i = 0; i < surfacesCount; ++i)
        surfacesQueue.enqueue((QMPlay2SurfaceID)surfaces[i]);
    return surfacesQueue;
}

bool VDPAUWriter::hwAccelInit(int W, int H, const char *codec_name)
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

    VdpStatus status = VDP_STATUS_OK;

    if (!ok || profile != p || outW != W || outH != H)
    {
        clr();

        profile = p;
        outW = W;
        outH = H;

        status = vdp_decoder_create(device, profile, outW, outH, 16, &decoder);
        if (status == VDP_STATUS_OK)
        {
            bool err = false;
            for (int i = 0; i < surfacesCount; ++i)
            {
                status = vdp_video_surface_create(device, VDP_CHROMA_TYPE_420, outW, outH, &surfaces[i]);
                if (status != VDP_STATUS_OK)
                {
                    for (int j = 0; j < i; ++j)
                        vdp_video_surface_destroy(surfaces[j]);
                    err = true;
                    break;
                }
            }
            if (!err)
            {
                static constexpr int parametersCount = 3;
                static constexpr VdpVideoMixerParameter parameters[parametersCount] =
                {
                    VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH,
                    VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT,
                    VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE
                };
                static constexpr VdpChromaType vdp_chroma_type = VDP_CHROMA_TYPE_420;
                const void *const parameterValues[parametersCount] =
                {
                    &outW,
                    &outH,
                    &vdp_chroma_type
                };

                surfacesCreated = true;

                /* Bywa, że nie można stworzyć video_mixer jeżeli są załączone nieobsługiwane poziomy skalowania oraz nie da się zastosować filtrów */
                featuresCountCreated = scalingLevelsIdx;
                for (int i = 0; i < scalingLevelsCount; ++i)
                {
                    VdpBool fs = false;
                    vdp_video_mixer_query_feature_support(device, features[i + scalingLevelsIdx], &fs);
                    if (!fs)
                        break;
                    ++featuresCountCreated;
                }

                if (vdp_video_mixer_create(device, featuresCountCreated, features, parametersCount, parameters, parameterValues, &mixer) == VDP_STATUS_OK)
                {
                    setFeatures();
                    ok = true;
                }
            }
        }
    }
    else
    {
        vdp_decoder_destroy(decoder);
        status = vdp_decoder_create(device, profile, outW, outH, 16, &decoder);
        if (!(ok = (status == VDP_STATUS_OK)))
        {
            decoder = 0;
            ok = false;
        }
    }

    if (status == VDP_STATUS_RESOURCES)
        QMPlay2Core.logError("VDPAU :: " + tr("Not enough memory"));

    return ok;
}

void VDPAUWriter::preemption_callback(VdpDevice, void *context)
{
    ((VDPAUWriter *)context)->mustRestartPlaying = true;
}

void VDPAUWriter::setFeatures()
{
    VdpBool featuresSupport[featuresCount] = {false};
    for (int i = 0; i < featuresCount; ++i)
        vdp_video_mixer_query_feature_support(device, features[i], featuresSupport + i);
    if (!featuresSupport[1] && featureEnables[1])
    {
        QMPlay2Core.log(tr("Not supported deinterlacing algorithm") + " - Temporal-spatial", ErrorLog | LogOnce);
        featureEnables[1] = false;
        featureEnables[0] = true;
    }
    if (!featuresSupport[0] && featureEnables[0])
    {
        QMPlay2Core.log(tr("Not supported deinterlacing algorithm") + " - Temporal", ErrorLog | LogOnce);
        featureEnables[0] = false;
    }
    vdp_video_mixer_set_feature_enables(mixer, featuresCountCreated, features, featureEnables);
    if (!featuresSupport[2] && featureEnables[2])
        QMPlay2Core.log(tr("Unsupported noisy reduction filter"), ErrorLog | LogOnce);
    if (!featuresSupport[3] && featureEnables[3])
        QMPlay2Core.log(tr("Unsupported image sharpness filter"), ErrorLog | LogOnce);
    if (featuresSupport[2] || featuresSupport[3])
    {
        static constexpr VdpVideoMixerAttribute attributes[] = {
            VDP_VIDEO_MIXER_ATTRIBUTE_NOISE_REDUCTION_LEVEL,
            VDP_VIDEO_MIXER_ATTRIBUTE_SHARPNESS_LEVEL
        };
        const void *attributeValues[] = {
            &noisereduction_lvl,
            &sharpness_lvl
        };
        vdp_video_mixer_set_attribute_values(mixer, 2, attributes, attributeValues);
    }
    for (int i = scalingLevelsCount - 1; i >= 0; --i)
        if (featureEnables[i + scalingLevelsIdx])
        {
            if (!featuresSupport[scalingLevelsIdx + i])
                QMPlay2Core.log(tr("Unsupported image scaling level") + QString(" (L%1)").arg(i+1), ErrorLog | LogOnce);
            break;
        }
}

void VDPAUWriter::videoVisible(bool v)
{
    visibleTim.setProperty("videoVisible", v && (visibleRegion() != QRegion() || QMPlay2Core.getVideoDock()->visibleRegion() != QRegion()));
    visibleTim.start(1);
}
void VDPAUWriter::doVideoVisible()
{
    //This can hide overlay if avilable
    const bool visible = visibleTim.property("videoVisible").toBool();
    if (visible != !!presentationQueue) //Only when visibility changes
    {
        presentationQueueCreate(visible ? winId() : 0);
        if (!visible)
            drawTim.stop();
        else if (!drawTim.isActive())
            drawTim.start(paused ? 1 : drawTimeout);
    }
}

void VDPAUWriter::presentationQueueCreate(WId winId)
{
    if (presentationQueue)
    {
        vdp_presentation_queue_destroy(presentationQueue);
        presentationQueue = 0;
    }
    if (queueTarget)
    {
        vdp_presentation_queue_target_destroy(queueTarget);
        queueTarget = 0;
    }
    if (!winId)
        return; //Don't change "lastWinId" to 0 because of "draw()" method
    VdpPresentationQueueTargetCreateX11 *vdp_presentation_queue_target_create_x11;
    if
    (
        vdp_get_proc_address(device, VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_CREATE_X11, (void **)&vdp_presentation_queue_target_create_x11) == VDP_STATUS_OK &&
        vdp_presentation_queue_target_create_x11(device, winId, &queueTarget) == VDP_STATUS_OK &&
        vdp_presentation_queue_create(device, queueTarget, &presentationQueue) == VDP_STATUS_OK
    )
    {
        static constexpr VdpColor vdp_background_color = {0.000f, 0.000f, 0.004f, 0.000f};
        vdp_presentation_queue_set_background_color(presentationQueue, (VdpColor *)&vdp_background_color);
        lastWinId = winId;
    }
}

void VDPAUWriter::draw(VdpVideoSurface surface_id)
{
    if (surface_id != VDP_INVALID_HANDLE && renderSurfaces[0] != surface_id)
    {
        renderSurfaces[2] = renderSurfaces[1];
        renderSurfaces[1] = renderSurfaces[0];
        renderSurfaces[0] = surface_id;
    }
    if (renderSurfaces[0] != VDP_INVALID_HANDLE)
    {
        const WId currWinId = winId();
        if (currWinId != lastWinId)
            presentationQueueCreate(currWinId);
        if (presentationQueue && outputSurfacesCreated)
        {
            VdpTime time;

            if (surface_id != VDP_INVALID_HANDLE && hasImage)
            {
                vdpau_display();
                if (++outputSurfaceIdx >= outputSurfacesCount)
                    outputSurfaceIdx = 0;
            }

            if (vdp_presentation_queue_block_until_surface_idle(presentationQueue, outputSurfaces[outputSurfaceIdx], &time) != VDP_STATUS_INVALID_HANDLE)
            {
                vdp_video_mixer_render
                (
                    mixer,
                    VDP_INVALID_HANDLE, nullptr,
                    field,
                    2, renderSurfaces + 1,
                    renderSurfaces[0],
                    1, &renderSurfaces[0], //is it OK?
                    &srcRect,
                    outputSurfaces[outputSurfaceIdx], nullptr, &dstRect,
                    0, nullptr
                );

                osd_mutex.lock();
                if (!osd_list.isEmpty())
                {
                    static VdpOutputSurfaceRenderBlendState blend_state =
                    {
                        VDP_OUTPUT_SURFACE_RENDER_BLEND_STATE_VERSION,
                        VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_ALPHA,
                        VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                        VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_ALPHA,
                        VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                        VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD,
                        VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD,
                        (VdpColor){0.0f, 0.0f, 0.0f, 0.0f}
                    };
                    QRect bounds;
                    const qreal scaleW = (qreal)W / outW, scaleH = (qreal)H / outH;
                    bool mustRepaint = Functions::mustRepaintOSD(osd_list, osd_ids, &scaleW, &scaleH, &bounds);
                    if (!mustRepaint)
                        mustRepaint = bitmapSurfaceSize != bounds.size();
                    if (mustRepaint)
                    {
                        if (osdImg.size() != bounds.size())
                            osdImg = QImage(bounds.size(), QImage::Format_ARGB32);
                        osdImg.fill(0);
                        QPainter p(&osdImg);
                        p.translate(-bounds.topLeft());
                        Functions::paintOSD(false, osd_list, scaleW, scaleH, p, &osd_ids);
                        if (bitmapSurfaceSize != bounds.size())
                        {
                            if (bitmapSurface != VDP_INVALID_HANDLE)
                                vdp_bitmap_surface_destroy(bitmapSurface);
                            if (vdp_bitmap_surface_create(device, VDP_RGBA_FORMAT_R8G8B8A8, bounds.width(), bounds.height(), VDP_TRUE, &bitmapSurface) == VDP_STATUS_OK)
                                bitmapSurfaceSize = bounds.size();
                            else
                            {
                                bitmapSurface = VDP_INVALID_HANDLE;
                                bitmapSurfaceSize = QSize();
                            }
                        }
                    }
                    if (bitmapSurface != VDP_INVALID_HANDLE)
                    {
                        if (mustRepaint)
                        {
                            const void *data = osdImg.constBits();
                            const quint32 linesize = bounds.width() << 2;
                            vdp_bitmap_surface_put_bits_native(bitmapSurface, &data, &linesize, nullptr);
                        }
                        const VdpRect bitmapDstRect = {(quint32)bounds.left() + X, (quint32)bounds.top() + Y, (quint32)bounds.right() + X + 1, (quint32)bounds.bottom() + Y + 1};
                        vdp_output_surface_render_bitmap_surface(outputSurfaces[outputSurfaceIdx], &bitmapDstRect, bitmapSurface, nullptr, nullptr, &blend_state, 0);
                    }
                }
                osd_mutex.unlock();

                if (surface_id == VDP_INVALID_HANDLE || !hasImage)
                    vdpau_display();

                hasImage = true;
            }
            else if (mustRestartPlaying)
            {
                emit QMPlay2Core.processParam("RestartPlaying");
                mustRestartPlaying = false;
            }
        }
    }
}
void VDPAUWriter::vdpau_display()
{
    vdp_presentation_queue_display(presentationQueue, outputSurfaces[outputSurfaceIdx], 0, 0, 0);
    if (drawTim.isActive())
        drawTim.stop();
}

void VDPAUWriter::resizeEvent(QResizeEvent *)
{
    const qreal dpr = devicePixelRatioF();

    const int winW = width()  * dpr;
    const int winH = height() * dpr;

    QRect dstQRect, srcQRect;
    Functions::getImageSize(aspect_ratio, zoom, winW, winH, W, H, &X, &Y, &dstQRect, &outW, &outH, &srcQRect);

    srcRect.x0 = srcQRect.left();
    srcRect.y0 = srcQRect.top();
    srcRect.x1 = srcQRect.right() + 1;
    srcRect.y1 = srcQRect.bottom() + 1;

    dstRect.x0 = dstQRect.left();
    dstRect.y0 = dstQRect.top();
    dstRect.x1 = dstQRect.right() + 1;
    dstRect.y1 = dstQRect.bottom() + 1;

    if (flip & Qt::Horizontal)
        qSwap(srcRect.x0, srcRect.x1);
    if (flip & Qt::Vertical)
        qSwap(srcRect.y0, srcRect.y1);

    const int desktopW = QApplication::desktop()->width() * dpr, desktopH = QApplication::desktop()->height() * dpr;
    QSize newOutputSurfacesSize(desktopW, desktopH);
    if (desktopW > 0 && desktopH > 0)
    {
        while (newOutputSurfacesSize.width() < winW)
            newOutputSurfacesSize.rwidth() += desktopW >> 1;
        while (newOutputSurfacesSize.height() < winH)
            newOutputSurfacesSize.rheight() += desktopH >> 1;
    }
    if (outputSurfacesSize != newOutputSurfacesSize)
    {
        if (outputSurfacesCreated)
        {
            destroyOutputSurfaces();
            outputSurfacesCreated = false;
            outputSurfacesSize = QSize();
        }
        for (int i = 0; i < outputSurfacesCount; ++i)
            if (vdp_output_surface_create(device, VDP_RGBA_FORMAT_B8G8R8A8, newOutputSurfacesSize.width(), newOutputSurfacesSize.height(), &outputSurfaces[i]) != VDP_STATUS_OK)
            {
                for (int j = 0; j < i; ++j)
                    vdp_output_surface_destroy(outputSurfaces[j]);
                return;
            }
        outputSurfacesSize = newOutputSurfacesSize;
        outputSurfacesCreated = true;
    }
}
void VDPAUWriter::paintEvent(QPaintEvent *)
{
    if (!drawTim.isActive())
        drawTim.start(paused ? 1 : drawTimeout);
}
bool VDPAUWriter::event(QEvent *e)
{
    /* Pass gesture and touch event to the parent */
    switch (e->type())
    {
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        case QEvent::Gesture:
            return QCoreApplication::sendEvent(parent(), e);
        default:
            return QWidget::event(e);
    }
}

QPaintEngine *VDPAUWriter::paintEngine() const
{
    return nullptr;
}

void VDPAUWriter::destroyOutputSurfaces()
{
    for (int i = 0; i < outputSurfacesCount; ++i)
        vdp_output_surface_destroy(outputSurfaces[i]);
}
void VDPAUWriter::clr()
{
    if (device)
    {
        if (bitmapSurface != VDP_INVALID_HANDLE)
            vdp_bitmap_surface_destroy(bitmapSurface);
        if (surfacesCreated)
            for (int i = 0; i < surfacesCount; ++i)
                vdp_video_surface_destroy(surfaces[i]);
        if (outputSurfacesCreated)
            destroyOutputSurfaces();
        if (mixer)
            vdp_video_mixer_destroy(mixer);
        if (decoder)
            vdp_decoder_destroy(decoder);
    }
    for (int i = 0; i < 3; ++i)
        renderSurfaces[i] = VDP_INVALID_HANDLE;
    bitmapSurface = VDP_INVALID_HANDLE;
    bitmapSurfaceSize = QSize();
    outputSurfacesSize = QSize();
    surfacesCreated = outputSurfacesCreated = ok = paused = hasImage = false;
    osd_ids.clear();
    outputSurfaceIdx = 0;
    osdImg = QImage();
    decoder = 0;
    mixer = 0;
}
