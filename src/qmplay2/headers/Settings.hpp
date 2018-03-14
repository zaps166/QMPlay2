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

#pragma once

#include <QMPlay2Core.hpp>

#include <QSettings>
#include <QMutex>
#include <QMap>
#include <QSet>

class QMPLAY2SHAREDLIB_EXPORT Settings : protected QSettings
{
	using SettingsMap = QMap<QString, QVariant>;
public:
	Settings(const QString &name);
	~Settings();

	void flush();

	void init(const QString &key, const QVariant &val);
	void set(const QString &key, const QVariant &val);
	bool contains(const QString &key) const;
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
