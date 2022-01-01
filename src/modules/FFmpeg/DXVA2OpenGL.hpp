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

#include <QOpenGLContext>
#include <QHash>

#include <functional>
#include <memory>

#include <GL/wglext.h>

#include <d3d9.h>
#include <dxva2api.h>

struct AVBufferRef;

using QMPlay2D3dDev9 = std::unique_ptr<IDirect3DDevice9, std::function<void(void *)>>;

class DXVA2OpenGL : public OpenGLHWInterop
{
public:
    DXVA2OpenGL(AVBufferRef *hwDeviceBufferRef);
    ~DXVA2OpenGL();

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

    bool checkCodec(const QByteArray &codecName, bool b10);

    bool initVideoProcessor();

private:
    QMPlay2D3dDev9 getD3dDevice9();

    bool ensureRenderTargets();
    bool unlockRenderTargets();

    void clearTextures();

public:
    AVBufferRef *m_hwDeviceBufferRef = nullptr;

private:
    using DXVA2CreateVideoServiceFn = HRESULT(WINAPI *)(IDirect3DDevice9 *, REFIID, void **);
    DXVA2CreateVideoServiceFn DXVA2CreateVideoService = nullptr;

    PFNWGLDXOPENDEVICENVPROC wglDXOpenDeviceNV = nullptr;
    PFNWGLDXSETRESOURCESHAREHANDLENVPROC wglDXSetResourceShareHandleNV = nullptr;
    PFNWGLDXREGISTEROBJECTNVPROC wglDXRegisterObjectNV = nullptr;
    PFNWGLDXLOCKOBJECTSNVPROC wglDXLockObjectsNV = nullptr;
    PFNWGLDXUNLOCKOBJECTSNVPROC wglDXUnlockObjectsNV = nullptr;
    PFNWGLDXUNREGISTEROBJECTNVPROC wglDXUnregisterObjectNV = nullptr;
    PFNWGLDXCLOSEDEVICENVPROC wglDXCloseDeviceNV = nullptr;

    IDirectXVideoProcessor *m_videoProcessor = nullptr;
    DXVA2_VideoSample m_videoSample = {};
    DXVA2_VideoProcessBltParams m_bltParams = {};

    bool m_dontReleaseRenderTarget = false;

    HANDLE m_glHandleD3D = nullptr;

    static constexpr int s_numRenderTargets = 2;
    struct RenderTarget
    {
        QHash<QPair<int, int>, IDirect3DSurface9 *> surfaces;
        IDirect3DSurface9 *surface = nullptr;
        HANDLE glSurface = nullptr;
        bool locked = false;
    } m_renderTargets[s_numRenderTargets];
    quint32 m_textures[2] = {};
    int m_renderTargetIdx = 0;
    int m_width = 0;
    int m_height = 0;
};
