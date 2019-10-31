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

#include <VDPAU.hpp>
#include <FFCommon.hpp>

#include <QMPlay2Core.hpp>
#include <VideoFrame.hpp>
#include <Functions.hpp>

#include <QDebug>

#include <vdpau/vdpau_x11.h>

VDPAU::VDPAU()
{
}
VDPAU::~VDPAU()
{
    if (m_outputSurface != VDP_INVALID_HANDLE)
        vdp_output_surface_destroy(m_outputSurface);
    if (m_mixer != VDP_INVALID_HANDLE)
        vdp_video_mixer_destroy(m_mixer);
    clearBufferedFrames();
    if (m_device != VDP_INVALID_HANDLE && vdp_device_destroy)
        vdp_device_destroy(m_device);
    if (m_display)
        XCloseDisplay(m_display);
}

bool VDPAU::open(const char *codecName)
{
    Q_ASSERT(!m_display);

    m_display = XOpenDisplay(nullptr);
    if (!m_display)
        return false;

    if (vdp_device_create_x11(m_display, 0, &m_device, &vdp_get_proc_address) != VDP_STATUS_OK)
        return false;

    int status = VDP_STATUS_OK;
    status |= vdp_get_proc_address(m_device, VDP_FUNC_ID_OUTPUT_SURFACE_CREATE, (void **)&vdp_output_surface_create);
    status |= vdp_get_proc_address(m_device, VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY, (void **)&vdp_output_surface_destroy);
    status |= vdp_get_proc_address(m_device, VDP_FUNC_ID_VIDEO_MIXER_CREATE, (void **)&vdp_video_mixer_create);
    status |= vdp_get_proc_address(m_device, VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES, (void **)&vdp_video_mixer_set_feature_enables);
    status |= vdp_get_proc_address(m_device, VDP_FUNC_ID_VIDEO_MIXER_DESTROY, (void **)&vdp_video_mixer_destroy);
    status |= vdp_get_proc_address(m_device, VDP_FUNC_ID_VIDEO_MIXER_RENDER, (void **)&vdp_video_mixer_render);
    status |= vdp_get_proc_address(m_device, VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES, (void **)&vdp_video_mixer_set_attribute_values);
    status |= vdp_get_proc_address(m_device, VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR, (void **)&vdp_surface_get_bits_ycbcr);
    status |= vdp_get_proc_address(m_device, VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE, (void **)&vdp_surface_get_bits_native);
    status |= vdp_get_proc_address(m_device, VDP_FUNC_ID_DEVICE_DESTROY, (void **)&vdp_device_destroy);
    status |= vdp_get_proc_address(m_device, VDP_FUNC_ID_GENERATE_CSC_MATRIX, (void **)&vdp_generate_csc_matrix);
    status |= vdp_get_proc_address(m_device, VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES, (void **)&vdp_decoder_query_capabilities);
    status |= vdp_get_proc_address(m_device, VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER, (void **)&vdp_preemption_callback_register);
    status |= vdp_get_proc_address(m_device, VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT, (void **)&vdp_video_mixer_query_feature_support);
    if (status != VDP_STATUS_OK)
        return false;

    auto checkCodecAvailability = [this](const std::initializer_list<VdpDecoderProfile> &profiles) {
        VdpBool isSupported = false;
        uint32_t out[4] = {};
        for (auto &&profile : profiles)
        {
            const VdpStatus ret = vdp_decoder_query_capabilities(
                m_device,
                profile,
                &isSupported,
                out + 0,
                out + 1,
                out + 2,
                out + 3
            );
            if (ret == VDP_STATUS_OK && isSupported)
                return true;
        }
        return false;
    };

    bool supported = false;

    if (qstrcmp(codecName, "h264") == 0)
        supported = checkCodecAvailability({VDP_DECODER_PROFILE_H264_HIGH, VDP_DECODER_PROFILE_H264_MAIN, VDP_DECODER_PROFILE_H264_BASELINE});
#ifdef VDP_DECODER_PROFILE_HEVC_MAIN
    else if (qstrcmp(codecName, "hevc") == 0)
        supported = checkCodecAvailability({VDP_DECODER_PROFILE_HEVC_MAIN});
#endif
#ifdef VDP_DECODER_PROFILE_VP9_PROFILE_0
    else if (qstrcmp(codecName, "vp9") == 0)
        supported = checkCodecAvailability({VDP_DECODER_PROFILE_VP9_PROFILE_0});
#endif
    else if (qstrcmp(codecName, "mpeg2video") == 0)
        supported = checkCodecAvailability({VDP_DECODER_PROFILE_MPEG2_MAIN, VDP_DECODER_PROFILE_MPEG2_SIMPLE});
    else if (qstrcmp(codecName, "mpeg4") == 0)
        supported = checkCodecAvailability({VDP_DECODER_PROFILE_MPEG4_PART2_ASP, VDP_DECODER_PROFILE_MPEG4_PART2_SP});
    else if (qstrcmp(codecName, "vc1") == 0)
        supported = checkCodecAvailability({VDP_DECODER_PROFILE_VC1_ADVANCED, VDP_DECODER_PROFILE_VC1_MAIN, VDP_DECODER_PROFILE_VC1_SIMPLE});
    else if (qstrcmp(codecName, "mpeg1video") == 0)
        supported = checkCodecAvailability({VDP_DECODER_PROFILE_MPEG1});

    if (!supported)
        return false;

    return true;
}

