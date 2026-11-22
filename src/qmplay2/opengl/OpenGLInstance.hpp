// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
