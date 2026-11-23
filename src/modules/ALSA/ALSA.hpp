// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <ALSAHeaders.hpp>

#include <Module.hpp>

class ALSA final : public Module
{
public:
    ALSA();
private:
    QList<Info> getModulesInfo(const bool) const override;
    void *createInstance(const QString &) override;

    SettingsWidget *getSettingsWidget() override;
};

/**/

class ModuleSettingsWidget final : public Module::SettingsWidget
{
    Q_DECLARE_TR_FUNCTIONS(ModuleSettingsWidget)
public:
    ModuleSettingsWidget(Module &);
private:
    void saveSettings() override;

    QCheckBox *enabledB, *autoMultichnB;
    QDoubleSpinBox *delayB;
    QComboBox *devicesB;
};
