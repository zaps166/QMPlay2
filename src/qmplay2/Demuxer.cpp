#include <Demuxer.hpp>

#include <QMPlay2Core.hpp>
#include <Functions.hpp>
#include <Module.hpp>

bool Demuxer::create( const QString &url, IOController< Demuxer > &demuxer, Playlist::Entries *tracks, bool onlyTracks )
{
	const QString scheme = Functions::getUrlScheme( url );
	if ( demuxer.isAborted() || url.isEmpty() || scheme.isEmpty() )
		return false;
	QString extension = Functions::fileExt( url ).toLower();
	for ( int i = 0 ; i <= 1 ; ++i )
		foreach ( Module *module, QMPlay2Core.getPluginsInstance() )
			foreach ( const Module::Info &mod, module->getModulesInfo() )
				if ( mod.type == Module::DEMUXER && ( mod.name == scheme || ( !i ? mod.extensions.contains( extension ) : mod.extensions.isEmpty() ) ) )
				{
					if ( !demuxer.assign( ( Demuxer * )module->createInstance( mod.name ) ) )
						continue;
					if ( tracks )
					{
						*tracks = demuxer->fetchTracks( url );
						if ( !tracks->isEmpty() )
						{
							demuxer.clear();
							return true;
						}
					}
					if ( !onlyTracks && demuxer->open( url ) )
						return true;
					demuxer.clear();
					if ( mod.name == scheme || demuxer.isAborted() )
						return false;
				}
	return false;
}
