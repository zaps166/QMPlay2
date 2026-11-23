// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <VideoFiltersHeaders.hpp>

#include <VideoFilter.hpp>

class BobDeint final : public VideoFilter
{
public:
    BobDeint();

    bool filter(QQueue<Frame> &framesQueue) override;

    bool processParams(bool *paramsCorrected) override;
};

#define BobDeintName "Bob"
