// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <FFmpegHeaders.hpp>

#include <opengl/OpenGLHWInterop.hpp>

class VTBOpenGL final : public OpenGLHWInterop
{
public:
    VTBOpenGL(AVBufferRef *hwDeviceBufferRef);
    ~VTBOpenGL();

    QString name() const override;

    Format getFormat() const override;
    bool isTextureRectangle() const override;
    bool isCopy() const override;

    bool init(const int *widths, const int *heights, const SetTextureParamsFn &setTextureParamsFn) override;
    void clear() override;

    bool mapFrame(Frame &videoFrame) override;
    quint32 getTexture(int plane) override;

    Frame getCpuFrame(const Frame &videoFrame) override;

public:
    AVBufferRef *m_hwDeviceBufferRef = nullptr;

private:
    quint32 m_textures[2] = {};

    int m_widths[2] = {};
    int m_heights[2] = {};
};
