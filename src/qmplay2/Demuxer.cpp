#include <Demuxer.hpp>

#include <QMPlay2Core.hpp>
#include <Functions.hpp>
#include <Module.hpp>

bool Demuxer::create( const QString &url, Demuxer *&demuxer, const bool &br, QMutex *demuxerMutex )
{
	QString scheme = Functions::getUrlScheme( url );
	if ( br || url.isEmpty() || scheme.isEmpty() )
		return false;
	QString extension = Functions::fileExt( url ).toLower();
	for ( int i = 0 ; i <= 1 ; ++i )
		foreach ( Module *module, QMPlay2Core.getPluginsInstance() )
			foreach ( Module::Info mod, module->getModulesInfo() )
				if ( mod.type == Module::DEMUXER && ( mod.name == scheme || ( !i ? mod.extensions.contains( extension ) : mod.extensions.isEmpty() ) ) )
				{
					if ( !( demuxer = ( Demuxer * )module->createInstance( mod.name ) ) )
						continue;
					if ( demuxer->open( url ) )
						return true;
					Functions::deleteThreadSafe( demuxer, demuxerMutex );
					if ( mod.name == scheme || br )
						return false;
				}
	return false;
}
