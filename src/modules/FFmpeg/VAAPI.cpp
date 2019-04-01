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

#include <VAAPI.hpp>
#include <Functions.hpp>
#include <ImgScaler.hpp>
#include <VideoFrame.hpp>
#include <QMPlay2Core.hpp>

#include <QX11Info>

#include <va/va_x11.h>
#include <va/va_glx.h>

VAAPI::VAAPI()
{}
VAAPI::~VAAPI()
{
    clearVPP();
    if (VADisp)
        vaTerminate(VADisp);
}

bool VAAPI::open(const char *codecName, bool &openGL)
{
    clearVPP();

    if (!QX11Info::isPlatformX11())
        return false;

    Display *display = QX11Info::display();
    if (!display)
        return false;

    VADisp = openGL ? vaGetDisplayGLX(display) : vaGetDisplay(display);
    if (!VADisp)
    {
        if (openGL)
        {
            VADisp = vaGetDisplay(display);
            openGL = false;
        }
        if (!VADisp)
            return false;
    }

    int major = 0, minor = 0;
    if (vaInitialize(VADisp, &major, &minor) != VA_STATUS_SUCCESS)
        return false;

    version = (major << 8) | minor;

    const QString vendor = vaQueryVendorString(VADisp);
    if (vendor.contains("VDPAU")) // Not supported in FFmpeg due to "vaQuerySurfaceAttributes()"
        return false;

    if (!hasProfile(codecName))
        return false;

    int fmtCount = vaMaxNumImageFormats(VADisp);
    VAImageFormat imgFmt[fmtCount];
    if (vaQueryImageFormats(VADisp, imgFmt, &fmtCount) == VA_STATUS_SUCCESS)
    {
        for (int i = 0; i < fmtCount; ++i)
        {
            if (imgFmt[i].fourcc == VA_FOURCC_NV12)
            {
                nv12ImageFmt = imgFmt[i];
                break;
            }
        }
    }

    return true;
}

void VAAPI::init(int width, int height,  bool allowFilters)
{
    if (!ok || outW != width || outH != height)
    {
        clearVPP();

        outW = width;
        outH = height;

        m_allowFilters = allowFilters;

        ok = true;
    }
    else
    {
        forward_reference = VA_INVALID_SURFACE;
        vpp_second = false;
    }
}

bool VAAPI::vaapiCreateSurface(VASurfaceID &surface, int w, int h)
{
    VASurfaceAttrib attrib;
    attrib.type = VASurfaceAttribPixelFormat;
    attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
    attrib.value.type = VAGenericValueTypeInteger;
    attrib.value.value.i = VA_FOURCC_NV12;
    return vaCreateSurfaces(VADisp, VA_RT_FORMAT_YUV420, w, h, &surface, 1, &attrib, 1) == VA_STATUS_SUCCESS;
}

