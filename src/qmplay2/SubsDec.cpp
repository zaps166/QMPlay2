#include <SubsDec.hpp>
#include <QMPlay2Core.hpp>
#include <Module.hpp>

SubsDec *SubsDec::create(const QString &type)
{
	if (type.isEmpty())
		return NULL;
	foreach (Module *module, QMPlay2Core.getPluginsInstance())
		foreach (const Module::Info &mod, module->getModulesInfo())
			if (mod.type == Module::SUBSDEC && mod.extensions.contains(type))
			{
				SubsDec *subsdec = (SubsDec *)module->createInstance(mod.name);
				if (!subsdec)
					continue;
				return subsdec;
			}
	return NULL;
}
QStringList SubsDec::extensions()
{
	QStringList extensions;
	foreach (Module *module, QMPlay2Core.getPluginsInstance())
		foreach (const Module::Info &mod, module->getModulesInfo())
			if (mod.type == Module::SUBSDEC)
				extensions << mod.extensions;
	return extensions;
}

SubsDec::~SubsDec()
{}
