// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QGroupBox>

class QCheckBox;
class QComboBox;
class Module;

class DeintSettingsW final : public QGroupBox
{
    Q_OBJECT
public:
    static void init();

    DeintSettingsW();
    ~DeintSettingsW();

    void setDeintEnabledDisabled();

    void writeSettings();
private slots:
    void softwareMethods(bool doubler);
    void setSoftwareMethodsToolTip(int idx);
private:
    QCheckBox *autoDeintB, *doublerB, *autoParityB;
    QCheckBox *m_vkYadifSpatialCheck = nullptr;
    QComboBox *softwareMethodsCB, *parityCB;
};
