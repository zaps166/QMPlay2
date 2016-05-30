#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QMPlay2Core.hpp>

#include <QSettings>
#include <QMutex>
#include <QMap>
#include <QSet>

class Settings : protected QSettings
{
	typedef QMap<QString, QVariant> SettingsMap;
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
	QVariant get(const QString &key, const QVariant &def = QVariant()) const;
private:
	void flushCache();

	mutable QMutex mutex;
	QSet<QString> toRemove;
	SettingsMap cache;
};

#endif
