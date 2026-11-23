// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Core.hpp>

class QMPLAY2SHAREDLIB_EXPORT Settings : protected QSettings
{
    using SettingsMap = QMap<QString, QVariant>;
public:
    Settings(const QString &name);
    ~Settings();

    void flush();

    void init(const QString &key, const std::function<QVariant()> &valFn);
    void init(const QString &key, const QVariant &val);
    void set(const QString &key, const QVariant &val);
    bool contains(const QString &key, bool checkGroups = false) const;
    void remove(const QString &key);

    template<typename T>
    inline T getWithBounds(const QString &key, T min, T max, T def = T()) const
    {
        return qBound(min, (T)get(key, def).toInt(), max);
    }

    inline bool getBool(const QString &key, const bool def = bool()) const
    {
        return get(key, def).toBool();
    }
    inline int getInt(const QString &key, const int def = int()) const
    {
        return get(key, def).toInt();
    }
    inline uint getUInt(const QString &key, const uint def = uint()) const
    {
        return get(key, def).toUInt();
    }
    inline double getDouble(const QString &key, const double def = double()) const
    {
        return get(key, def).toDouble();
    }
    inline QByteArray getByteArray(const QString &key, const QByteArray &def = QByteArray()) const
    {
        return get(key, def).toByteArray();
    }
    inline QString getString(const QString &key, const QString &def = QString()) const
    {
        return get(key, def).toString();
    }
    inline QStringList getStringList(const QString &key, const QStringList &def = {}) const
    {
        return get(key, def).toStringList();
    }
    inline QRect getRect(const QString &key, const QRect &def = QRect()) const
    {
        return get(key, def).toRect();
    }
    inline QColor getColor(const QString &key, const QColor &def = QColor()) const
    {
        return get(key, def).value<QColor>();
    }
private:
    QVariant get(const QString &key, const QVariant &def = QVariant()) const;

    void flushCache();

    mutable QMutex mutex;
    QSet<QString> toRemove;
    SettingsMap cache;
};
