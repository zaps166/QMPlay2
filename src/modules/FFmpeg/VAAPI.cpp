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

#include <VAAPI.hpp>
#include <FFCommon.hpp>
#include <Functions.hpp>
#include <ImgScaler.hpp>
#include <Frame.hpp>
#include <QMPlay2Core.hpp>

#ifdef USE_VULKAN
#   include "../../qmvk/PhysicalDevice.hpp"

#   include <vulkan/VulkanInstance.hpp>
#endif

extern "C"
{
    #include <libavutil/buffer.h>
}

#include <QGuiApplication>
#include <QFileInfo>
#include <QDebug>
#include <QDir>

#include <va/va_drm.h>

#include <unistd.h>
#include <fcntl.h>

VAAPI::VAAPI()
{
#ifdef FIND_HWACCEL_DRIVERS_PATH
    FFCommon::setDriversPath("dri", "LIBVA_DRIVERS_PATH");
#endif
}
VAAPI::~VAAPI()
{
    clearVPP(); // Must be freed before destroying VADisplay
    av_buffer_unref(&m_hwDeviceBufferRef);
    if (VADisp)
    {
        vaTerminate(VADisp);
        if (m_fd > -1)
            ::close(m_fd);
    }
}

bool VAAPI::open(bool checkNV12)
{
    clearVPP();

    int major = 0, minor = 0;

    QString devFilePath;

    const auto eglCardFilePath = qgetenv("QMPLAY2_EGL_CARD_FILE_PATH");
    if (QMPlay2Core.isVulkanRenderer())
    {
#ifdef USE_VULKAN
        using namespace QmVk;
        const auto linuxPCIPath = QString::fromStdString(
            static_pointer_cast<Instance>(QMPlay2Core.gpuInstance())->physicalDevice()->linuxPCIPath()
        );
        if (!linuxPCIPath.isEmpty())
        {
            const auto renders = QDir("/dev/dri/by-path").entryInfoList({"*-render"}, QDir::Files | QDir::System);
            for (auto &&render : renders)
            {
                if (render.fileName().contains(linuxPCIPath))
                {
                    devFilePath = render.symLinkTarget();
                    break;
                }
            }
        }
#endif
    }
    else if (!eglCardFilePath.isEmpty())
    {
        const auto cards = QDir("/dev/dri/by-path").entryInfoList({"*-card"}, QDir::Files | QDir::System);
        for (auto &&card : cards)
        {
            if (card.symLinkTarget() == eglCardFilePath)
            {
                devFilePath = QFileInfo(card.filePath().replace("-card", "-render")).symLinkTarget();
                break;
            }
        }
    }

    const auto devs = QDir("/dev/dri").entryInfoList({"renderD*"}, QDir::Files | QDir::System, QDir::Name);
    for (auto &&dev : devs)
    {
        if (!devFilePath.isEmpty() && devFilePath != dev.filePath())
            continue;

        const int  fd = ::open(dev.filePath().toLocal8Bit().constData(), O_RDWR);
        if (fd < 0)
            continue;

        if (auto disp = vaGetDisplayDRM(fd))
        {
            if (vaInitialize(disp, &major, &minor) == VA_STATUS_SUCCESS)
            {
                m_fd = fd;
                VADisp = disp;
                qDebug().noquote() << "VA-API :: Initialized device:" << dev.fileName();
                break;
            }
            qDebug().noquote() << "VA-API :: Unable to initialize device:" << dev.fileName();
            vaTerminate(disp);
        }

        ::close(fd);
    }

    if (!VADisp)
        return false;

    m_version = (major << 8) | minor;

    m_vendor = vaQueryVendorString(VADisp);
    if (m_vendor.isEmpty())
        return false;

    int fmtCount = vaMaxNumImageFormats(VADisp);
    VAImageFormat imgFmt[fmtCount];
    if (fmtCount > 0 && vaQueryImageFormats(VADisp, imgFmt, &fmtCount) == VA_STATUS_SUCCESS)
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
    if (checkNV12 && nv12ImageFmt.fourcc != VA_FOURCC_NV12)
        return false;

    return true;
}

