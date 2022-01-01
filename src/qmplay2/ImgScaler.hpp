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

#include <QMPlay2Lib.hpp>

/* YUV planar to RGB32 */

struct SwsContext;
class Frame;

class QMPLAY2SHAREDLIB_EXPORT ImgScaler
{
public:
    ImgScaler();
    inline ~ImgScaler()
    {
        destroy();
    }

    bool create(const Frame &videoFrame, int newWdst = -1, int newHdst = -1);
    bool scale(const Frame &videoFrame, void *dst = nullptr);
    void scale(const void *src[], const int srcLinesize[], void *dst);
    void destroy();
private:
    SwsContext *m_swsCtx;
    int m_srcH, m_dstLinesize;
};
