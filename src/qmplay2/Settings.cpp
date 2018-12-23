/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Settings.hpp>

#include <CppUtils.hpp>

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

void Settings::init(const QString &key, const QVariant &val)
{
	QMutexLocker mL(&mutex);
	if (!cache.contains(key) && !QSettings::contains(key))
		cache[key] = val;
	toRemove.remove(key);
}
bool Settings::contains(const QString &key) const
{
	QMutexLocker mL(&mutex);
	if (cache.contains(key))
		return true;
	if (toRemove.contains(key))
		return false;
	return QSettings::contains(key);
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
	for (const QString &key : asConst(toRemove))
		QSettings::remove(key);
	toRemove.clear();

	for (auto it = cache.constBegin(), end_it = cache.constEnd(); it != end_it; ++it)
		QSettings::setValue(it.key(), it.value());
	cache.clear();
}
