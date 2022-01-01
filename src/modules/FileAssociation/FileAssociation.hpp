/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
