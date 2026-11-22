// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <opengl/OpenGLHWInterop.hpp>
#include <VAAPI.hpp>

#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <mutex>

#include <va/va_drmcommon.h>

class VAAPIOpenGL : public OpenGLHWInterop
{
    struct EGL;

public:
    VAAPIOpenGL();
    ~VAAPIOpenGL();

    QString name() const override;

    Format getFormat() const override;
    bool isCopy() const override;

    bool init(const int *widths, const int *heights, const SetTextureParamsFn &setTextureParamsFn) override;
    void clear() override;

    bool mapFrame(Frame &videoFrame) override;
    quint32 getTexture(int plane) override;

    Frame getCpuFrame(const Frame &videoFrame) override;

    /**/

    inline void setVAAPI(const std::shared_ptr<VAAPI> &vaapi)
    {
        m_vaapi = vaapi;
    }
    inline std::shared_ptr<VAAPI> getVAAPI() const
    {
        return m_vaapi;
    }

    void clearSurfaces(bool lock = true);

public:
    void insertAvailableSurface(uintptr_t id);

private:
    void clearTextures();

    static void closeFDs(const VADRMPRIMESurfaceDescriptor &vaSurfaceDescr);

private:
    std::shared_ptr<VAAPI> m_vaapi;
    const int m_numPlanes = 2;

    quint32 m_textures[2] = {};

    int m_widths[2] = {};
    int m_heights[2] = {};

    std::unique_ptr<EGL> m_egl;

    bool m_hasDmaBufImportModifiers = false;

    std::mutex m_mutex;

    std::unordered_set<uintptr_t> m_availableSurfaces;
    std::unordered_map<VASurfaceID, VADRMPRIMESurfaceDescriptor> m_surfaces;
};
