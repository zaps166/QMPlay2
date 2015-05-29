#include <QMPlay2Extensions.hpp>

#include <QMPlay2Core.hpp>
#include <Module.hpp>

QList< QMPlay2Extensions * > QMPlay2Extensions::guiExtensionsList;

void QMPlay2Extensions::openExtensions()
{
	if ( !guiExtensionsList.isEmpty() )
		return;
	foreach ( Module *module, QMPlay2Core.getPluginsInstance() )
		foreach ( const Module::Info &mod, module->getModulesInfo() )
			if ( mod.type == Module::QMPLAY2EXTENSION )
			{
				QMPlay2Extensions *QMPlay2Ext = ( QMPlay2Extensions * )module->createInstance( mod.name );
				if ( QMPlay2Ext )
					guiExtensionsList.append( QMPlay2Ext );
			}
	foreach ( QMPlay2Extensions *QMPlay2Ext, guiExtensionsList )
		QMPlay2Ext->init();
}

void QMPlay2Extensions::closeExtensions()
{
	while ( guiExtensionsList.size() )
		delete guiExtensionsList.takeFirst();
}
