/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

#pragma once

#include <QAbstractTableModel>
#include <QCoreApplication>

class QAction;

class ShortcutHandler : public QAbstractTableModel
{
	Q_DECLARE_TR_FUNCTIONS(ShortcutHandler)

public:
	ShortcutHandler(QObject *parent);
	~ShortcutHandler() final;

	int columnCount(const QModelIndex &parent) const override final;
	int rowCount(const QModelIndex &parent) const override final;

	Qt::ItemFlags flags(const QModelIndex &index) const override final;

	QVariant headerData(int section, Qt::Orientation orientation, int role) const override final;

	QVariant data(const QModelIndex &index, int role) const override final;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override final;

	void appendAction(QAction *action, const QString &settingsName, const QString &defaultShortcut);

	void save();
	void restore();
	void reset();

private:
	QList<QAction *> m_actions;
	QHash<QAction *, QPair<QString, QString>> m_shortcuts;
};
