// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <OtherVFiltersW.hpp>
#include <Settings.hpp>
#include <Module.hpp>
#include <Main.hpp>

OtherVFiltersW::OtherVFiltersW(bool hw) :
    hw(hw)
{
    QList<QPair<Module *, Module::Info>> pluginsInstances;

    setIconSize({32, 32});

    if (!hw)
    {
        setSelectionMode(QAbstractItemView::ExtendedSelection);
        setDragDropMode(QAbstractItemView::InternalMove);

        QPair<QStringList, QList<bool>> videoFilters;
        for (const QString &filter : QMPlay2Core.getSettings().getStringList("VideoFilters"))
        {
            videoFilters.first += filter.mid(1);
            videoFilters.second += filter.left(1).toInt();
        }

        for (int i = 0; i < videoFilters.first.count(); ++i)
            pluginsInstances += QPair<Module *, Module::Info>();

        for (Module *pluginInstance : QMPlay2Core.getPluginsInstance())
            for (Module::Info moduleInfo : pluginInstance->getModulesInfo())
                if ((moduleInfo.type & 0xF) == Module::VIDEOFILTER && !(moduleInfo.type & Module::DEINTERLACE))
                {
                    const int idx = videoFilters.first.indexOf(moduleInfo.name);
                    if (idx > -1)
                    {
                        if (videoFilters.second[idx])
                            moduleInfo.type |= Module::USERFLAG;
                        pluginsInstances[idx] = {pluginInstance, moduleInfo};
                    }
                    else
                        pluginsInstances += {pluginInstance, moduleInfo};
                }
    }
    else
    {
        for (Module *pluginInstance : QMPlay2Core.getPluginsInstance())
            for (const Module::Info &moduleInfo : pluginInstance->getModulesInfo())
                if ((moduleInfo.type & 0xF) == Module::WRITER && (moduleInfo.type & Module::VIDEOHWFILTER))
                    pluginsInstances += {pluginInstance, moduleInfo};
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
        item->setIcon(QMPlay2GUI.getIcon(moduleInfo.icon.isNull() ? module->icon() : moduleInfo.icon));
        if (!hw)
            item->setCheckState(moduleInfo.type & Module::USERFLAG ? Qt::Checked : Qt::Unchecked);
        item->setText(moduleInfo.name);
    }
}

void OtherVFiltersW::writeSettings()
{
    if (!hw)
    {
        QStringList filters;
        for (int i = 0; i < count(); ++i)
            filters += (item(i)->checkState() == Qt::Checked ? '1' : '0') + item(i)->text();
        QMPlay2Core.getSettings().set("VideoFilters", filters);
    }
}
