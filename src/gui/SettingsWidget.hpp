/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2026  Błażej Szczygieł

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
class QVBoxLayout;
class QHBoxLayout;
class QGroupBox;
class QComboBox;
class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;
class QRadioButton;
class QPushButton;
class QToolButton;
class QListWidget;
class QTabWidget;
class Page3;
class Page4;
class Page5;
class Page6;

struct UiModulesList
{
    QHBoxLayout *horizontalLayout;
    QListWidget *list;
    QVBoxLayout *buttonsW;
    QToolButton *moveUp;
    QToolButton *moveDown;
};

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

    void setupPlaybackUi(QWidget *w);
    void setupGeneralUi(QWidget *w);
    void setupModulesListUi(QGroupBox *groupB, UiModulesList &ml);

    void restoreVideoEq();

    void restartApp();

    QString chooseDir(const QString &currPth);

    inline QString getSelectedProfile();

    void showEvent(QShowEvent *) override;
    void closeEvent(QCloseEvent *) override;

private:
    UiModulesList modulesList[3];

    QGroupBox *m_showCoversGB;
    QCheckBox *m_showDirCoversB;
    QCheckBox *m_enlargeSmallCoversB;
    QCheckBox *m_blurCoversB;
    QPushButton *m_clearCoversCache;
    QPushButton *m_setKeyBindingsB;
    QPushButton *m_resetSettingsB;
    QComboBox *m_langBox;
    QComboBox *m_styleBox;
    QComboBox *m_encodingB;
    QComboBox *m_audioLangB;
    QComboBox *m_subsLangB;
    QLineEdit *m_screenshotE;
    QComboBox *m_screenshotFormatB;
    QToolButton *m_screenshotB;
    QLineEdit *m_outputFileE;
    QToolButton *m_outputFileB;
    QCheckBox *m_iconsFromTheme;
    QPushButton *m_setAppearanceB;
    QComboBox *m_profileB;
    QToolButton *m_profileRemoveB;
    QCheckBox *m_autoUpdatesB;
    QCheckBox *m_autoOpenVideoWindowB;
    QCheckBox *m_tabsNorths;
    QCheckBox *m_allowOnlyOneInstance;
    QCheckBox *m_displayOnlyFileName;
    QCheckBox *m_restoreRepeatMode;
    QGroupBox *m_proxyB;
    QGroupBox *m_proxyLoginB;
    QLineEdit *m_proxyUserE;
    QLineEdit *m_proxyPasswordE;
    QLineEdit *m_proxyHostE;
    QSpinBox *m_proxyPortB;
    QCheckBox *m_stillImages;
    QCheckBox *m_trayNotifiesDefault;
    QCheckBox *m_autoDelNonGroupEntries;
    QCheckBox *m_hideArtistMetadata;
    QCheckBox *m_autoRestoreMainWindowOnVideoB;
    QCheckBox *m_skipPlaylistsWithinFiles;
    QGroupBox *m_ytDlGB;
    QCheckBox *m_cookiesFromBrowserCB;
    QCheckBox *m_customYtDlCB;
    QLineEdit *m_customYtDlE;
    QToolButton *m_customYtDlB;
    QCheckBox *m_dontUpdateYtDlCB;
    QPushButton *m_removeYtDlB;
    QLineEdit *m_cookiesFromBrowserE;
    QCheckBox *m_dfltYtDlQualCB;
    QLineEdit *m_dfltYtDlQualE;
    QCheckBox *m_additionalYtDlParamsCB;
    QLineEdit *m_additionalYtDlParamsE;
    QCheckBox *m_keepDocksSizeB;
    QCheckBox *m_fullscreenPanelsRightSideB;
    QCheckBox *m_hideLogoB;
    QGroupBox *m_replayGain;
    QCheckBox *m_replayGainAlbum;
    QCheckBox *m_replayGainPreventClipping;
    QDoubleSpinBox *m_replayGainPreamp;
    QDoubleSpinBox *m_replayGainPreampNoMetadata;
    QCheckBox *m_leftMouseTogglePlay;
    QCheckBox *m_middleMouseToggleFullscreen;
    QCheckBox *m_keepVideoDelay;
    QCheckBox *m_silence;
    QCheckBox *m_keepSpeed;
    QCheckBox *m_keepZoom;
    QCheckBox *m_keepSubtitlesScale;
    QCheckBox *m_keepSubtitlesDelay;
    QCheckBox *m_syncVtoA;
    QSpinBox *m_shortSeekB;
    QSpinBox *m_longSeekB;
    QSpinBox *m_bufferLocalB;
    QDoubleSpinBox *m_bufferNetworkB;
    QComboBox *m_backwardBufferNetworkB;
    QDoubleSpinBox *m_bufferLiveB;
    QDoubleSpinBox *m_playIfBufferedB;
    QComboBox *m_desiredVideoHeightB;
    QSpinBox *m_maxVolB;
    QCheckBox *m_forceSamplerate;
    QSpinBox *m_samplerateB;
    QCheckBox *m_forceChannels;
    QSpinBox *m_channelsB;
    QCheckBox *m_resamplerFirst;
    QCheckBox *m_keepARatio;
    QCheckBox *m_accurateSeekB;
    QCheckBox *m_ignorePlaybackError;
    QCheckBox *m_savePos;
    QGroupBox *m_wheelActionB;
    QRadioButton *m_wheelSeekB;
    QRadioButton *m_wheelVolumeB;
    QCheckBox *m_restoreVideoEq;
    QCheckBox *m_showBufferedTimeOnSlider;
    QCheckBox *m_storeARatioAndZoomB;
    QCheckBox *m_unpauseWhenSeekingB;
    QCheckBox *m_disableSubtitlesAtStartup;
    QCheckBox *m_restoreAVSStateB;
    QCheckBox *m_storeUrlPosB;
    QHBoxLayout *m_modulesListLayout;

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
private:
    void chooseOutputFileDir();
private slots:
    void setAppearance();
    void setKeyBindings();
    void clearCoversCache();
#ifdef USE_YOUTUBEDL
private:
    void removeYouTubeDl();
private slots:
#endif
    void resetSettings();
    void profileListIndexChanged(int index);
    void removeProfile();
signals:
    void settingsChanged(int, bool, bool initFilters);
    void setWheelStep(int);
    void setVolMax(int);
    void keepDocksSizeChanged(bool keepDocksSize);
    void fullscreenPanelsRightSideChanged(bool checked);
};
