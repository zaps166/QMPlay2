// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <PlaylistsHeaders.hpp>

#include <Playlist.hpp>

class XSPF final : public Playlist
{
    Entries read() override;
    bool write(const Entries &) override;
};

#define XSPFName "XSPF"
