// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <GUIHeaders.hpp>

#include <AddressBox.hpp>

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
