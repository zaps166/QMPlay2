// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <VideoFilter.hpp>

#include <QThreadPool>

class YadifDeint final : public VideoFilter
{
public:
    YadifDeint(bool m_doubler, bool m_spatialCheck);

    bool filter(QQueue<Frame> &framesQueue) override;

    bool processParams(bool *paramsCorrected) override;
private:
    const bool m_doubler;
    const bool m_spatialCheck;
    QThreadPool m_threadsPool;
};

#define YadifDeintName "Yadif"
#define YadifNoSpatialDeintName YadifDeintName " (no spatial check)"
#define Yadif2xDeintName YadifDeintName " 2x"
#define Yadif2xNoSpatialDeintName Yadif2xDeintName " (no spatial check)"

#define YadifDescr "Yet Another DeInterlacong Filter"
