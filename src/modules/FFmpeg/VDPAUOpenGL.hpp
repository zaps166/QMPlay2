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

#include <opengl/OpenGLHWInterop.hpp>
#include <VDPAU.hpp>

#include <QOpenGLContext>

class VDPAUOpenGL : public OpenGLHWInterop
{
public:
    VDPAUOpenGL(const std::shared_ptr<VDPAU> &vdpau);
    ~VDPAUOpenGL();

    QString name() const override;

    Format getFormat() const override;

    bool init(const int *widths, const int *heights, const SetTextureParamsFn &setTextureParamsFn) override;
    void clear() override;

    bool mapFrame(Frame &videoFrame) override;
    quint32 getTexture(int plane) override;

    QImage getImage(const Frame &videoFrame) override;

    void getVideAdjustmentCap(VideoAdjustment &videoAdjustmentCap) override;
    void setVideoAdjustment(const VideoAdjustment &videoAdjustment) override;

    /**/

    inline std::shared_ptr<VDPAU> getVDPAU() const
    {
        return m_vdpau;
    }

private:
    void clearSurfaces();

    void clearObsoleteSurfaces();

    void deleteGlSurface(VDPAUOutputSurface &surface);

private:
    using GLvdpauSurfaceNV = intptr_t;

    std::shared_ptr<VDPAU> m_vdpau;

    bool m_isInitialized = false;
    SetTextureParamsFn m_setTextureParamsFn;

    using PFNVDPAUInitNVPROC = void(*)(uintptr_t vdpDevice, VdpGetProcAddress getProcAddress);
    PFNVDPAUInitNVPROC VDPAUInitNV = nullptr;

    using PFNVDPAUFiniNVPROC = void(*)();
    PFNVDPAUFiniNVPROC VDPAUFiniNV = nullptr;

    using PFNVDPAURegisterSurfaceNVPROC = GLvdpauSurfaceNV(*)(uintptr_t vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames);
    PFNVDPAURegisterSurfaceNVPROC VDPAURegisterOutputSurfaceNV = nullptr;

    using PFNVDPAUUnregisterSurfaceNVPROC = void(*)(GLvdpauSurfaceNV surface);
    PFNVDPAUUnregisterSurfaceNVPROC VDPAUUnregisterSurfaceNV = nullptr;

    using PFNVDPAUSurfaceAccessNVPROC = void(*)(GLvdpauSurfaceNV surface, GLenum access);
    PFNVDPAUSurfaceAccessNVPROC VDPAUSurfaceAccessNV = nullptr;

    using PFNVDPAUMapUnmapSurfacesNVPROC = void(*)(GLsizei numSurfaces, const GLvdpauSurfaceNV *surfaces);
    PFNVDPAUMapUnmapSurfacesNVPROC VDPAUMapSurfacesNV = nullptr;
    PFNVDPAUMapUnmapSurfacesNVPROC VDPAUUnmapSurfacesNV = nullptr;
};
