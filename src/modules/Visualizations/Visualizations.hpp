// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <VisualizationsHeaders.hpp>

#include <Module.hpp>

class Visualizations final : public Module
{
public:
    Visualizations();
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

    QSpinBox *refTimeB = nullptr, *sndLenB, *fftSizeB;
    QComboBox *m_fftLimitFreqB;
    QCheckBox *m_fftLinearScaleB;
};
