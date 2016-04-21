#include <Reader.hpp>

#include <QMPlay2Core.hpp>
#include <Functions.hpp>
#include <Module.hpp>

bool Reader::create(const QString &url, IOController<Reader> &reader, const QString &plugName)
{
	QString scheme = Functions::getUrlScheme(url);
	if (reader.isAborted() || url.isEmpty() || scheme.isEmpty())
		return false;
	foreach (Module *module, QMPlay2Core.getPluginsInstance())
		foreach (const Module::Info &mod, module->getModulesInfo())
			if (mod.type == Module::READER && mod.extensions.contains(scheme) && (plugName.isEmpty() || mod.name == plugName))
			{
				if (reader.assign((Reader *)module->createInstance(mod.name)))
				{
					reader->_url = url;
					if (reader->open())
						return true;
					reader.clear();
				}
				if (reader.isAborted())
					break;
			}
	return false;
}