void VAAPI::maybeInitVPP(int surfaceW, int surfaceH)
{
    if (!m_allowFilters || use_vpp)
        return;

    use_vpp = true;

    if
    (
        vaCreateConfig(VADisp, VAProfileNone, VAEntrypointVideoProc, nullptr, 0, &config_vpp) == VA_STATUS_SUCCESS &&
        vaCreateContext(VADisp, config_vpp, 0, 0, 0, nullptr, 0, &context_vpp) == VA_STATUS_SUCCESS &&
        vaapiCreateSurface(id_vpp, surfaceW, surfaceH)
    )
    {
        unsigned num_filters = VAProcFilterCount;
        VAProcFilterType filters[VAProcFilterCount];
        if (vaQueryVideoProcFilters(VADisp, context_vpp, filters, &num_filters) != VA_STATUS_SUCCESS)
            num_filters = 0;
        if (num_filters > 0)
        {
            /* Creating dummy filter (some drivers/api versions (1.6.x and Ivy Bridge) crashes without any filter) - this is unreachable now */
            VAProcFilterParameterBufferBase none_params = {VAProcFilterNone};
            if (vaCreateBuffer(VADisp, context_vpp, VAProcFilterParameterBufferType, sizeof none_params, 1, &none_params, &vpp_buffers[VAProcFilterNone]) != VA_STATUS_SUCCESS)
                vpp_buffers[VAProcFilterNone] = VA_INVALID_ID;
            /* Searching deinterlacing filter */
            if (vpp_deint_type != VAProcDeinterlacingNone)
            {
                for (unsigned i = 0; i < num_filters; ++i)
                {
                    if (filters[i] == VAProcFilterDeinterlacing)
                    {
                        VAProcFilterCapDeinterlacing deinterlacing_caps[VAProcDeinterlacingCount];
                        unsigned num_deinterlacing_caps = VAProcDeinterlacingCount;
                        if (vaQueryVideoProcFilterCaps(VADisp, context_vpp, VAProcFilterDeinterlacing, &deinterlacing_caps, &num_deinterlacing_caps) != VA_STATUS_SUCCESS)
                            num_deinterlacing_caps = 0;
                        bool vpp_deint_types[2] = {false};
                        for (unsigned j = 0; j < num_deinterlacing_caps; ++j)
                        {
                            switch (deinterlacing_caps[j].type)
                            {
                                case VAProcDeinterlacingMotionAdaptive:
                                    vpp_deint_types[0] = true;
                                    break;
                                case VAProcDeinterlacingMotionCompensated:
                                    vpp_deint_types[1] = true;
                                    break;
                                default:
                                    break;
                            }
                        }
                        if (vpp_deint_type == VAProcDeinterlacingMotionCompensated && !vpp_deint_types[1])
                        {
                            QMPlay2Core.log("VA-API :: " + tr("Not supported deinterlacing algorithm") + " - Motion compensated", ErrorLog | LogOnce);
                            vpp_deint_type = VAProcDeinterlacingMotionAdaptive;
                        }
                        if (vpp_deint_type == VAProcDeinterlacingMotionAdaptive && !vpp_deint_types[0])
                        {
                            QMPlay2Core.log("VA-API :: " + tr("Not supported deinterlacing algorithm") + " - Motion adaptive", ErrorLog | LogOnce);
                            vpp_deint_type = VAProcDeinterlacingBob;
                        }
                        if (vpp_deint_type != VAProcDeinterlacingNone)
                        {
                            VAProcFilterParameterBufferDeinterlacing deint_params = {
                                VAProcFilterDeinterlacing,
                                vpp_deint_type,
                                0,
                                {}
                            };
                            if (vaCreateBuffer(VADisp, context_vpp, VAProcFilterParameterBufferType, sizeof deint_params, 1, &deint_params, &vpp_buffers[VAProcFilterDeinterlacing]) != VA_STATUS_SUCCESS)
                                vpp_buffers[VAProcFilterDeinterlacing] = VA_INVALID_ID;
                        }
                        break;
                    }
                }
            }
            return;
        }
    }

    if (vpp_deint_type != VAProcDeinterlacingNone) //Show error only when filter is required
        QMPlay2Core.log("VA-API :: " + tr("Cannot open video filters"), ErrorLog | LogOnce);

    clearVPP();
}
void VAAPI::clearVPP(bool resetAllowFilters)
{
    if (use_vpp)
    {
        for (int i = 0; i < VAProcFilterCount; ++i)
            if (vpp_buffers[i] != VA_INVALID_ID)
                vaDestroyBuffer(VADisp, vpp_buffers[i]);
        if (id_vpp != VA_INVALID_SURFACE)
            vaDestroySurfaces(VADisp, &id_vpp, 1);
        if (context_vpp)
            vaDestroyContext(VADisp, context_vpp);
        if (config_vpp)
            vaDestroyConfig(VADisp, config_vpp);
        use_vpp = false;
    }
    id_vpp = forward_reference = VA_INVALID_SURFACE;
    for (int i = 0; i < VAProcFilterCount; ++i)
        vpp_buffers[i] = VA_INVALID_ID;
    vpp_second = false;
    context_vpp = 0;
    config_vpp = 0;
    if (resetAllowFilters)
        m_allowFilters = false;
}

