/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

Reader::~Reader()
{}
