// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Lib.hpp>

#include <QHash>
#include <QString>
#include <QVariant>

class QMPLAY2SHAREDLIB_EXPORT ModuleParams
{
    Q_DISABLE_COPY(ModuleParams)

public:
    bool modParam(const QString &key, const QVariant &val);
    inline QVariant getParam(const QString &key) const
    {
        return paramList.value(key);
    }
    inline bool hasParam(const QString &key) const
    {
        return paramList.contains(key);
    }

    virtual bool processParams(bool *paramsCorrected = nullptr);

protected:
    ModuleParams() = default;
    virtual ~ModuleParams() = default;

    inline void addParam(const QString &key, const QVariant &val = QVariant())
    {
        paramList.insert(key, val);
    }
    inline void removeParam(const QString &key)
    {
        paramList.remove(key);
    }

private:
    QHash<QString, QVariant> paramList;
};
