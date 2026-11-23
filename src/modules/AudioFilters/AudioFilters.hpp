// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <AudioFiltersHeaders.hpp>

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

#ifdef USE_AVAUDIOFILTER
    QGroupBox *m_avAudioFilterB = nullptr;
    QLineEdit *m_avAudioFilterE = nullptr;
#endif

    QPushButton *defaultB;
};
