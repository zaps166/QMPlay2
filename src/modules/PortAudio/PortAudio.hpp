// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <PortAudioHeaders.hpp>

#include <Module.hpp>

class PortAudio final : public Module
{
public:
    PortAudio();
    ~PortAudio();

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

    QCheckBox *enabledB;
    QComboBox *devicesB;
    QDoubleSpinBox *delayB;
#ifdef Q_OS_WIN
    QCheckBox *m_exclusive;
#endif
#ifdef Q_OS_MACOS
    QCheckBox *bitPerfect;
#endif
};
