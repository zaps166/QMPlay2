// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <ModuleCommon.hpp>
#include <ModuleParams.hpp>
#include <IOController.hpp>

class QMPLAY2SHAREDLIB_EXPORT Reader : protected ModuleCommon, public ModuleParams, public BasicIO
{
public:
    static bool create(const QString &url, IOController<Reader> &reader, const QString &plugName = QString());

    inline QString getUrl() const
    {
        return _url;
    }

    virtual bool readyRead() const = 0;
    virtual bool canSeek() const = 0;

    virtual bool seek(qint64) = 0;
    virtual QByteArray read(qint64) = 0;
    virtual bool atEnd() const = 0;

    virtual qint64 size() const = 0;
    virtual qint64 pos() const = 0;
    virtual QString name() const = 0;

private:
    virtual bool open() = 0;

    QString _url;
};
