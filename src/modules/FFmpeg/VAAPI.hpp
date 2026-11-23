// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <FFmpegHeaders.hpp>

class Frame;

class VAAPI
{
    Q_DECLARE_TR_FUNCTIONS(VAAPI)

public:
    VAAPI();
    ~VAAPI();

    bool open();

    void init(int codecId, int width, int height, bool allowFilters);

    bool vaapiCreateSurface(VASurfaceID &surface, int w, int h);

    void maybeInitVPP(int surfaceW, int surfaceH);
    void clearVPP(bool resetAllowFilters = true);

    void applyVideoAdjustment(int brightness, int contrast, int saturation, int hue);

    bool filterVideo(const Frame &frame, VASurfaceID &id, int &field);

    VASurfaceID getVppId();

    void insertFrame(VASurfaceID id, AVFrame *frame);

    bool checkCodec(const char *codecName) const;

private:
    void clearVPPFrames();

public:
    AVBufferRef *m_hwDeviceBufferRef = nullptr;

    QString m_vendor;
    bool m_isMesaRadeon = false;
    QVersionNumber m_driverVersion; // Used for Mesa Radeon workarounds

    bool ok = false;

    int m_fd = -1;
    VADisplay VADisp = nullptr;

    int outW = 0, outH = 0;

    int m_codecId = 0;

    std::unique_ptr<QMutex> m_mutex;

    // Postprocessing
    VAProcDeinterlacingType vpp_deint_type = VAProcDeinterlacingNone;
    bool use_vpp = false;

private:
    int m_version = 0;

    // Postprocessing
    bool m_allowFilters = false;
    VAContextID context_vpp;
    VAConfigID config_vpp;
    VABufferID m_vppDeintBuff;
    VASurfaceID id_vpp;
    QVector<VASurfaceID> m_refs;
    VASurfaceID m_lastVppSurface;
    int m_nBackwardRefs, m_nForwardRefs;
    bool m_hasVppFrame = false;

    QHash<VASurfaceID, Frame> m_vppFrames;
};
