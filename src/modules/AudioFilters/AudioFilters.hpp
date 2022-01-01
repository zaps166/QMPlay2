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

class AudioFilters final : public Module
{
public:
    AudioFilters();
private:
    QList<Info> getModulesInfo(const bool) const override;
    void *createInstance(const QString &) override;

    SettingsWidget *getSettingsWidget() override;
};

/**/

class QDoubleSpinBox;
class QPushButton;
class QGroupBox;
class QCheckBox;
class QComboBox;
class QSpinBox;
class Slider;

class ModuleSettingsWidget final : public Module::SettingsWidget
{
    Q_OBJECT
public:
    ModuleSettingsWidget(Module &);
private slots:
    void bs2b();
    void voiceRemovalToggle();
    void phaseReverse();
    void swapStereo();
    void echo();
    void compressor();
    void defaultSettings();
private:
    void saveSettings() override;

    bool restoringDefault;

    QGroupBox *bs2bB;
    QSpinBox *bs2bFcutB;
    QDoubleSpinBox *bs2bFeedB;

    QCheckBox *voiceRemovalB;

    QGroupBox *phaseReverseB;
    QCheckBox *phaseReverseRightB;

    QCheckBox *swapStereoB;

    QGroupBox *echoB;
    Slider *echoDelayS, *echoVolumeS, *echoFeedbackS;
    QCheckBox *echoSurroundB;

    QGroupBox *compressorB;
    Slider *compressorPeakS, *compressorReleaseTimeS, *compressorFastRatioS, *compressorRatioS;

    QComboBox *eqQualityB;
    QSpinBox *eqSlidersB, *eqMinFreqB, *eqMaxFreqB;

    QPushButton *defaultB;
};
