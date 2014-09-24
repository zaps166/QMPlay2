#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QMPlay2Core.hpp>
#include <QSettings>

class Settings
{
public:
	inline Settings( const QString &name ) :
		settings( QMPlay2Core.getSettingsDir() + name + ".ini", QSettings::IniFormat ) {}

	inline bool contains( const QString &key ) const
	{
		return settings.contains( key );
	}

	inline void init( const QString &key, const QVariant &val )
	{
		if ( !contains( key ) )
			settings.setValue( key, val );
	}

	inline void set( const QString &key, const QVariant &val )
	{
		settings.setValue( key, val );
	}

	inline QVariant get( const QString &key, const QVariant &def = QVariant() ) const
	{
		return settings.value( key, def );
	}
	inline bool getBool( const QString &key, const bool def = bool() ) const
	{
		return settings.value( key, def ).toBool();
	}
	inline int getInt( const QString &key, const int def = int() ) const
	{
		return settings.value( key, def ).toInt();
	}
	inline uint getUInt( const QString &key, const uint def = uint() ) const
	{
		return settings.value( key, def ).toUInt();
	}
	inline double getDouble( const QString &key, const double def = double() ) const
	{
		return settings.value( key, def ).toDouble();
	}
	inline QByteArray getByteArray( const QString &key, const QByteArray &def = QByteArray() ) const
	{
		return settings.value( key, def ).toByteArray();
	}
	inline QString getString( const QString &key, const QString &def = QString() ) const
	{
		return settings.value( key, def ).toString();
	}

	inline void remove( const QString &key )
	{
		settings.remove( key );
	}
private:
	QSettings settings;
};

#endif
