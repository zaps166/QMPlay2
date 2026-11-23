// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <SubtitlesHeaders.hpp>

#include <SubsDec.hpp>

class SRT final : public SubsDec
{
    bool toASS(const QByteArray &, class LibASS *, double) override;
};

#define SRTSubsName "SRT Subtitles"
