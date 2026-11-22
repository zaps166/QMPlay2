// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <ModuleCommon.hpp>
#include <ModuleParams.hpp>
#include <IOController.hpp>

#include <QStringList>

class QMPLAY2SHAREDLIB_EXPORT Writer : public ModuleCommon, public ModuleParams, public BasicIO
{
public:
    static Writer *create(const QString &url, const QStringList &modNames = {});

    inline QString getUrl() const
    {
        return m_url;
    }

    virtual bool readyWrite() const = 0;

    virtual qint64 write(const QByteArray &) = 0;

    virtual QString name() const = 0;

private:
    virtual bool open() = 0;

private:
    QString m_url;
};
