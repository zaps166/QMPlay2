// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Module.hpp>

class PipeWire final : public Module
{
public:
    PipeWire();
    ~PipeWire();

private:
    QList<Info> getModulesInfo(const bool) const override;
    void *createInstance(const QString &) override;

    SettingsWidget *getSettingsWidget() override;
};

/**/

#include <QCoreApplication>

class QCheckBox;

class ModuleSettingsWidget final : public Module::SettingsWidget
{
    Q_DECLARE_TR_FUNCTIONS(ModuleSettingsWidget)

public:
    ModuleSettingsWidget(Module &module);
    ~ModuleSettingsWidget();

private:
    void saveSettings() override;

    QCheckBox *m_enabledCheckBox;
};
