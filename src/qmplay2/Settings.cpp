// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <Settings.hpp>

Settings::Settings(const QString &name) :
    QSettings(QMPlay2Core.getSettingsDir() + QMPlay2Core.getSettingsProfile() + name + ".ini", QSettings::IniFormat)
{}
Settings::~Settings()
{
    QMutexLocker mL(&mutex);
    flushCache();
}

void Settings::flush()
{
    QMutexLocker mL(&mutex);
    flushCache();
    sync();
}

void Settings::init(const QString &key, const std::function<QVariant()> &valFn)
{
    Q_ASSERT(valFn);
    QMutexLocker mL(&mutex);
    const auto it = toRemove.find(key);
    const bool foundInToRemove = (it != toRemove.end());
    if (!cache.contains(key) && (foundInToRemove || !QSettings::contains(key)))
        cache[key] = valFn();
    if (foundInToRemove)
        toRemove.erase(it);
}
void Settings::init(const QString &key, const QVariant &val)
{
    init(key, [&] {
        return val;
    });
}
bool Settings::contains(const QString &key, bool checkGroups) const
{
    QMutexLocker mL(&mutex);
    if (cache.contains(key))
        return true;
    if (toRemove.contains(key))
        return false;
    if (QSettings::contains(key))
        return true;
    if (checkGroups)
        return QSettings::childGroups().contains(key);
    return false;
}
void Settings::set(const QString &key, const QVariant &val)
{
    QMutexLocker mL(&mutex);
    toRemove.remove(key);
    cache[key] = val;
}
void Settings::remove(const QString &key)
{
    QMutexLocker mL(&mutex);
    toRemove.insert(key);
    cache.remove(key);
}

QVariant Settings::get(const QString &key, const QVariant &def) const
{
    QMutexLocker mL(&mutex);
    auto it = cache.find(key);
    if (it != cache.end())
        return it.value();
    if (toRemove.contains(key))
        return def;
    return QSettings::value(key, def);
}

void Settings::flushCache()
{
    for (const QString &key : std::as_const(toRemove))
        QSettings::remove(key);
    toRemove.clear();

    for (auto it = cache.constBegin(), end_it = cache.constEnd(); it != end_it; ++it)
        QSettings::setValue(it.key(), it.value());
    cache.clear();
}
