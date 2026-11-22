// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
