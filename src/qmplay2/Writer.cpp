#include <Writer.hpp>

#include <QMPlay2Core.hpp>
#include <Functions.hpp>
#include <Module.hpp>

Writer *Writer::create( const QString &url, const QStringList &modNames )
{
	QString scheme = Functions::getUrlScheme( url );
	if ( url.isEmpty() || scheme.isEmpty() )
		return NULL;
	QVector< QPair< Module *, Module::Info > > pluginsInstances( modNames.count() );
	foreach ( Module *pluginInstance, QMPlay2Core.getPluginsInstance() )
		foreach ( Module::Info mod, pluginInstance->getModulesInfo() )
			if ( mod.type == Module::WRITER && mod.extensions.contains( scheme ) )
			{
				if ( modNames.isEmpty() )
					pluginsInstances += qMakePair( pluginInstance, mod );
				else
				{
					const int idx = modNames.indexOf( mod.name );
					if ( idx > -1 )
						pluginsInstances[ idx ] = qMakePair( pluginInstance, mod );
				}
			}
	for ( int i = 0 ; i < pluginsInstances.count() ; i++ )
	{
		Module *module = pluginsInstances[ i ].first;
		Module::Info &moduleInfo = pluginsInstances[ i ].second;
		if ( !module || moduleInfo.name.isEmpty() )
			continue;
		Writer *writer = ( Writer * )module->createInstance( moduleInfo.name );
		if ( !writer )
			continue;
		writer->_url = url;
		if ( writer->open() )
			return writer;
		delete writer;
	}
	return NULL;
}
