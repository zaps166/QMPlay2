#include <Decoder.hpp>

#include <QMPlay2Core.hpp>
#include <StreamInfo.hpp>
#include <Module.hpp>

class QMPlay2DummyDecoder : public Decoder
{
	QString name() const
	{
		return QString();
	}

	int decode( Packet &packet, QByteArray &dest, bool flush, unsigned hurry_up )
	{
		Q_UNUSED( flush )
		Q_UNUSED( hurry_up )
		return ( dest = packet ).size();
	}

	bool open( StreamInfo *_streamInfo, Writer * )
	{
		streamInfo = _streamInfo;
		return true;
	}
};

Decoder *Decoder::create( StreamInfo *streamInfo, Writer *writer, const QStringList &modNames )
{
	if ( !streamInfo->subs_to_decode )
	{
		Decoder *decoder = new QMPlay2DummyDecoder;
		decoder->open( streamInfo );
		return decoder;
	}
	QVector< QPair< Module *, Module::Info > > pluginsInstances( modNames.count() );
	foreach ( Module *pluginInstance, QMPlay2Core.getPluginsInstance() )
		foreach ( Module::Info mod, pluginInstance->getModulesInfo() )
			if ( mod.type == Module::DECODER )
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
		Decoder *decoder = ( Decoder * )module->createInstance( moduleInfo.name );
		if ( !decoder )
			continue;
		if ( decoder->open( streamInfo, writer ) )
			return decoder;
		delete decoder;
	}
	return NULL;
}
