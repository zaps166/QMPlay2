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

#include <QCoreApplication>

class QDoubleSpinBox;
class QCheckBox;
class QComboBox;

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