void VAAPI::applyVideoAdjustment(int brightness, int contrast, int saturation, int hue)
{
    int num_attribs = vaMaxNumDisplayAttributes(VADisp);
    VADisplayAttribute attribs[num_attribs];
    if (!vaQueryDisplayAttributes(VADisp, attribs, &num_attribs))
    {
        for (int i = 0; i < num_attribs; ++i)
        {
            switch (attribs[i].type)
            {
                case VADisplayAttribHue:
                    attribs[i].value = Functions::scaleEQValue(hue, attribs[i].min_value, attribs[i].max_value);
                    break;
                case VADisplayAttribSaturation:
                    attribs[i].value = Functions::scaleEQValue(saturation, attribs[i].min_value, attribs[i].max_value);
                    break;
                case VADisplayAttribBrightness:
                    attribs[i].value = Functions::scaleEQValue(brightness, attribs[i].min_value, attribs[i].max_value);
                    break;
                case VADisplayAttribContrast:
                    attribs[i].value = Functions::scaleEQValue(contrast, attribs[i].min_value, attribs[i].max_value);
                    break;
                default:
                    break;
            }
        }
        vaSetDisplayAttributes(VADisp, attribs, num_attribs);
    }
}

bool VAAPI::filterVideo(const VideoFrame &videoFrame, VASurfaceID &id, int &field)
{
    const VASurfaceID curr_id = videoFrame.surfaceId;
    if (!checkSurface(curr_id))
        return false;
    const bool do_vpp_deint = (field != 0) && vpp_buffers[VAProcFilterDeinterlacing] != VA_INVALID_ID;
    if (use_vpp && !do_vpp_deint)
    {
        forward_reference = VA_INVALID_SURFACE;
        vpp_second = false;
    }
    if (use_vpp && (do_vpp_deint || version <= 0x0023))
    {
        bool vpp_ok = false;

        if (do_vpp_deint && forward_reference == VA_INVALID_SURFACE)
            forward_reference = curr_id;
        if (!vpp_second && forward_reference == curr_id)
            return false;

        if (do_vpp_deint)
        {
            VAProcFilterParameterBufferDeinterlacing *deint_params = nullptr;
            if (vaMapBuffer(VADisp, vpp_buffers[VAProcFilterDeinterlacing], (void **)&deint_params) == VA_STATUS_SUCCESS)
            {
                if (version > 0x0025 || !vpp_second)
                    deint_params->flags = (field == VA_TOP_FIELD) ? 0 : VA_DEINTERLACING_BOTTOM_FIELD;
                vaUnmapBuffer(VADisp, vpp_buffers[VAProcFilterDeinterlacing]);
            }
        }

        VABufferID pipeline_buf;
        if (vaCreateBuffer(VADisp, context_vpp, VAProcPipelineParameterBufferType, sizeof(VAProcPipelineParameterBuffer), 1, nullptr, &pipeline_buf) == VA_STATUS_SUCCESS)
        {
            VAProcPipelineParameterBuffer *pipeline_param = nullptr;
            if (vaMapBuffer(VADisp, pipeline_buf, (void **)&pipeline_param) == VA_STATUS_SUCCESS)
            {
                memset(pipeline_param, 0, sizeof *pipeline_param);
                pipeline_param->surface = curr_id;
                pipeline_param->output_background_color = 0xFF000000;

                pipeline_param->num_filters = 1;
                if (!do_vpp_deint)
                    pipeline_param->filters = &vpp_buffers[VAProcFilterNone]; //Sometimes it can prevent crash, but sometimes it can produce green image, so it is disabled for newer VA-API versions which don't crash without VPP
                else
                {
                    pipeline_param->filters = &vpp_buffers[VAProcFilterDeinterlacing];
                    pipeline_param->num_forward_references = 1;
                    pipeline_param->forward_references = &forward_reference;
                }

                vaUnmapBuffer(VADisp, pipeline_buf);

                if (vaBeginPicture(VADisp, context_vpp, id_vpp) == VA_STATUS_SUCCESS)
                {
                    vpp_ok = vaRenderPicture(VADisp, context_vpp, &pipeline_buf, 1) == VA_STATUS_SUCCESS;
                    vaEndPicture(VADisp, context_vpp);
                }
            }
            if (!vpp_ok)
                vaDestroyBuffer(VADisp, pipeline_buf);
        }

        if (vpp_second)
            forward_reference = curr_id;
        if (do_vpp_deint)
            vpp_second = !vpp_second;

        if ((ok = vpp_ok))
        {
            id = id_vpp;
            if (do_vpp_deint)
                field = 0;
            return true;
        }

        return false;
    }
    else
    {
        id = curr_id;
    }
    return true;
}

