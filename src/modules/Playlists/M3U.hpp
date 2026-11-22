// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Playlist.hpp>

class M3U final : public Playlist
{
    Entries read() override;
    bool write(const Entries &) override;
};

#define M3UName "M3U"