void VAAPI::init(int width, int height,  bool allowFilters)
{
    clearVPP();

    outW = width;
    outH = height;

    m_allowFilters = allowFilters;

    ok = true;
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
    if (!m_allowFilters || use_vpp || surfaceW < outW || surfaceH < outH)
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
                        bool vpp_deint_types[3] = {false};
                        for (unsigned j = 0; j < num_deinterlacing_caps; ++j)
                        {
                            switch (deinterlacing_caps[j].type)
                            {
                                case VAProcDeinterlacingBob:
                                    vpp_deint_types[0] = true;
                                    break;
                                case VAProcDeinterlacingMotionAdaptive:
                                    vpp_deint_types[1] = true;
                                    break;
                                case VAProcDeinterlacingMotionCompensated:
                                    vpp_deint_types[2] = true;
                                    break;
                                default:
                                    break;
                            }
                        }
                        if (vpp_deint_type == VAProcDeinterlacingMotionCompensated && !vpp_deint_types[2])
                        {
                            QMPlay2Core.log("VA-API :: " + tr("Unsupported deinterlacing algorithm") + " - Motion compensated", ErrorLog | LogOnce);
                            vpp_deint_type = VAProcDeinterlacingMotionAdaptive;
                        }
                        if (vpp_deint_type == VAProcDeinterlacingMotionAdaptive && !vpp_deint_types[1])
                        {
                            QMPlay2Core.log("VA-API :: " + tr("Unsupported deinterlacing algorithm") + " - Motion adaptive", ErrorLog | LogOnce);
                            vpp_deint_type = (m_fd > -1) ? VAProcDeinterlacingBob : VAProcDeinterlacingNone;
                        }
                        if (vpp_deint_type == VAProcDeinterlacingBob && !vpp_deint_types[0])
                        {
                            QMPlay2Core.log("VA-API :: " + tr("Unsupported deinterlacing algorithm") + " - Bob", ErrorLog | LogOnce);
                            vpp_deint_type = VAProcDeinterlacingNone;
                        }
                        if (vpp_deint_type != VAProcDeinterlacingNone)
                        {
                            VAProcFilterParameterBufferDeinterlacing deint_params = {};
                            deint_params.type = VAProcFilterDeinterlacing;
                            deint_params.algorithm = vpp_deint_type;
                            if (vaCreateBuffer(VADisp, context_vpp, VAProcFilterParameterBufferType, sizeof deint_params, 1, &deint_params, &m_vppDeintBuff) != VA_STATUS_SUCCESS)
                                m_vppDeintBuff = VA_INVALID_ID;
                        }
                        break;
                    }
                }

                VAProcPipelineCaps caps = {};
                if (vaQueryVideoProcPipelineCaps(VADisp, config_vpp, &m_vppDeintBuff, 1, &caps) == VA_STATUS_SUCCESS)
                {
                    m_nBackwardRefs = caps.num_backward_references;
                    m_nForwardRefs = caps.num_forward_references;
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
    if (vpp_deint_type == VAProcDeinterlacingBob && m_fd < 0)
        vpp_deint_type = VAProcDeinterlacingNone;
    if (use_vpp)
    {
        if (m_vppDeintBuff != VA_INVALID_ID)
            vaDestroyBuffer(VADisp, m_vppDeintBuff);
        if (id_vpp != VA_INVALID_SURFACE)
            vaDestroySurfaces(VADisp, &id_vpp, 1);
        if (context_vpp)
            vaDestroyContext(VADisp, context_vpp);
        if (config_vpp)
            vaDestroyConfig(VADisp, config_vpp);
        use_vpp = false;
    }
    clearVPPFrames();
    m_vppDeintBuff = VA_INVALID_ID;
    context_vpp = 0;
    config_vpp = 0;
    m_nBackwardRefs = m_nForwardRefs = 0;
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

bool VAAPI::filterVideo(const Frame &frame, VASurfaceID &id, int &field)
{
    const bool doDeint = (field != VA_FRAME_PICTURE && m_vppDeintBuff != VA_INVALID_ID);

    if (use_vpp && !doDeint)
        clearVPPFrames();

    const VASurfaceID currId = frame.hwData();

    if (!use_vpp || !doDeint)
    {
        id = currId;
        return true;
    }

    // Example: {future(backward ref, frame 3), current(frame 2), past(forward ref, frame 1), past (forward ref, frame 0)}
    const int requiredRefs = m_nBackwardRefs + 1 + m_nForwardRefs;
    if (m_refs.count() != requiredRefs)
    {
        m_refs.fill(currId, requiredRefs);
    }
    else if (!frame.isSecondField())
    {
        m_vppFrames.remove(m_refs.takeLast());
        m_vppFrames.insert(currId, frame);
        m_refs.push_front(currId);
    }

    VAProcFilterParameterBufferDeinterlacing *deintParams = nullptr;
    if (vaMapBuffer(VADisp, m_vppDeintBuff, (void **)&deintParams) != VA_STATUS_SUCCESS)
    {
        ok = false;
        return false;
    }

    deintParams->flags = (field == VA_TOP_FIELD) ? 0 : VA_DEINTERLACING_BOTTOM_FIELD;
    if (frame.isSecondField() == (field == VA_TOP_FIELD))
        deintParams->flags |= VA_DEINTERLACING_BOTTOM_FIELD_FIRST;
    vaUnmapBuffer(VADisp, m_vppDeintBuff);

    VAProcPipelineParameterBuffer pipelineParam = {};

    pipelineParam.surface = m_refs[m_nBackwardRefs];
    pipelineParam.output_background_color = 0xFF000000;

    pipelineParam.num_filters = 1;
    pipelineParam.filters = &m_vppDeintBuff;

    if (m_nForwardRefs > 0)
    {
        pipelineParam.forward_references = &m_refs[m_nBackwardRefs + 1];
        pipelineParam.num_forward_references = m_nForwardRefs;
    }

    if (m_nBackwardRefs > 0)
    {
        pipelineParam.backward_references = &m_refs[0];
        pipelineParam.num_backward_references = m_nBackwardRefs;
    }

    VABufferID pipelineBuf = VA_INVALID_ID;
    if (vaCreateBuffer(VADisp,
                       context_vpp,
                       VAProcPipelineParameterBufferType,
                       sizeof(pipelineParam),
                       1,
                       &pipelineParam,
                       &pipelineBuf) != VA_STATUS_SUCCESS)
    {
        ok = false;
        return false;
    }

    if (vaBeginPicture(VADisp, context_vpp, id_vpp) != VA_STATUS_SUCCESS)
    {
        ok = false;
        return false;
    }

    ok = (vaRenderPicture(VADisp, context_vpp, &pipelineBuf, 1) == VA_STATUS_SUCCESS);
    vaEndPicture(VADisp, context_vpp);

    if (!ok || m_version >= 0x0028)
        vaDestroyBuffer(VADisp, pipelineBuf);

    if (!ok)
        return false;

    id = id_vpp;
    field = 0;
    m_hasVppFrame = true;
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
QImage VAAPI::getImage(const Frame &videoFrame) const
{
    ImgScaler imgScaler;
    if (!imgScaler.create(videoFrame))
        return QImage();

    VAImage image;
    quint8 *vaData = getNV12Image(image, m_hasVppFrame ? id_vpp : videoFrame.hwData());
    if (vaData)
    {
        const void *data[2] = {
            vaData + image.offsets[0],
            vaData + image.offsets[1]
        };
        QImage img(videoFrame.width(), videoFrame.height(), QImage::Format_RGB32);
        imgScaler.scale(data, (const int *)image.pitches, img.bits());
        vaUnmapBuffer(VADisp, image.buf);
        vaDestroyImage(VADisp, image.image_id);
        return img;
    }
    return QImage();
}

bool VAAPI::checkCodec(const char *codecName) const
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

void VAAPI::clearVPPFrames()
{
    m_refs.clear();
    m_vppFrames.clear();
    m_hasVppFrame = false;
}
