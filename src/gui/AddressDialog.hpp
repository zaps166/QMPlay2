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

#pragma once

#include <AddressBox.hpp>

#include <QCoreApplication>
#include <QCheckBox>
#include <QDialog>

class AddressDialog final : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(AddressDialog)
public:
    AddressDialog(QWidget *);
    ~AddressDialog();

    inline bool addAndPlay() const
    {
        return addAndPlayB.isChecked();
    }

    inline QString url() const
    {
        return addrB.url();
    }
private:
    AddressBox addrB;
    QCheckBox addAndPlayB;
};
