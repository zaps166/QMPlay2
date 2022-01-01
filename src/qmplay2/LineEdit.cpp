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
