// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <CuvidHWInterop.hpp>
#include <opengl/OpenGLHWInterop.hpp>

class CuvidOpenGL final : public CuvidHWInterop, public OpenGLHWInterop
{
public:
    CuvidOpenGL(const std::shared_ptr<CUcontext> &cuCtx);
    ~CuvidOpenGL();

    QString name() const override;

    Format getFormat() const override;

    bool init(const int *widths, const int *heights, const SetTextureParamsFn &setTextureParamsFn) override;
    void clear() override;

    bool mapFrame(Frame &videoFrame) override;
    quint32 getTexture(int plane) override;

    Frame getCpuFrame(const Frame &videoFrame) override;

private:
    quint32 m_textures[2] = {};

    int m_widths[2] = {};
    int m_heights[2] = {};

    CUgraphicsResource m_res[2] = {};
};
