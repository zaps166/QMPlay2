// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QListWidget>

class OtherVFiltersW : public QListWidget
{
public:
    OtherVFiltersW(bool hw);
    ~OtherVFiltersW() = default;

    void writeSettings();
private:
    bool hw;
};
