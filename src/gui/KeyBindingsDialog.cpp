// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <KeyBindingsDialog.hpp>
#include <ShortcutHandler.hpp>
#include <Functions.hpp>
#include <Main.hpp>

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
    layout->setContentsMargins(3, 3, 3, 3);

    if (Q_LIKELY(p))
        resize(qMin(p->width(), 600), p->height() * 9 / 10);
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
