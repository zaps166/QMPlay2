#include <Settings.hpp>

Settings::Settings(const QString &name) :
	QSettings( QMPlay2Core.getSettingsDir() + name + ".ini", QSettings::IniFormat ),
	timerID( 0 )
{}
Settings::~Settings()
{
	QMutexLocker mL( &mutex );
	flushCache();
}

void Settings::init( const QString &key, const QVariant &val )
{
	QMutexLocker mL( &mutex );
	if ( !cache.isEmpty() )
		flushCache();
	if ( !QSettings::contains( key ) )
		QSettings::setValue( key, val );
}
bool Settings::contains( const QString &key ) const
{
	QMutexLocker mL( &mutex );
	if ( !cache.isEmpty() && cache.contains( key ) )
		return true;
	return QSettings::contains( key );
}
void Settings::set( const QString &key, const QVariant &val )
{
	QMutexLocker mL( &mutex );
	cache[ key ] = val;
	if ( timerID )
		killTimer( timerID );
	timerID = startTimer( 2000 );
}
void Settings::remove( const QString &key )
{
	QMutexLocker mL( &mutex );
	if ( !cache.isEmpty() )
		flushCache();
	QSettings::remove( key );
}

QVariant Settings::get( const QString &key, const QVariant &def ) const
{
	QMutexLocker mL( &mutex );
	if ( !cache.isEmpty() && cache.contains( key ) )
		return cache[ key ];
	return QSettings::value( key, def );
}

void Settings::timerEvent( QTimerEvent * )
{
	QMutexLocker mL( &mutex );
	if ( timerID )
		flushCache();
}
void Settings::flushCache()
{
	if ( timerID )
	{
		killTimer( timerID );
		timerID = 0;
	}
	for ( SettingsMap::const_iterator it = cache.begin(), end_it = cache.end() ; it != end_it ; ++it )
		QSettings::setValue( it.key(), it.value() );
	cache.clear();
}
