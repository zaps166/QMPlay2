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

#include <QCoreApplication>

class QComboBox;

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

class QCheckBox;
class QLabel;

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
