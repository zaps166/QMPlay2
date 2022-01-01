/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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
#include <Settings.hpp>

#include <QAction>
#include <QDebug>

ShortcutHandler::ShortcutHandler(QObject *parent) :
    QAbstractTableModel(parent)
{}
ShortcutHandler::~ShortcutHandler()
{}

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
            return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
    }
    return Qt::NoItemFlags;
}

QVariant ShortcutHandler::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
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
        QAction *action = m_actions[index.row()];
        switch (index.column())
        {
            case 0:
                return action->text().remove('&');
            case 1:
                return m_shortcuts.value(action).first;
        }
    }
    return QVariant();
}
bool ShortcutHandler::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole && index.column() == 1 && index.row() >= 0 && index.row() < m_actions.count())
    {
        const QString trimmedValue = value.toString().trimmed();
        const QString shortcut = QKeySequence(trimmedValue).toString();
        if (!shortcut.isEmpty() || trimmedValue.isEmpty())
        {
            m_shortcuts[m_actions.at(index.row())].first = shortcut;
            emit dataChanged(index, index);
            return true;
        }
    }
    return false;
}

void ShortcutHandler::appendAction(QAction *action, const QString &settingsName, const QString &defaultShortcut)
{
    const QString shortcut = QMPlay2Core.getSettings().getString(settingsName, defaultShortcut);
    action->setProperty("settingsId", settingsName);
    action->setShortcut(shortcut);

    m_actions.append(action);
    m_shortcuts[action] = {action->shortcut().toString(), defaultShortcut};
}

bool ShortcutHandler::save()
{
    QSet<QString> shortcuts;
    for (auto it = m_shortcuts.constBegin(), itEnd = m_shortcuts.constEnd(); it != itEnd; ++it)
    {
        const QString &shortcut = it.value().first;
        if (shortcut.isEmpty())
            continue;

        if (shortcuts.contains(shortcut))
        {
            qDebug() << "Duplicated shortcut:" << shortcut;
            return false;
        }

        shortcuts.insert(shortcut);
    }

    Settings &settings = QMPlay2Core.getSettings();
    for (auto it = m_shortcuts.constBegin(), itEnd = m_shortcuts.constEnd(); it != itEnd; ++it)
    {
        const QString &shortcut = it.value().first;
        it.key()->setShortcut(shortcut);
        settings.set(it.key()->property("settingsId").toString(), shortcut);
    }
    settings.flush();

    return true;
}
void ShortcutHandler::restore()
{
    for (auto it = m_shortcuts.begin(), itEnd = m_shortcuts.end(); it != itEnd; ++it)
        it.value().first = it.key()->shortcut().toString();
    emit dataChanged(createIndex(0, 1), createIndex(m_actions.count(), 1));
}
void ShortcutHandler::reset()
{
    for (auto it = m_shortcuts.constBegin(), itEnd = m_shortcuts.constEnd(); it != itEnd; ++it)
    {
        const QString defaultShortcut = it.value().second;
        m_shortcuts[it.key()].first = defaultShortcut;
    }
    emit dataChanged(createIndex(0, 1), createIndex(m_actions.count(), 1));
}
