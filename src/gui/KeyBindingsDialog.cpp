/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2021  Błażej Szczygieł

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

#include <KeyBindingsDialog.hpp>
#include <ShortcutHandler.hpp>
#include <Functions.hpp>
#include <Main.hpp>

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QHeaderView>
#include <QTableView>
#include <QBoxLayout>

KeyBindingsDialog::KeyBindingsDialog(QWidget *p) :
    QDialog(p)
{
    setWindowTitle(tr("Key bindings settings"));

    QTableView *shortcuts = new QTableView;
    shortcuts->setModel(QMPlay2GUI.shortcutHandler);
    shortcuts->setFrameShape(QFrame::NoFrame);
    shortcuts->setAlternatingRowColors(true);
    shortcuts->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    shortcuts->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    shortcuts->setSelectionMode(QAbstractItemView::SingleSelection);
    shortcuts->verticalHeader()->setVisible(false);

    buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults, Qt::Horizontal, this);
    connect(buttons, SIGNAL(clicked(QAbstractButton *)), this, SLOT(clicked(QAbstractButton *)));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(shortcuts);
    layout->addWidget(buttons);
    layout->setMargin(3);

    if (QWidget *w = parentWidget())
        resize(600, w->height() * 9 / 10);
}

void KeyBindingsDialog::clicked(QAbstractButton *button)
{
    switch (buttons->buttonRole(button))
    {
        case QDialogButtonBox::AcceptRole:
            if (!QMPlay2GUI.shortcutHandler->save())
            {
                QMessageBox::warning(this, windowTitle(), tr("Some key shortcuts are duplicated!"));
                return;
            }
            break;
        case QDialogButtonBox::RejectRole:
            QMPlay2GUI.shortcutHandler->restore();
            break;
        case QDialogButtonBox::ResetRole:
            QMPlay2GUI.shortcutHandler->reset();
            return;
        default:
            break;
    }
    close();
}
