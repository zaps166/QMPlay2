// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QString>
#include <QVector>

extern "C" {
    #include <libavutil/avutil.h>
}

class ChapterInfo
{
public:
    inline ChapterInfo(double start, double end) :
        start(start), end(end)
    {}

    QString title;
    double start, end;
};

class ProgramInfo
{
public:
    inline ProgramInfo(int number) :
        number(number)
    {}

    int number;
    QVector<QPair<int, AVMediaType>> streams;
    int64_t bitrate = 0;
};
