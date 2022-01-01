/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

constexpr float verticesYCbCr[8][8] = {
    /* Normal */
    {
        -1.0f, -1.0f, //0. Left-bottom
        +1.0f, -1.0f, //1. Right-bottom
        -1.0f, +1.0f, //2. Left-top
        +1.0f, +1.0f, //3. Right-top
    },
    /* Horizontal flip */
    {
        +1.0f, -1.0f, //1. Right-bottom
        -1.0f, -1.0f, //0. Left-bottom
        +1.0f, +1.0f, //3. Right-top
        -1.0f, +1.0f, //2. Left-top
    },
    /* Vertical flip */
    {
        -1.0f, +1.0f, //2. Left-top
        +1.0f, +1.0f, //3. Right-top
        -1.0f, -1.0f, //0. Left-bottom
        +1.0f, -1.0f, //1. Right-bottom
    },
    /* Rotated 180 */
    {
        +1.0f, +1.0f, //3. Right-top
        -1.0f, +1.0f, //2. Left-top
        +1.0f, -1.0f, //1. Right-bottom
        -1.0f, -1.0f, //0. Left-bottom
    },

    /* Rotated 90 */
    {
        -1.0f, +1.0f, //2. Left-top
        -1.0f, -1.0f, //0. Left-bottom
        +1.0f, +1.0f, //3. Right-top
        +1.0f, -1.0f, //1. Right-bottom
    },
    /* Rotated 90 + horizontal flip */
    {
        +1.0f, +1.0f, //3. Right-top
        +1.0f, -1.0f, //1. Right-bottom
        -1.0f, +1.0f, //2. Left-top
        -1.0f, -1.0f, //0. Left-bottom
    },
    /* Rotated 90 + vertical flip */
    {
        -1.0f, -1.0f, //0. Left-bottom
        -1.0f, +1.0f, //2. Left-top
        +1.0f, -1.0f, //1. Right-bottom
        +1.0f, +1.0f, //3. Right-top
    },
    /* Rotated 270 */
    {
        +1.0f, -1.0f, //1. Right-bottom
        +1.0f, +1.0f, //3. Right-top
        -1.0f, -1.0f, //0. Left-bottom
        -1.0f, +1.0f, //2. Left-top
    },
};
constexpr float texCoordOSD[8] = {
    0.0f, 1.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
};
