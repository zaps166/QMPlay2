// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <FFmpegHeaders.hpp>

#include <Module.hpp>

class FFmpeg final : public Module
{
    Q_DECLARE_TR_FUNCTIONS(FFmpeg)
public:
    FFmpeg();
    ~FFmpeg();
private:
    QList<Info> getModulesInfo(const bool) const override;
    void *createInstance(const QString &) override;

    SettingsWidget *getSettingsWidget() override;

    void videoDeintSave() override;

    /**/

    QIcon demuxIcon;
#ifdef QMPlay2_VKVIDEO
    QIcon vkVideoIcon;
#endif
#ifdef QMPlay2_VAAPI
    QIcon vaapiIcon;
    QComboBox *m_vaapiDeintMethodB = nullptr;
#endif
#if defined(QMPlay2_DXVA2) || defined(QMPlay2_D3D11VA)
    QIcon dxIcon;
#endif
#ifdef QMPlay2_VTB
    QIcon vtbIcon;
#endif

#ifdef QMPlay2_VKVIDEO
    bool vkVideoHwDownload = false;
#endif
    bool vkVideoSupported = false;
    bool dxva2Supported = false;
    bool d3d11vaSupported = false;
};

/**/

class Slider;

class ModuleSettingsWidget final : public Module::SettingsWidget
{
    Q_DECLARE_TR_FUNCTIONS(ModuleSettingsWidget)
public:
    ModuleSettingsWidget(Module &module, bool vkVideo, bool dxva2, bool d3d11va);
private:
    void saveSettings() override;

    QGroupBox *demuxerB;
    QCheckBox *reconnectNetworkB;
    QCheckBox *allowExperimentalB;
    QGroupBox *hurryUpB;
    QCheckBox *skipFramesB, *forceSkipFramesB;
    QGroupBox *decoderB;
#ifdef QMPlay2_VKVIDEO
    QCheckBox *decoderVKVIDEO = nullptr;
#endif
#ifdef QMPlay2_VAAPI
    QCheckBox *decoderVAAPIEB;
#endif
#ifdef QMPlay2_DXVA2
    QCheckBox *decoderDXVA2EB = nullptr;
#endif
#ifdef QMPlay2_D3D11VA
    QGroupBox *decoderD3D11VA = nullptr;
    QCheckBox *d3d11vaZeroCopy = nullptr;
#endif
#ifdef QMPlay2_VTB
    QCheckBox *decoderVTBEB;
#endif
    QSpinBox *threadsB;
    QComboBox *lowresB, *thrTypeB;
    QSpinBox *m_teletextPageB = nullptr;
    QCheckBox *m_teletextTransparentB = nullptr;
};
