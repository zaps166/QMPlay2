// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <OpenMPTHeaders.hpp>

#include <Module.hpp>

class OpenMPT final : public Module
{
public:
    OpenMPT();

private:
    QList<Info> getModulesInfo(const bool) const override;
    void *createInstance(const QString &) override;
    SettingsWidget *getSettingsWidget() override;

private:
    QIcon m_modIcon{":/MOD.svg"};
};

/**/

class ModuleSettingsWidget final : public Module::SettingsWidget
{
    Q_DECLARE_TR_FUNCTIONS(ModuleSettingsWidget)

public:
    ModuleSettingsWidget(Module &);

private:
    void saveSettings() override;

private:
    QCheckBox *m_enabled{nullptr};
    QComboBox *m_subsongMode{nullptr};
    QSpinBox *m_stereoSeparation{nullptr};
    QComboBox *m_interpolationFilter{nullptr};
    QComboBox *m_visMode{nullptr};
    QWidget *m_visSettingsWidget{nullptr};
    QSpinBox *m_visChannels{nullptr};
    QSpinBox *m_visRows{nullptr};
};
