#include <Module.hpp>
#include <ModuleCommon.hpp>

void Module::SetInstances( bool &restartPlaying )
{
	mutex.lock();
	foreach ( ModuleCommon *mc, instances )
		if ( !mc->set() )
			restartPlaying = true;
	mutex.unlock();
}
