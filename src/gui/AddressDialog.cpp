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

#include <AddressDialog.hpp>

#include <Settings.hpp>

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLineEdit>

AddressDialog::AddressDialog(QWidget *p)
    : QDialog(p)
    , addrB(Qt::Vertical, QString(), QMPlay2Core.getSettings().getString("AddressDialog/Choice"))
{
    setWindowTitle(tr("Add address"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    addAndPlayB.setText(tr("Play"));
    addAndPlayB.setChecked(QMPlay2Core.getSettings().getBool("AddressDialog/AddAndPlay", true));

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(&addrB, 0, 0, 1, 2);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 1, 0, 1, 2); //vSpacer
    layout->addWidget(&addAndPlayB, 2, 0, 1, 1);
    layout->addWidget(buttonBox, 2, 1, 1, 1);
    layout->setMargin(3);

    addrB.setFocus();
    resize(625, 0);
}
AddressDialog::~AddressDialog()
{
    if (result())
    {
        QMPlay2Core.getSettings().set("AddressDialog/AddAndPlay", addAndPlayB.isChecked());
        QMPlay2Core.getSettings().set("AddressDialog/Choice", addrB.getCurrentText());
    }
}
