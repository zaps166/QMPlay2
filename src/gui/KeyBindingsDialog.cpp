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

#include <KeyBindingsDialog.hpp>
#include <ShortcutHandler.hpp>
#include <Main.hpp>
#include <Settings.hpp>

#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QTableView>
#include <QHeaderView>

KeyBindingsDialog::KeyBindingsDialog(QWidget *p) :
	QDialog(p)
{
	setWindowTitle(tr("Key Binding settings"));

	QTableView *shortcuts = new QTableView(this);
	shortcuts->setModel(ShortcutHandler::instance());
	shortcuts->setFrameShape(QFrame::NoFrame);
	shortcuts->setAlternatingRowColors(true);
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	shortcuts->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	shortcuts->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
#else
	shortcuts->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
	shortcuts->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
#endif
	shortcuts->verticalHeader()->setVisible(false);

	buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Apply | QDialogButtonBox::RestoreDefaults, this);
	connect(buttons, SIGNAL(clicked(QAbstractButton*)), this, SLOT(clicked(QAbstractButton*)));

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(shortcuts);
	layout->addWidget(buttons);
}

void KeyBindingsDialog::clicked(QAbstractButton *button)
{
	switch (buttons->buttonRole(button))
	{
		case QDialogButtonBox::ApplyRole:
			ShortcutHandler::instance()->save();
			break;
		case QDialogButtonBox::RejectRole:
			ShortcutHandler::instance()->restore();
			break;
		case QDialogButtonBox::ResetRole:
			ShortcutHandler::instance()->reset();
			break;
		default:
			return;
	}
	this->close();
}

void KeyBindingsDialog::showEvent(QShowEvent *)
{
	QMPlay2GUI.restoreGeometry("KeyBindingsDialog/Geometry", this, QSize(450, 450));
}

void KeyBindingsDialog::closeEvent(QCloseEvent *)
{
	QMPlay2Core.getSettings().set("KeyBindingsDialog/Geometry", geometry());
}