void VDPAU::registerPreemptionCallback(VdpPreemptionCallback callback, void *context)
{
    vdp_preemption_callback_register(m_device, callback, context);
}

void VDPAU::clearBufferedFrames()
{
    m_bufferedFrames.clear();
}

void VDPAU::applyVideoAdjustment(int saturation, int hue, int sharpness)
{
    m_saturation = (saturation / 100.0f) + 1.0f;
    m_hue = hue / 31.830989f;
    m_sharpness = sharpness;
    m_mustSetCSCMatrix = true;
    m_mustApplyVideoMixerFeatures = true; // For sharpness
}
void VDPAU::setVideoMixerDeintNr(int deintMethod, bool nrEnabled, float nrLevel)
{
    m_deintMethod = deintMethod;
    m_nrEnabled = nrEnabled;
    m_nrLevel = nrLevel;
    m_mustApplyVideoMixerFeatures = true;
}

void VDPAU::maybeCreateVideoMixer(int surfaceW, int surfaceH, const VideoFrame &decoded)
{
    VdpColorStandard colorStandard;
    switch (decoded.colorSpace)
    {
        case QMPlay2ColorSpace::BT709:
            colorStandard = VDP_COLOR_STANDARD_ITUR_BT_709;
            break;
        case QMPlay2ColorSpace::SMPTE240M:
            colorStandard = VDP_COLOR_STANDARD_SMPTE_240M;
            break;
        default:
            colorStandard = VDP_COLOR_STANDARD_ITUR_BT_601;
            break;
    }
    if (m_colorStandard != colorStandard || m_isLimitedRange != decoded.limited)
    {
        m_colorStandard = colorStandard;
        m_isLimitedRange = decoded.limited;
        m_mustSetCSCMatrix = true;
    }

    if (m_surfaceW != surfaceW || m_surfaceH != surfaceH)
    {
        m_surfaceW = surfaceW;
        m_surfaceH = surfaceH;
        if (m_mixer != VDP_INVALID_HANDLE)
        {
            vdp_video_mixer_destroy(m_mixer);
            m_mixer = VDP_INVALID_HANDLE;
        }
        clearBufferedFrames();
    }

    if (m_mixer != VDP_INVALID_HANDLE)
        return;

    const uint32_t featureCount = 4;
    const VdpVideoMixerFeature features[] = {
        VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL,
        VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL,
        VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION,
        VDP_VIDEO_MIXER_FEATURE_SHARPNESS,
    };

    const uint32_t parameterCount = 3;
    const VdpVideoMixerParameter parameters[parameterCount] =
    {
        VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH,
        VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT,
        VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE,
    };
    const VdpChromaType chromaType = VDP_CHROMA_TYPE_420;
    const void *const parameterValues[parameterCount] =
    {
        &surfaceW,
        &surfaceH,
        &chromaType
    };

    vdp_video_mixer_create(
        m_device,
        featureCount,
        features,
        parameterCount,
        parameters,
        parameterValues,
        &m_mixer
    );

    m_mustSetCSCMatrix = true;
}

