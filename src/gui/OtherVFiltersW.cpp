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

#include <OtherVFiltersW.hpp>
#include <Settings.hpp>
#include <Module.hpp>
#include <Main.hpp>

OtherVFiltersW::OtherVFiltersW()
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setDragDropMode(QAbstractItemView::InternalMove);

	QPair<QStringList, QList<bool> > videoFilters;
	foreach (const QString &filter, QMPlay2Core.getSettings().get("VideoFilters").toStringList())
	{
		videoFilters.first += filter.mid(1);
		videoFilters.second += filter.left(1).toInt();
	}

	QList<QPair<Module *, Module::Info> > pluginsInstances;
	for (int i = 0; i < videoFilters.first.count(); ++i)
		pluginsInstances += QPair<Module *, Module::Info>();

	foreach (Module *pluginInstance, QMPlay2Core.getPluginsInstance())
		foreach (Module::Info moduleInfo, pluginInstance->getModulesInfo())
			if ((moduleInfo.type & 0xF) == Module::VIDEOFILTER && !(moduleInfo.type & Module::DEINTERLACE))
			{
				const int idx = videoFilters.first.indexOf(moduleInfo.name);
				if (idx > -1)
				{
					if (videoFilters.second[idx])
						moduleInfo.type |= Module::USERFLAG;
					pluginsInstances[idx] = qMakePair(pluginInstance, moduleInfo);
				}
				else
					pluginsInstances += qMakePair(pluginInstance, moduleInfo);
			}

	for (int i = 0; i < pluginsInstances.count(); i++)
	{
		Module *module = pluginsInstances[i].first;
		Module::Info &moduleInfo = pluginsInstances[i].second;
		if (!module || moduleInfo.name.isEmpty())
			continue;
		QListWidgetItem *item = new QListWidgetItem(this);
		item->setData(Qt::UserRole, module->name());
		item->setData(Qt::ToolTipRole, moduleInfo.description);
		item->setIcon(QMPlay2GUI.getIcon(moduleInfo.img.isNull() ? module->image() : moduleInfo.img));
		item->setCheckState(moduleInfo.type & Module::USERFLAG ? Qt::Checked : Qt::Unchecked);
		item->setText(moduleInfo.name);
	}
}

void OtherVFiltersW::writeSettings()
{
	QStringList filters;
	for (int i = 0; i < count(); ++i)
		filters += (item(i)->checkState() == Qt::Checked ? '1' : '0') + item(i)->text();
	QMPlay2Core.getSettings().set("VideoFilters", filters);
}
