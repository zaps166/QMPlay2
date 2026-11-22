// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Module.hpp>

#include <QCoreApplication>

class CDIODestroyTimer;

class AudioCD final : public Module
{
    Q_OBJECT
public:
    AudioCD();
    ~AudioCD();
private:
    QList<Info> getModulesInfo(const bool) const override;
    void *createInstance(const QString &) override;

    QList<QAction *> getAddActions() override;

    SettingsWidget *getSettingsWidget() override;

    QIcon CD;

    CDIODestroyTimer *cdioDestroyTimer;
private slots:
    void add();
    void browseCDImage();
};

/**/

class QGridLayout;
class QGroupBox;
class QCheckBox;

class ModuleSettingsWidget final : public Module::SettingsWidget
{
    Q_DECLARE_TR_FUNCTIONS(ModuleSettingsWidget)
public:
    ModuleSettingsWidget(Module &);
private:
    void saveSettings() override;

    QGroupBox *audioCDB;
    QCheckBox *useCDDB, *useCDTEXT;
};