bool VDPAU::videoMixerRender(const VideoFrame &videoFrame, VdpOutputSurface &id, VdpVideoMixerPictureStructure videoMixerPictureStructure)
{
    if (m_frameW != videoFrame.size.width || m_frameH != videoFrame.size.height)
    {
        m_frameW = videoFrame.size.width;
        m_frameH = videoFrame.size.height;
        if (m_outputSurface != VDP_INVALID_HANDLE)
        {
            vdp_output_surface_destroy(m_outputSurface);
            m_outputSurface = VDP_INVALID_HANDLE;
        }
        clearBufferedFrames();
    }

    if (m_outputSurface == VDP_INVALID_HANDLE)
    {
        VdpStatus status = vdp_output_surface_create(
            m_device,
            VDP_RGBA_FORMAT_B8G8R8A8,
            m_frameW,
            m_frameH,
            &m_outputSurface
        );
        if (status != VDP_STATUS_OK)
            return false;
    }

    if (m_mustSetCSCMatrix)
    {
        setCSCMatrix();
        m_mustSetCSCMatrix = false;
    }
    if (m_mustApplyVideoMixerFeatures)
    {
        applyVideoMixerFeatures();
        m_mustApplyVideoMixerFeatures = false;
    }

    constexpr size_t numVideoFrames = 4; // 2 past, 1 current, 1 future
    VdpVideoSurface surfaces[numVideoFrames];

    if ((videoMixerPictureStructure == VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME || m_deintMethod == 0) && !m_nrEnabled)
    {
        clearBufferedFrames();
        for (size_t i = 0; i < numVideoFrames; ++i)
            surfaces[i] = (i == 1) ? videoFrame.surfaceId : VDP_INVALID_HANDLE;
    }
    else
    {
        if (m_bufferedFrames.empty() || m_bufferedFrames[0].surfaceId != videoFrame.surfaceId)
        {
            while (m_bufferedFrames.size() >= numVideoFrames)
                m_bufferedFrames.pop_back();
            m_bufferedFrames.push_front(videoFrame);
        }

        for (size_t i = 0; i < numVideoFrames; ++i)
        {
            surfaces[i] = (m_bufferedFrames.size() > i)
                ? m_bufferedFrames[i].surfaceId
                : VDP_INVALID_HANDLE
            ;
        }
        if (surfaces[1] == VDP_INVALID_HANDLE)
            surfaces[1] = surfaces[0];
    }

    const VdpRect srcRect {
        0u,
        0u,
        (uint32_t)m_frameW,
        (uint32_t)m_frameH,
    };
    VdpStatus status = vdp_video_mixer_render(
        m_mixer,
        VDP_INVALID_HANDLE, nullptr,
        videoMixerPictureStructure,
        2, &surfaces[2],
        surfaces[1],
        1, &surfaces[0],
        &srcRect,
        m_outputSurface, nullptr, nullptr,
        0, nullptr
    );
    if (status != VDP_STATUS_OK)
    {
        if (status == VDP_STATUS_INVALID_HANDLE)
        {
            id = VDP_INVALID_HANDLE;
            return true;
        }
        return false;
    }

    id = m_outputSurface;
    return true;
}

