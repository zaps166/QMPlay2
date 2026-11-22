// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <LineEdit.hpp>

#include <QMPlay2Core.hpp>

#include <QAction>

LineEdit::LineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    QAction *clearAct = addAction(QMPlay2Core.getIconFromTheme("edit-clear"), TrailingPosition);
    connect(clearAct, &QAction::triggered, this, &LineEdit::clearText);
    connect(this, &LineEdit::textChanged, this, [=](const QString &text) {
        clearAct->setVisible(!text.isEmpty());
    });
    clearAct->setToolTip(tr("Clear"));
    clearAct->setVisible(false);
}

void LineEdit::clearText()
{
    clear();
    emit clearButtonClicked();
}
