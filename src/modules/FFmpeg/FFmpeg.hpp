/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

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

    bool vkVideoHwDownload = false;
    bool vkVideoSupported = false;
    bool dxva2Supported = false;
    bool d3d11vaSupported = false;
};

/**/

class QCheckBox;
class QGroupBox;
class QSpinBox;
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
};
