// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Module.hpp>

class Chiptune final : public Module
{
public:
    Chiptune();
private:
    QList<Info> getModulesInfo(const bool) const override;
    void *createInstance(const QString &) override;

    SettingsWidget *getSettingsWidget() override;

    QIcon GMEIcon, SIDIcon;
};

/**/

#include <QCoreApplication>

class QCheckBox;
class QSpinBox;

class ModuleSettingsWidget final : public Module::SettingsWidget
{
    Q_DECLARE_TR_FUNCTIONS(ModuleSettingsWidget)
public:
    ModuleSettingsWidget(Module &);
private:
    void saveSettings() override;

#ifdef USE_GME
    QCheckBox *gmeB;
#endif
#ifdef USE_SIDPLAY
    QCheckBox *sidB;
#endif
    QSpinBox *lengthB;
};
