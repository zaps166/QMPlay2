// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Lib.hpp>

#include <QStringList>

class LibASS;

class QMPLAY2SHAREDLIB_EXPORT SubsDec
{
public:
    static SubsDec *create(const QString &);
    static QStringList extensions();

    virtual ~SubsDec() = default;

    virtual bool toASS(const QByteArray &, LibASS *, double) = 0;
};