bool VDPAU::getNV12(VideoFrame &decoded, VdpVideoSurface id)
{
    void *data[] = {
        decoded.buffer[0].data(),
        decoded.buffer[2].data(),
        decoded.buffer[1].data()
    };
    return (vdp_surface_get_bits_ycbcr(id, VDP_YCBCR_FORMAT_YV12, data, (uint32_t *)decoded.linesize) == VDP_STATUS_OK);
}
bool VDPAU::getRGB(uint8_t *dest, const VideoFrameSize &size)
{
    if (m_outputSurface == VDP_INVALID_HANDLE || !dest)
        return false;

    if (size.width != m_frameW || size.height != m_frameH)
        return false;

    const uint32_t lineSize = Functions::aligned(m_frameW, 8) * 4;
    if (vdp_surface_get_bits_native(m_outputSurface, nullptr, (void **)&dest, &lineSize) == VDP_STATUS_OK)
    {
        // FIXME: Don't align RGB frame in VideoThr
        for (int y = 0; y < m_frameH; ++y)
        {
            for (uint32_t x = m_frameW * 4; x < lineSize; ++x)
            {
                dest[y * lineSize + x] = 0;
            }
        }
        return true;
    }

    return false;
}

void VDPAU::setCSCMatrix()
{
    VdpProcamp procamp = {
        VDP_PROCAMP_VERSION,
        m_isLimitedRange ? 0.0f : (16.0f / 255.0f),
        1.0f,
        m_saturation,
        m_hue,
    };

    VdpCSCMatrix matrix = {};

    if (vdp_generate_csc_matrix(&procamp, m_colorStandard, &matrix) != VDP_STATUS_OK)
        return;

    if (!m_isLimitedRange)
    {
        constexpr float coeff = 255.0f / (235.0f - 16.0f);
        constexpr int n = sizeof(matrix) / sizeof(float);
        auto matrixData = (float *)matrix;
        for (int i = 0; i < n; ++i)
            matrixData[i] /= coeff;
    }

    const VdpVideoMixerAttribute attributes[] = {
        VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX,
    };
    const void *attributeValues[] = {
        &matrix,
    };
    vdp_video_mixer_set_attribute_values(m_mixer, 1, attributes, attributeValues);
}
void VDPAU::applyVideoMixerFeatures()
{
    const bool sharpness = setVideoMixerFeature(
        m_sharpness != 0,
        VDP_VIDEO_MIXER_FEATURE_SHARPNESS,
        VDP_VIDEO_MIXER_ATTRIBUTE_SHARPNESS_LEVEL,
        m_sharpness / 100.0f
    );
    if (m_sharpness != 0 && !sharpness)
        QMPlay2Core.log(tr("Unsupported image sharpness filter"), ErrorLog | LogOnce);

    const bool temporalDeint = setVideoMixerFeature(
        m_deintMethod == 1,
        VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL
    );
    const bool temploralSpatialDeint = setVideoMixerFeature(
        m_deintMethod == 2,
        VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL
    );

    const bool nr = setVideoMixerFeature(
        m_nrEnabled,
        VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION,
        VDP_VIDEO_MIXER_ATTRIBUTE_NOISE_REDUCTION_LEVEL,
        m_nrLevel
    );

    if (m_deintMethod == 1 && !temporalDeint)
        QMPlay2Core.log(tr("Unsupported deinterlacing algorithm") + " - Temporal", ErrorLog | LogOnce);
    if (m_deintMethod == 2 && !temploralSpatialDeint)
        QMPlay2Core.log(tr("Unsupported deinterlacing algorithm") + " - Temporal-spatial", ErrorLog | LogOnce);
    if (m_nrEnabled && !nr)
        QMPlay2Core.log(tr("Unsupported noise reduction filter"), ErrorLog | LogOnce);
}

bool VDPAU::setVideoMixerFeature(VdpBool enabled, VdpVideoMixerFeature feature, VdpVideoMixerAttribute attribute, float value)
{
    VdpBool supported = false;
    if (vdp_video_mixer_query_feature_support(m_device, feature, &supported) != VDP_STATUS_OK)
        return false;
    if (!supported)
        return false;

    if (attribute != VDP_INVALID_HANDLE)
    {
        const void *values[] = {
            &value,
        };
        vdp_video_mixer_set_attribute_values(m_mixer, 1, &attribute, values);
    }

    vdp_video_mixer_set_feature_enables(m_mixer, 1, &feature, &enabled);

    return true;
}
