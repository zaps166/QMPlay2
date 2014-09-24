#include <AudioFilter.hpp>

#include <QMPlay2Core.hpp>
#include <Module.hpp>

QVector< AudioFilter * > AudioFilter::open()
{
	QVector< AudioFilter * > filterList;
	foreach ( Module *module, QMPlay2Core.getPluginsInstance() )
		foreach ( Module::Info mod, module->getModulesInfo() )
			if ( mod.type == Module::AUDIOFILTER )
			{
				AudioFilter *filter = ( AudioFilter * )module->createInstance( mod.name );
				if ( filter )
					filterList.append( filter );
			}
	filterList.squeeze();
	return filterList;
}
