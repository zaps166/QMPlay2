/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2021  Błażej Szczygieł

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
};

/**/

class QToolButton;
class QListWidget;
class QComboBox;
class QGroupBox;
class QCheckBox;
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
#endif

#ifdef USE_LASTFM
    QGroupBox *downloadCoversGB;
    QCheckBox *allowBigCovers, *updateNowPlayingAndScrobbleB;
    LineEdit *loginE, *passwordE;
#endif
};
