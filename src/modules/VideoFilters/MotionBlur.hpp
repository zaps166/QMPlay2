// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <VideoFilter.hpp>

class MotionBlur final : public VideoFilter
{
public:
    MotionBlur();

    bool filter(QQueue<Frame> &framesQueue) override;

    bool processParams(bool *paramsCorrected) override;
};

#define MotionBlurName "Motion Blur"
