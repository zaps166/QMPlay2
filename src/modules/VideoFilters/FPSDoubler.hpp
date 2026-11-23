// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <VideoFiltersHeaders.hpp>

#include <VideoFilter.hpp>

class FPSDoubler final : public VideoFilter
{
public:
    FPSDoubler(Module &module, bool &fullScreen);
    ~FPSDoubler();

    bool set() override;

    bool filter(QQueue<Frame> &framesQueue) override;

    bool processParams(bool *paramsCorrected) override;

private:
    bool &m_fullScreen;

    double m_minFps = 0.0;
    double m_maxFps = 0.0;
    bool m_onlyFullScreen = false;

    double m_fps = 0.0;

    double m_frameTimeSum = 0.0;
    int m_frames = 0;
};

#define FPSDoublerName "FPS Doubler"
