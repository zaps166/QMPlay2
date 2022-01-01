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

#include <VDPAUOpenGL.hpp>
#include <FFCommon.hpp>

#include <QMPlay2Core.hpp>

#include <QDebug>
#include <QSet>

VDPAUOpenGL::VDPAUOpenGL(const std::shared_ptr<VDPAU> &vdpau)
    : m_vdpau(vdpau)
{}
VDPAUOpenGL::~VDPAUOpenGL()
{}

QString VDPAUOpenGL::name() const
{
    return VDPAUWriterName;
}

OpenGLHWInterop::Format VDPAUOpenGL::getFormat() const
{
    return RGB32;
}

bool VDPAUOpenGL::init(const int *widths, const int *heights, const SetTextureParamsFn &setTextureParamsFn)
{
    Q_UNUSED(widths)
    Q_UNUSED(heights)

    m_setTextureParamsFn = setTextureParamsFn;

    m_vdpau->m_outputSurfacesMutex.lock();
    clearObsoleteSurfaces();
    for (auto &&outputSurfacePair : m_vdpau->m_outputSurfacesMap)
    {
        auto &outputSurface = outputSurfacePair.second;
        if (outputSurface.glTexture != 0)
            m_setTextureParamsFn(outputSurface.glTexture);
    }
    m_vdpau->m_outputSurfacesMutex.unlock();

    if (m_isInitialized)
        return true;

    const auto context = QOpenGLContext::currentContext();
    if (!context)
    {
        QMPlay2Core.logError("VDPAU :: Unable to get OpenGL context");
        m_error = true;
        return false;
    }

    if (!context->extensions().contains("GL_NV_vdpau_interop"))
    {
        QMPlay2Core.logError("VDPAU :: GL_NV_vdpau_interop extension is not available");
        m_error = true;
        return false;
    }

    VDPAUInitNV = (PFNVDPAUInitNVPROC)context->getProcAddress("VDPAUInitNV");
    VDPAUFiniNV = (PFNVDPAUFiniNVPROC)context->getProcAddress("VDPAUFiniNV");
    VDPAURegisterOutputSurfaceNV = (PFNVDPAURegisterSurfaceNVPROC)context->getProcAddress("VDPAURegisterOutputSurfaceNV");
    VDPAUUnregisterSurfaceNV = (PFNVDPAUUnregisterSurfaceNVPROC)context->getProcAddress("VDPAUUnregisterSurfaceNV");
    VDPAUSurfaceAccessNV = (PFNVDPAUSurfaceAccessNVPROC)context->getProcAddress("VDPAUSurfaceAccessNV");
    VDPAUMapSurfacesNV = (PFNVDPAUMapUnmapSurfacesNVPROC)context->getProcAddress("VDPAUMapSurfacesNV");
    VDPAUUnmapSurfacesNV = (PFNVDPAUMapUnmapSurfacesNVPROC)context->getProcAddress("VDPAUUnmapSurfacesNV");
    if (!VDPAUInitNV || !VDPAUFiniNV || !VDPAURegisterOutputSurfaceNV || !VDPAUUnregisterSurfaceNV || !VDPAUSurfaceAccessNV || !VDPAUMapSurfacesNV || !VDPAUUnmapSurfacesNV)
    {
        QMPlay2Core.logError("VDPAU :: Unable to get VDPAU interop function pointers");
        m_error = true;
        return false;
    }

    VDPAUInitNV(m_vdpau->m_device, m_vdpau->vdp_get_proc_address);
    if (glGetError() != 0)
    {
        QMPlay2Core.logError("VDPAU :: Unable to initialize VDPAU <-> GL interop");
        m_error = true;
        return false;
    }

    m_isInitialized = true;
    return true;
}
void VDPAUOpenGL::clear()
{
    clearSurfaces();
    m_setTextureParamsFn = nullptr;

    if (!m_isInitialized)
        return;

    VDPAUFiniNV();

    VDPAUInitNV = nullptr;
    VDPAUFiniNV = nullptr;
    VDPAURegisterOutputSurfaceNV = nullptr;
    VDPAUUnregisterSurfaceNV = nullptr;
    VDPAUSurfaceAccessNV = nullptr;
    VDPAUMapSurfacesNV = nullptr;
    VDPAUUnmapSurfacesNV = nullptr;

    m_isInitialized = false;
}

