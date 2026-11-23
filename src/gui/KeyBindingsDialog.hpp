// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <GUIHeaders.hpp>

class KeyBindingsDialog : public QDialog
{
    Q_OBJECT
public:
    KeyBindingsDialog(QWidget *p);

private slots:
    void clicked(QAbstractButton *button);

private:
    QDialogButtonBox *buttons;
};
