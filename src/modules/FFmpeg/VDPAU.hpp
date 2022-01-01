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

#pragma once

#include <VideoFilter.hpp>

#include <QCoreApplication>
#include <QMutex>
#include <QSize>

#include <unordered_map>

#include <vdpau/vdpau.h>

struct VDPAUOutputSurface
{
    VdpOutputSurface surface = VDP_INVALID_HANDLE;
    uint32_t glTexture = 0;
    intptr_t glSurface = 0;
    bool busy = false;
    bool displaying = false;
    bool obsolete = false;
};

class VDPAU : public VideoFilter, public std::enable_shared_from_this<VDPAU>
{
    Q_DECLARE_TR_FUNCTIONS(VDPAU)

public:
    VDPAU(AVBufferRef *hwDeviceBufferRef);
    ~VDPAU();

    bool init();
    bool checkCodec(const char *codecName);

    void registerPreemptionCallback(VdpPreemptionCallback callback, void *context);

    bool hasError() const;

    void applyVideoAdjustment(int saturation, int hue, int sharpness);
    void setVideoMixerDeintNr(int deintMethod, bool nrEnabled, float nrLevel);

    void maybeCreateVideoMixer(int surfaceW, int surfaceH, const Frame &decoded);

    bool getYV12(Frame &decoded, VdpVideoSurface id);
    bool getRGB(uint8_t *dest, int width, int height);

    VDPAUOutputSurface *getDisplayingOutputSurface();

public:
    void clearBuffer() override;

    bool filter(QQueue<Frame> &framesQueue) override;

    bool processParams(bool *paramsCorrected) override;

private:
    void setCSCMatrix();
    void applyVideoMixerFeatures();

    bool setVideoMixerFeature(VdpBool enabled, VdpVideoMixerFeature feature, VdpVideoMixerAttribute attribute = VDP_INVALID_HANDLE, float value = 0.0f);

public:
    AVBufferRef *m_hwDeviceBufferRef = nullptr;

    VdpDevice m_device = VDP_INVALID_HANDLE;

    VdpVideoMixer m_mixer = VDP_INVALID_HANDLE;
    QSize m_surfaceSize;

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

    QMutex m_outputSurfacesMutex;
    std::unordered_map<quintptr, VDPAUOutputSurface> m_outputSurfacesMap;
    quintptr m_id = 0;
    QSize m_outputSurfaceSize;

    bool m_deinterlace = false;
    std::atomic_bool m_error {false};

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