bool VDPAUOpenGL::mapFrame(Frame &videoFrame)
{
    QMutexLocker locker(&m_vdpau->m_outputSurfacesMutex);

    clearObsoleteSurfaces();

    if (auto displayingOutputSurface = m_vdpau->getDisplayingOutputSurface())
    {
        VDPAUUnmapSurfacesNV(1, &displayingOutputSurface->glSurface);
        displayingOutputSurface->displaying = false;
    }

    auto it = m_vdpau->m_outputSurfacesMap.find(videoFrame.customData());
    if (it == m_vdpau->m_outputSurfacesMap.end())
        return false;

    auto &outputSurface = it->second;

    videoFrame.setOnDestroyFn(nullptr);
    outputSurface.busy = false;

    if (outputSurface.glTexture == 0)
    {
        glGenTextures(1, &outputSurface.glTexture);
        m_setTextureParamsFn(outputSurface.glTexture);
    }

    if (outputSurface.glSurface == 0)
    {
        outputSurface.glSurface = VDPAURegisterOutputSurfaceNV(outputSurface.surface, GL_TEXTURE_2D, 1, &outputSurface.glTexture);
        if (outputSurface.glSurface == 0)
        {
            m_error = true;
            return false;
        }
        VDPAUSurfaceAccessNV(outputSurface.glSurface, GL_READ_ONLY);
    }

    Q_ASSERT(!outputSurface.displaying);
    VDPAUMapSurfacesNV(1, &outputSurface.glSurface);
    if (glGetError() != 0)
    {
        m_error = true;
        return false;
    }
    outputSurface.displaying = true;

    return true;
}
quint32 VDPAUOpenGL::getTexture(int plane)
{
    Q_UNUSED(plane)
    QMutexLocker locker(&m_vdpau->m_outputSurfacesMutex);
    if (auto displayingOutputSurface = m_vdpau->getDisplayingOutputSurface())
        return displayingOutputSurface->glTexture;
    return 0;
}

QImage VDPAUOpenGL::getImage(const Frame &videoFrame)
{
    QImage img(videoFrame.width(), videoFrame.height(), QImage::Format_RGB32);
    if (m_vdpau->getRGB(img.bits(), videoFrame.width(), videoFrame.height()))
        return img;
    return QImage();
}

void VDPAUOpenGL::getVideAdjustmentCap(VideoAdjustment &videoAdjustmentCap)
{
    videoAdjustmentCap.brightness = false;
    videoAdjustmentCap.contrast = false;
    videoAdjustmentCap.saturation = true;
    videoAdjustmentCap.hue = true;
    videoAdjustmentCap.sharpness = true;
}
void VDPAUOpenGL::setVideoAdjustment(const VideoAdjustment &videoAdjustment)
{
    m_vdpau->applyVideoAdjustment(videoAdjustment.saturation, videoAdjustment.hue, videoAdjustment.sharpness);
}

void VDPAUOpenGL::clearSurfaces()
{
    QMutexLocker locker(&m_vdpau->m_outputSurfacesMutex);
    for (auto &&outputSurfacePair : m_vdpau->m_outputSurfacesMap)
        deleteGlSurface(outputSurfacePair.second);
    clearObsoleteSurfaces();
}

void VDPAUOpenGL::clearObsoleteSurfaces()
{
    for (auto it = m_vdpau->m_outputSurfacesMap.begin(); it != m_vdpau->m_outputSurfacesMap.end();)
    {
        auto &outputSurface = it->second;
        if (outputSurface.obsolete && !outputSurface.busy && !outputSurface.displaying)
        {
            deleteGlSurface(outputSurface);
            m_vdpau->vdp_output_surface_destroy(outputSurface.surface);
            it = m_vdpau->m_outputSurfacesMap.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void VDPAUOpenGL::deleteGlSurface(VDPAUOutputSurface &surface)
{
    if (surface.displaying)
    {
        VDPAUUnmapSurfacesNV(1, &surface.glSurface);
        surface.displaying = false;
    }
    if (surface.glSurface != 0)
    {
        VDPAUUnregisterSurfaceNV(surface.glSurface);
        surface.glSurface = 0;
    }
    if (surface.glTexture != 0)
    {
        glDeleteTextures(1, &surface.glTexture);
        surface.glTexture = 0;
    }
}
