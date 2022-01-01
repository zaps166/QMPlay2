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
#include <VAAPI.hpp>

#include <memory>

class VAAPIOpenGL : public OpenGLHWInterop
{
    struct EGL;

public:
    VAAPIOpenGL(const std::shared_ptr<VAAPI> &vaapi);
    ~VAAPIOpenGL();

    QString name() const override;

    Format getFormat() const override;
    bool isCopy() const override;

    bool init(const int *widths, const int *heights, const SetTextureParamsFn &setTextureParamsFn) override;
    void clear() override;

    bool mapFrame(Frame &videoFrame) override;
    quint32 getTexture(int plane) override;

    QImage getImage(const Frame &videoFrame) override;

    /**/

    inline std::shared_ptr<VAAPI> getVAAPI() const
    {
        return m_vaapi;
    }

private:
    void clearTextures();

private:
    const std::shared_ptr<VAAPI> m_vaapi;
    const int m_numPlanes = 2;

    quint32 m_textures[2] = {};

    int m_widths[2] = {};
    int m_heights[2] = {};

    std::unique_ptr<EGL> m_egl;

    bool m_hasDmaBufImportModifiers = false;
};
