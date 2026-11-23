// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <NotifyHeaders.hpp>

#include <Module.hpp>

class Notify final : public Module
{
public:
    Notify();

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
    ModuleSettingsWidget(Module &module);

private:
    void saveSettings() override;

    QGroupBox *m_notify;
    QDoubleSpinBox *m_timeoutSB;
    QCheckBox *m_notifyVolumeB, *m_notifyTitleB, *m_notifyPlayStateB;
    QGroupBox *m_customMsgG;
    QLineEdit *m_customSummary, *m_customBody;
};
