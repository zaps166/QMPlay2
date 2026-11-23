// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <SubtitlesHeaders.hpp>

#include <SubsDec.hpp>

class Classic final : public SubsDec
{
public:
    Classic(bool, double);
private:
    bool toASS(const QByteArray &, class LibASS *, double) override;

    /**/

    bool Use_mDVD_FPS;
    double Sub_max_s;
};

#define ClassicSubsName "Classic Subtitles"
