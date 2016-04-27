#include <Module.hpp>
#include <ModuleCommon.hpp>

Module::~Module()
{}

QList<QAction *> Module::getAddActions()
{
	return QList<QAction *>();
}

Module::SettingsWidget *Module::getSettingsWidget()
{
	return NULL;
}

void Module::setInstances(bool &restartPlaying)
{
	QMutexLocker locker(&mutex);
	foreach (ModuleCommon *mc, instances)
		if (!mc->set())
			restartPlaying = true;
}
