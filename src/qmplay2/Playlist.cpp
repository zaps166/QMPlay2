#include <Playlist.hpp>

#include <QMPlay2Core.hpp>
#include <Functions.hpp>
#include <Module.hpp>
#include <Writer.hpp>
#include <Reader.hpp>

Playlist::Entries Playlist::read( const QString &url, QString *name )
{
	Entries list;
	Playlist *playlist = create( url, ReadOnly, name );
	if ( playlist )
	{
		list = playlist->_read();
		delete playlist;
	}
	return list;
}
bool Playlist::write( const Entries &list, const QString &url, QString *name )
{
	bool OK = false;
	Playlist *playlist = create( url, WriteOnly, name );
	if ( playlist )
	{
		OK = playlist->_write( list );
		delete playlist;
	}
	return OK;
}
QString Playlist::name( const QString &url )
{
	QString name;
	create( url, NoOpen, &name );
	return name;
}
QStringList Playlist::extensions()
{
	QStringList extensions;
	foreach ( Module *module, QMPlay2Core.getPluginsInstance() )
		foreach ( const Module::Info &mod, module->getModulesInfo() )
			if ( mod.type == Module::PLAYLIST )
				extensions += mod.extensions;
	return extensions;
}

Playlist *Playlist::create( const QString &url, OpenMode openMode, QString *name )
{
	QString extension = Functions::fileExt( url ).toLower();
	if ( extension.isEmpty() )
		return NULL;
	foreach ( Module *module, QMPlay2Core.getPluginsInstance() )
		foreach ( const Module::Info &mod, module->getModulesInfo() )
			if ( mod.type == Module::PLAYLIST && mod.extensions.contains( extension ) )
			{
				if ( openMode == NoOpen )
				{
					if ( name )
						*name = mod.name;
					return NULL;
				}
				Playlist *playlist = ( Playlist * )module->createInstance( mod.name );
				if ( !playlist )
					continue;
				switch ( openMode )
				{
					case ReadOnly:
					{
						IOController< Reader > &reader = playlist->ioCtrl.toRef< Reader >();
						Reader::create( url, reader ); //TODO przerywanie (po co?)
						if ( reader && reader->size() <= 0 )
							reader.clear();
					} break;
					case WriteOnly:
						playlist->ioCtrl.assign( Writer::create( url ) );
						break;
					default:
						break;
				}
				if ( playlist->ioCtrl )
				{
					if ( name )
						*name = mod.name;
					return playlist;
				}
				delete playlist;
			}
	return NULL;
}

QList< QByteArray > Playlist::readLines()
{
	IOController< Reader > &reader = ioCtrl.toRef< Reader >();
	return reader->read( reader->size() ).replace( '\r', QByteArray() ).split( '\n' );
}
