// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Module.hpp>

#include <QCoreApplication>

class OpenSLES : public Module
{
public:
    OpenSLES();
private:
    QList<Info> getModulesInfo(const bool) const;
    void *createInstance(const QString &);

    SettingsWidget *getSettingsWidget();
};

/**/

class QCheckBox;

class ModuleSettingsWidget : public Module::SettingsWidget
{
    Q_DECLARE_TR_FUNCTIONS(ModuleSettingsWidget)

public:
    ModuleSettingsWidget(Module &);
private:
    void saveSettings();

    QCheckBox *openslesB;
};
