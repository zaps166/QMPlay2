#include <Writer.hpp>

#include <QMPlay2Core.hpp>
#include <Functions.hpp>
#include <Module.hpp>

#include <QFile>

class QMPlay2FileWriter : public Writer
{
	bool readyWrite() const
	{
		return f.isOpen();
	}

	qint64 write( const QByteArray &arr )
	{
		return f.write( arr );
	}

	qint64 size() const
	{
		return f.size();
	}
	QString name() const
	{
		return "File Writer";
	}

	bool open()
	{
		f.setFileName( getUrl().mid( 7 ) );
		return f.open( QIODevice::WriteOnly );
	}

	/**/

	QFile f;
};

Writer *Writer::create( const QString &url, const QStringList &modNames )
{
	const QString scheme = Functions::getUrlScheme( url );
	if ( url.isEmpty() || scheme.isEmpty() )
		return NULL;
	Writer *writer = NULL;
	if ( modNames.isEmpty() && scheme == "file" )
	{
		writer = new QMPlay2FileWriter;
		writer->_url = url;
		if ( writer->open() )
			return writer;
		delete writer;
		return NULL;
	}
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
		writer = ( Writer * )module->createInstance( moduleInfo.name );
		if ( !writer )
			continue;
		writer->_url = url;
		if ( writer->open() )
			return writer;
		delete writer;
	}
	return NULL;
}
