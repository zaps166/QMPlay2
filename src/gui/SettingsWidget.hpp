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

#include <QWidget>

#include <functional>

class QListWidgetItem;
class QGridLayout;
class QPushButton;
class QTabWidget;
class Page3;
class Page4;
class Page5;
class Page6;

namespace Ui {
    class GeneralSettings;
    class PlaybackSettings;
    class ModulesList;
}

class SettingsWidget final : public QWidget
{
    Q_OBJECT
public:
    static void InitSettings();
    static void SetAudioChannelsMenu();
    static void SetAudioChannels(int chn);

    SettingsWidget(int page, const QString &module, QWidget *videoEq);
    ~SettingsWidget();

    void setAudioChannels();
private:
    static void applyProxy();

    void createRendererSettings();

    void restoreVideoEq();

    void restartApp();

    inline QString getSelectedProfile();

    void showEvent(QShowEvent *) override;
    void closeEvent(QCloseEvent *) override;

private:
    Ui::GeneralSettings *generalSettingsPage;
    Ui::PlaybackSettings *playbackSettingsPage;
    Ui::ModulesList *modulesList[3];
    Page3 *page3;
    Page4 *page4;
    Page5 *page5;
    Page6 *page6;

    QWidget *m_modulesListGroupBox[3];

    QTabWidget *tabW;
    QString lastM[3];

    QWidget *videoEq, *videoEqOriginalParent;

    bool wasShow;
    int moduleIndex;

    std::function<void()> m_setRenderersCurrentIndexFn;
    std::vector<std::function<void(bool &initFilters)>> m_rendererApplyFunctions;
private slots:
    void chStyle();
    void apply();
    void chModule(QListWidgetItem *);
    void tabCh(int);
    void openModuleSettings(QListWidgetItem *);
    void moveModule();
    void chooseScreenshotDir();
    void setAppearance();
    void setKeyBindings();
    void clearCoversCache();
#ifdef USE_YOUTUBEDL
    void removeYouTubeDl();
#endif
    void resetSettings();
    void profileListIndexChanged(int index);
    void removeProfile();
signals:
    void settingsChanged(int, bool, bool initFilters);
    void setWheelStep(int);
    void setVolMax(int);
};
