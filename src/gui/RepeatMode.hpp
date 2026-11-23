// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <GUIHeaders.hpp>

enum RepeatMode
{
    RepeatNormal,

    RepeatEntry,
    RepeatGroup,
    RepeatList,

    RandomMode,
    RandomGroupMode,

    RepeatRandom,
    RepeatRandomGroup,

    RepeatStopAfter,

    //Number of items in enum
    RepeatModeCount
};
