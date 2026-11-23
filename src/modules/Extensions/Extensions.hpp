// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <ExtensionsHeaders.hpp>

#include <Module.hpp>

class Extensions final : public Module
{
public:
    Extensions();
private:
    QList<Info> getModulesInfo(const bool) const override;
    void *createInstance(const QString &) override;

    SettingsWidget *getSettingsWidget() override;

    QIcon downloader;
#ifdef USE_YOUTUBE
    QIcon youtube;
#endif
#ifdef USE_RADIO
    QIcon radio;
#endif
#ifdef USE_LASTFM
    QIcon lastfm;
#endif
#ifdef USE_OPENSUBTITLES
    QIcon opensubtitles;
#endif
};

/**/

class LineEdit;

class ModuleSettingsWidget final : public Module::SettingsWidget
{
    Q_OBJECT
public:
    ModuleSettingsWidget(Module &);
private slots:
#ifdef USE_LASTFM
    void loginPasswordEnable(bool checked);
    void passwordEdited();
#endif
private:
    void saveSettings() override;

#ifdef USE_MPRIS2
    QCheckBox *MPRIS2B;
#endif

#ifdef USE_YOUTUBE
    QCheckBox *userNameB, *subtitlesB;
    QComboBox *m_preferredCodec, *qualityPreset;
    QCheckBox *m_allowVp9Hdr;
#endif

#ifdef USE_LASTFM
    QGroupBox *downloadCoversGB;
    QCheckBox *allowBigCovers, *updateNowPlayingAndScrobbleB;
    LineEdit *loginE, *passwordE;
#endif
};
