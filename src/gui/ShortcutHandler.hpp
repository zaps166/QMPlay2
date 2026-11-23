// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <GUIHeaders.hpp>

class ShortcutHandler final : public QAbstractTableModel
{
    Q_DECLARE_TR_FUNCTIONS(ShortcutHandler)

public:
    ShortcutHandler(QObject *parent);
    ~ShortcutHandler();

    int columnCount(const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    void appendAction(QAction *action, const QString &settingsName, const QString &defaultShortcut);

    bool save();
    void restore();
    void reset();

private:
    QList<QAction *> m_actions;
    QHash<QAction *, QPair<QString, QString>> m_shortcuts;
};