quint8 *VAAPI::getNV12Image(VAImage &image, VASurfaceID surfaceID) const
{
    if (nv12ImageFmt.fourcc == VA_FOURCC_NV12)
    {
        VAImageFormat imgFmt = nv12ImageFmt;
        if (vaCreateImage(VADisp, &imgFmt, outW, outH, &image) == VA_STATUS_SUCCESS)
        {
            quint8 *data;
            if
            (
                vaSyncSurface(VADisp, surfaceID) == VA_STATUS_SUCCESS &&
                vaGetImage(VADisp, surfaceID, 0, 0, outW, outH, image.image_id) == VA_STATUS_SUCCESS &&
                vaMapBuffer(VADisp, image.buf, (void **)&data) == VA_STATUS_SUCCESS
            ) return data;
            vaDestroyImage(VADisp, image.image_id);
        }
    }
    return nullptr;
}
bool VAAPI::getImage(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) const
{
    VAImage image;
    quint8 *vaData = getNV12Image(image, videoFrame.surfaceId);
    if (vaData)
    {
        const void *data[2] = {
            vaData + image.offsets[0],
            vaData + image.offsets[1]
        };
        nv12ToRGB32->scale(data, (const int *)image.pitches, dest);
        vaUnmapBuffer(VADisp, image.buf);
        vaDestroyImage(VADisp, image.image_id);
        return true;
    }
    return false;
}

void VAAPI::clearSurfaces()
{
    QMutexLocker locker(&m_surfacesMutex);
    m_surfaces.clear();
}
void VAAPI::insertSurface(quintptr id)
{
    QMutexLocker locker(&m_surfacesMutex);
    m_surfaces.insert(id);
}
bool VAAPI::checkSurface(quintptr id)
{
    QMutexLocker locker(&m_surfacesMutex);
    return m_surfaces.contains(id);
}

bool VAAPI::hasProfile(const char *codecName) const
{
    // Optional check - FFmpeg opens VA-API when first frame is given to the codec,
    // so check here if codec is supported to not open VideoWriter unnecessarily.

    int numProfiles = vaMaxNumProfiles(VADisp);
    QVector<VAProfile> vaProfiles(numProfiles);
    if (vaQueryConfigProfiles(VADisp, vaProfiles.data(), &numProfiles) != VA_STATUS_SUCCESS)
        return false;
    vaProfiles.resize(numProfiles);

    if (qstrcmp(codecName, "h264") == 0)
    {
        return vaProfiles.contains(VAProfileH264High)
                || vaProfiles.contains(VAProfileH264Main)
                || vaProfiles.contains(VAProfileH264ConstrainedBaseline);
    }
#if VA_VERSION_HEX >= 0x230000 // 1.3.0
    else if (qstrcmp(codecName, "vp8") == 0)
    {
        return vaProfiles.contains(VAProfileVP8Version0_3);
    }
#endif
#if VA_VERSION_HEX >= 0x250000 // 1.5.0
    else if (qstrcmp(codecName, "hevc") == 0)
    {
        return vaProfiles.contains(VAProfileHEVCMain);
    }
#endif
#if VA_VERSION_HEX >= 0x260000 // 1.6.0
    else if (qstrcmp(codecName, "vp9") == 0)
    {
        return vaProfiles.contains(VAProfileVP9Profile0)
#   if VA_VERSION_HEX >= 0x270000
                || vaProfiles.contains(VAProfileVP9Profile2)
#   endif
        ;
    }
#endif
    else if (qstrcmp(codecName, "mpeg2video") == 0)
    {
        return vaProfiles.contains(VAProfileMPEG2Main)
                || vaProfiles.contains(VAProfileMPEG2Simple);
    }
    else if (qstrcmp(codecName, "mpeg4") == 0)
    {
        return vaProfiles.contains(VAProfileMPEG4Main)
                || vaProfiles.contains(VAProfileMPEG4Simple)
                || vaProfiles.contains(VAProfileMPEG4AdvancedSimple);
    }
    else if (qstrcmp(codecName, "vc1") == 0 || qstrcmp(codecName, "wmv3") == 0)
    {
        return vaProfiles.contains(VAProfileVC1Advanced)
                || vaProfiles.contains(VAProfileVC1Main)
                || vaProfiles.contains(VAProfileVC1Simple);
    }
    else if (qstrcmp(codecName, "h263") == 0)
    {
        return vaProfiles.contains(VAProfileH263Baseline);
    }

    return false;
}
