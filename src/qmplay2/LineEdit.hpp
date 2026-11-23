// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

class QMPLAY2SHAREDLIB_EXPORT LineEdit final : public QLineEdit
{
    Q_OBJECT

public:
    LineEdit(QWidget *parent = nullptr);

    void clearText();

signals:
    void clearButtonClicked();
};
