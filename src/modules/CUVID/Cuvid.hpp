// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <CUVIDHeaders.hpp>

#include <Module.hpp>

class Cuvid final : public Module
{
    Q_DECLARE_TR_FUNCTIONS(Cuvid)

public:
    Cuvid();
    ~Cuvid();

private:
    QList<Info> getModulesInfo(const bool showDisabled) const override;
    void *createInstance(const QString &name) override;

    SettingsWidget *getSettingsWidget() override;

    void videoDeintSave() override;

    /**/

    QComboBox *m_deintMethodB = nullptr;
};

/**/

class ModuleSettingsWidget : public Module::SettingsWidget
{
    Q_DECLARE_TR_FUNCTIONS(ModuleSettingsWidget)

public:
    ModuleSettingsWidget(Module &module);

private:
    void saveSettings() override;

    QCheckBox *m_enabledB, *m_decodeMPEG4;
#ifdef Q_OS_WIN
    QCheckBox *m_checkFirstGPU;
#endif
};
