// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Module.hpp>

class FileAssociation final : public Module
{
    Q_OBJECT
public:
    FileAssociation();
private:
    QList<Info> getModulesInfo(const bool) const override;
    void *createInstance(const QString &) override;

    SettingsWidget *getSettingsWidget() override;

    bool reallyFirsttime;
private slots:
    void firsttime();
};

/**/

class QListWidget;
class QPushButton;
class QGroupBox;
class QCheckBox;

class ModuleSettingsWidget final : public Module::SettingsWidget
{
    Q_OBJECT
    friend class FileAssociation;
public:
    ModuleSettingsWidget(Module &);
private:
    void saveSettings() override;
    void addExtension(const QString &, const bool, const bool isPlaylist = false);
private slots:
    void selectAll();
    void checkSelected();
private:
    bool ignoreCheck;

    QGroupBox *associateB;
    QPushButton *selectAllB;
    QListWidget *extensionLW;
    QCheckBox *addDirB, *addDrvB, *audioCDB;
};
