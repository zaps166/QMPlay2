// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <VideoFiltersHeaders.hpp>

#include <Module.hpp>

class VFilters final : public Module
{
    Q_DECLARE_TR_FUNCTIONS(VFilters)
public:
    VFilters();
private:
    QList<Info> getModulesInfo(const bool) const override;
    void *createInstance(const QString &) override;

    SettingsWidget *getSettingsWidget() override;

private:
    bool m_fullScreen = false;
};

/**/

class ModuleSettingsWidget final : public Module::SettingsWidget
{
    Q_DECLARE_TR_FUNCTIONS(ModuleSettingsWidget)

public:
    ModuleSettingsWidget(Module &module);

private:
    void saveSettings() override;

private:
    QDoubleSpinBox *const m_minFpsSpinBox;
    QDoubleSpinBox *const m_maxFpsSpinBox;
    QCheckBox *const m_onlyFullScreenCheckBox;
};
