/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2025  Błażej Szczygieł

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
