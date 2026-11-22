// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <VideoFilter.hpp>
#include <QMPlay2Lib.hpp>

class QMPLAY2SHAREDLIB_EXPORT DeintHWPrepareFilter : public VideoFilter
{
public:
    DeintHWPrepareFilter();
    ~DeintHWPrepareFilter();

    bool filter(QQueue<Frame> &framesQueue) override;

    bool processParams(bool *paramsCorrected) override;

private:
    bool m_deinterlace = false;
};
