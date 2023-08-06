/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2023  Błażej Szczygieł

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

#include <GPUInstance.hpp>

class OpenGLInstance final : public GPUInstance
{
public:
    bool init();

public:
    QString name() const override;
    QMPlay2CoreClass::Renderer renderer() const override;

    VideoWriter *createOrGetVideoOutput() override;

public:
    bool isGLES = false;

    bool hasVbo = false;
    bool hasPbo = false;

    bool hasMapBuffer = false;
    bool hasMapBufferRange = false;

    bool canUse16bitTexture = false;

    int glVer = 0;
};
