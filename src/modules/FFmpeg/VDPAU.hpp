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

#pragma once

#include <Frame.hpp>

#include <QCoreApplication>
#include <QMutex>

#include <deque>

#include <vdpau/vdpau.h>

class VDPAU
{
    Q_DECLARE_TR_FUNCTIONS(VDPAU)

public:
    VDPAU(AVBufferRef *hwDeviceBufferRef);
    ~VDPAU();

    bool init();
    bool checkCodec(const char *codecName);

    void registerPreemptionCallback(VdpPreemptionCallback callback, void *context);

    void clearBufferedFrames();

    void applyVideoAdjustment(int saturation, int hue, int sharpness);
    void setVideoMixerDeintNr(int deintMethod, bool nrEnabled, float nrLevel);

    void maybeCreateVideoMixer(int surfaceW, int surfaceH, const Frame &decoded);

    bool videoMixerRender(const Frame &videoFrame, VdpOutputSurface &id, VdpVideoMixerPictureStructure videoMixerPictureStructure);

    bool getYV12(Frame &decoded, VdpVideoSurface id);
    bool getRGB(uint8_t *dest, int width, int height);

private:
    void setCSCMatrix();
    void applyVideoMixerFeatures();

    bool setVideoMixerFeature(VdpBool enabled, VdpVideoMixerFeature feature, VdpVideoMixerAttribute attribute = VDP_INVALID_HANDLE, float value = 0.0f);

public:
    AVBufferRef *m_hwDeviceBufferRef = nullptr;

    VdpDevice m_device = VDP_INVALID_HANDLE;

    VdpVideoMixer m_mixer = VDP_INVALID_HANDLE;
    VdpOutputSurface m_outputSurface = VDP_INVALID_HANDLE;

    int m_surfaceW = 0;
    int m_surfaceH = 0;

    int m_frameW = 0;
    int m_frameH = 0;

    VdpColorStandard m_colorStandard = VDP_COLOR_STANDARD_ITUR_BT_601;
    bool m_isLimitedRange = true;

    float m_saturation = 1.0f;
    float m_hue = 0.0f;
    int m_sharpness = 0;

    int m_deintMethod = 0;
    bool m_nrEnabled = false;
    float m_nrLevel = 0.0f;

    bool m_mustSetCSCMatrix = false;
    bool m_mustApplyVideoMixerFeatures = false;

    QMutex m_framesMutex;
    std::deque<Frame> m_bufferedFrames;

    VdpGetProcAddress *vdp_get_proc_address = nullptr;
    VdpOutputSurfaceCreate *vdp_output_surface_create = nullptr;
    VdpOutputSurfaceDestroy *vdp_output_surface_destroy = nullptr;
    VdpVideoMixerCreate *vdp_video_mixer_create = nullptr;
    VdpVideoMixerSetFeatureEnables *vdp_video_mixer_set_feature_enables = nullptr;
    VdpVideoMixerDestroy *vdp_video_mixer_destroy = nullptr;
    VdpVideoMixerRender *vdp_video_mixer_render = nullptr;
    VdpVideoMixerSetAttributeValues *vdp_video_mixer_set_attribute_values = nullptr;
    VdpVideoSurfaceGetBitsYCbCr *vdp_surface_get_bits_ycbcr = nullptr;
    VdpOutputSurfaceGetBitsNative *vdp_surface_get_bits_native = nullptr;
    VdpDeviceDestroy *vdp_device_destroy = nullptr;
    VdpGenerateCSCMatrix *vdp_generate_csc_matrix = nullptr;
    VdpDecoderQueryCapabilities *vdp_decoder_query_capabilities = nullptr;
    VdpPreemptionCallbackRegister *vdp_preemption_callback_register = nullptr;
    VdpVideoMixerQueryFeatureSupport *vdp_video_mixer_query_feature_support = nullptr;
};
