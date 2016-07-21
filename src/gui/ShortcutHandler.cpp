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

#include <ShortcutHandler.hpp>
#include <QMPlay2Core.hpp>
#include <Settings.hpp>

#include <QApplication>

ShortcutHandler *ShortcutHandler::s_instance = NULL;

ShortcutHandler *ShortcutHandler::instance()
{
	if (s_instance == NULL)
		s_instance = new ShortcutHandler;
	return s_instance;
}

ShortcutHandler::ShortcutHandler() :
	settings(QMPlay2Core.getSettings())
{}

ShortcutHandler::~ShortcutHandler()
{}

void ShortcutHandler::appendAction(QAction *action, const QString &settingsName, const QString &default_shortcut)
{
	QString shortcut = settings.getString(settingsName, default_shortcut);
	action->setShortcut(shortcut);
	action->setProperty("settingsId", settingsName);

	m_actions.append(action);
	m_shortcuts.insert(action, shortcut);
	m_defaultShortcuts.insert(action, default_shortcut);
}

int ShortcutHandler::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return 2;
}

int ShortcutHandler::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return m_actions.count();
}

Qt::ItemFlags ShortcutHandler::flags(const QModelIndex &index) const
{
	switch (index.column())
	{
		case 0:
			return Qt::ItemIsEnabled;
		case 1:
			return Qt::ItemIsEnabled | Qt::ItemIsEditable;
	}
	return Qt::NoItemFlags;
}

QVariant ShortcutHandler::headerData(int section, Qt::Orientation orientation, int role) const
{
	Q_UNUSED(orientation);
	if (role == Qt::DisplayRole)
	{
		switch (section)
		{
			case 0:
				return tr("Action");
			case 1:
				return tr("Key sequence");
		}
	}
	return QVariant();
}

QVariant ShortcutHandler::data(const QModelIndex &index, int role) const
{
	if ((role == Qt::DisplayRole || role == Qt::EditRole) && index.row() >= 0 && index.row() < m_actions.count())
	{
		QAction *action = m_actions.at(index.row());
		switch (index.column())
		{
			case 0:
				return action->text().remove(QLatin1Char('&'));
			case 1:
				return m_shortcuts.value(action);
		}
	}
	return QVariant();
}

bool ShortcutHandler::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == Qt::EditRole && index.column() == 1 && index.row() >= 0 && index.row() < m_actions.count())
	{
		m_shortcuts.insert(m_actions.at(index.row()), value.toString());
		emit dataChanged(index, index);
		return true;
	}
	return false;
}

void ShortcutHandler::save()
{
	for (Shortcuts::const_iterator iterator = m_shortcuts.constBegin(); iterator != m_shortcuts.constEnd(); ++iterator)
	{
		iterator.key()->setShortcut(iterator.value().trimmed());
		settings.set(iterator.key()->property("settingsId").toString(), iterator.value());
	}
}

void ShortcutHandler::restore()
{
	for (Shortcuts::iterator iterator = m_shortcuts.begin(); iterator != m_shortcuts.end(); ++iterator)
	{
		iterator.value() = iterator.key()->shortcut().toString();
	}
	emit dataChanged(createIndex(0, 1), createIndex(m_actions.count(), 1));
}

void ShortcutHandler::reset()
{
	for (Shortcuts::const_iterator iterator = m_defaultShortcuts.constBegin(); iterator != m_defaultShortcuts.constEnd(); ++iterator)
	{
		m_shortcuts.insert(iterator.key(), iterator.value());
		iterator.key()->setShortcut(iterator.value().trimmed());
		settings.set(iterator.key()->property("settingsId").toString(), iterator.value());
	}
	emit dataChanged(createIndex(0, 1), createIndex(m_actions.count(), 1));
}
