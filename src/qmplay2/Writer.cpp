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

	qint64 write(const QByteArray &arr)
	{
		return f.write(arr);
	}

	QString name() const
	{
		return "File Writer";
	}

	bool open()
	{
		f.setFileName(getUrl().mid(7));
		return f.open(QIODevice::WriteOnly);
	}

	/**/

	QFile f;
};

Writer *Writer::create(const QString &url, const QStringList &modNames)
{
	const QString scheme = Functions::getUrlScheme(url);
	if (url.isEmpty() || scheme.isEmpty())
		return NULL;
	Writer *writer = NULL;
	if (modNames.isEmpty() && scheme == "file")
	{
		writer = new QMPlay2FileWriter;
		writer->_url = url;
		if (writer->open())
			return writer;
		delete writer;
		return NULL;
	}

	typedef QVector<QPair<Module *, Module::Info> > pluginsList_t;
	pluginsList_t pluginsInstances(modNames.count());
	foreach (Module *pluginInstance, QMPlay2Core.getPluginsInstance())
		foreach (const Module::Info &mod, pluginInstance->getModulesInfo())
			if (mod.type == Module::WRITER && mod.extensions.contains(scheme))
			{
				if (modNames.isEmpty())
					pluginsInstances += qMakePair(pluginInstance, mod);
				else
				{
					const int idx = modNames.indexOf(mod.name);
					if (idx > -1)
						pluginsInstances[idx] = qMakePair(pluginInstance, mod);
				}
			}
	for(pluginsList_t::const_iterator it = pluginsInstances.constBegin(), end = pluginsInstances.constEnd();
		it != end; ++it)
	{
		Module *module = it->first;
		const Module::Info &moduleInfo = it->second;
		if (!module || moduleInfo.name.isEmpty())
			continue;
		writer = (Writer *)module->createInstance(moduleInfo.name);
		if (!writer)
			continue;
		writer->_url = url;
		if (writer->open())
			return writer;
		delete writer;
	}
	return NULL;
}

Writer::~Writer()
{}
