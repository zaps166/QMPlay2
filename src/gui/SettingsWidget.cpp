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

#include <SettingsWidget.hpp>

#include <DeintSettingsW.hpp>
#include <OtherVFiltersW.hpp>
#include <OSDSettingsW.hpp>
#include <Functions.hpp>
#ifdef USE_YOUTUBEDL
#   include <YouTubeDL.hpp>
#endif
#include <Notifies.hpp>
#include <Main.hpp>
#ifdef USE_VULKAN
#   include "../qmvk/PhysicalDevice.hpp"

#   include <vulkan/VulkanInstance.hpp>
#endif

#include <QStackedWidget>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QRadioButton>
#include <QApplication>
#include <QFormLayout>
#include <QFileDialog>
#include <QPushButton>
#include <QListWidget>
#include <QMessageBox>
#include <QScrollArea>
#include <QToolButton>
#include <QGridLayout>
#include <QMainWindow>
#include <QBoxLayout>
#include <QTextCodec>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QBuffer>
#include <QLabel>

#include <KeyBindingsDialog.hpp>
#include <Appearance.hpp>
#include <Settings.hpp>
#include <MenuBar.hpp>
#include <Module.hpp>

#include "ui_SettingsGeneral.h"
#include "ui_SettingsPlayback.h"
#include "ui_SettingsPlaybackModulesList.h"

#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS) && !defined(Q_OS_ANDROID)
    #define ICONS_FROM_THEME
#endif

class Page3 : public QWidget
{
public:
    inline Page3() :
        module(nullptr)
    {}

    QListWidget *listW;
    QScrollArea *scrollA;
    Module *module;
};
class Page4 : public OSDSettingsW
{
public:
    inline Page4() :
        OSDSettingsW("Subtitles")
    {}

    QGroupBox *toAssGB;
    QCheckBox *colorsAndBordersB, *marginsAndAlignmentB, *fontsB, *overridePlayResB;
};
class Page5 : public OSDSettingsW
{
public:
    inline Page5() :
        OSDSettingsW("OSD")
    {}

    QCheckBox *enabledB;
};
class Page6 : public QScrollArea
{
public:
    inline Page6()
    {
        setWidgetResizable(true);
        setFrameShape(QFrame::NoFrame);
    }

    DeintSettingsW *deintSettingsW;
    QGroupBox *videoEqContainer;
    OtherVFiltersW *otherVFiltersW;
};

template<typename TextWidget>
static inline void appendColon(TextWidget *tw)
{
    tw->setText(tw->text() + ": ");
}

/**/

void SettingsWidget::InitSettings()
{
    Settings &QMPSettings = QMPlay2Core.getSettings();

    QTextCodec *codec = QTextCodec::codecForLocale();
    QMPSettings.init("FallbackSubtitlesEncoding", codec ? codec->name().constData() : "System");

    QMPSettings.init("AudioEnabled", true);
    QMPSettings.init("VideoEnabled", true);
    QMPSettings.init("SubtitlesEnabled", true);

    QMPSettings.init("AudioLanguage", QString());
    QMPSettings.init("SubtitlesLanguage", QString());
    QMPSettings.init("screenshotPth", QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).value(0, QDir::homePath()));
#ifdef Q_OS_WIN
    QMPSettings.init("screenshotFormat", ".bmp");
#else
    QMPSettings.init("screenshotFormat", ".ppm");
#endif
#ifdef ICONS_FROM_THEME
    QMPSettings.init("IconsFromTheme", true);
#else
    QMPSettings.init("IconsFromTheme", false);
#endif
    QMPSettings.init("ShowCovers", true);
    QMPSettings.init("BlurCovers", true);
    QMPSettings.init("EnlargeCovers", false);
    QMPSettings.init("ShowDirCovers", true);
    QMPSettings.init("AutoOpenVideoWindow", true);
    QMPSettings.init("AutoRestoreMainWindowOnVideo", true);
#ifdef UPDATES
    if (!QMPSettings.contains("AutoUpdates"))
        QMPSettings.init("AutoUpdates", !QFile::exists(QMPlay2Core.getShareDir() + "noautoupdates"));
#endif
    QMPSettings.init("MainWidget/TabPositionNorth", false);
#ifdef QMPLAY2_ALLOW_ONLY_ONE_INSTANCE
    QMPSettings.init("AllowOnlyOneInstance", false);
#endif
    QMPSettings.init("HideArtistMetadata", false);
    QMPSettings.init("DisplayOnlyFileName", false);
    QMPSettings.init("SkipPlaylistsWithinFiles", true);
    QMPSettings.init("RestoreRepeatMode", false);
    QMPSettings.init("StillImages", false);
    QMPSettings.init("TrayNotifiesDefault", false);
    QMPSettings.init("AutoDelNonGroupEntries", false);
    QMPSettings.init("Proxy/Use", false);
    QMPSettings.init("Proxy/Host", QString());
    QMPSettings.init("Proxy/Port", 80);
    QMPSettings.init("Proxy/Login", false);
    QMPSettings.init("Proxy/User", QString());
    QMPSettings.init("Proxy/Password", QString());

    QMPSettings.init("OpenGL/OnWindow", false);
    QMPSettings.init("OpenGL/VSync", true);
    QMPSettings.init("OpenGL/BypassCompositor", false);

    QMPSettings.init("Vulkan/Device", QByteArray());
#ifdef Q_OS_WIN
    // Use V-Sync by default, because Mailbox present mode behaves weirdly on NVIDIA on Windows
    QMPSettings.init("Vulkan/VSync", Qt::Checked);
#else
    QMPSettings.init("Vulkan/VSync", Qt::PartiallyChecked);
#endif
    QMPSettings.init("Vulkan/AlwaysGPUDeint", true);
    QMPSettings.init("Vulkan/ForceVulkanYadif", false);
    QMPSettings.init("Vulkan/YadifSpatialCheck", true);
    QMPSettings.init("Vulkan/HQScaleDown", false);
    QMPSettings.init("Vulkan/HQScaleUp", false);
    QMPSettings.init("Vulkan/BypassCompositor", true);

    QMPSettings.init("ShortSeek", 5);
    QMPSettings.init("LongSeek", 30);
    QMPSettings.init("AVBufferLocal", 100);
    QMPSettings.init("AVBufferNetwork", 50000);
    QMPSettings.init("BackwardBuffer", 1);
    QMPSettings.init("PlayIfBuffered", 1.75);
    QMPSettings.init("MaxVol", 100);
    QMPSettings.init("VolumeL", 100);
    QMPSettings.init("VolumeR", 100);
    QMPSettings.init("ForceSamplerate", false);
    QMPSettings.init("Samplerate", 48000);
    QMPSettings.init("ForceChannels", false);
    QMPSettings.init("Channels", 2);
    QMPSettings.init("ResamplerFirst", true);
    QMPSettings.init("ReplayGain/Enabled", false);
    QMPSettings.init("ReplayGain/Album", false);
    QMPSettings.init("ReplayGain/PreventClipping", true);
    QMPSettings.init("ReplayGain/Preamp", 0.0);
    QMPSettings.init("ShowBufferedTimeOnSlider", true);
    QMPSettings.init("WheelAction", true);
    QMPSettings.init("WheelSeek", true);
    QMPSettings.init("LeftMouseTogglePlay", 0);
    QMPSettings.init("MiddleMouseToggleFullscreen", false);
    QMPSettings.init("AccurateSeek", Qt::PartiallyChecked);
    QMPSettings.init("UnpauseWhenSeeking", false);
    QMPSettings.init("RestoreAVSState", false);
    QMPSettings.init("DisableSubtitlesAtStartup", false);
    QMPSettings.init("StoreARatioAndZoom", false);
    QMPSettings.init("SavePos", false);
    QMPSettings.init("KeepZoom", false);
    QMPSettings.init("KeepARatio", false);
    QMPSettings.init("KeepSubtitlesDelay", false);
    QMPSettings.init("KeepSubtitlesScale", false);
    QMPSettings.init("KeepVideoDelay", false);
    QMPSettings.init("KeepSpeed", false);
    QMPSettings.init("SyncVtoA", true);
    QMPSettings.init("Silence", true);
    QMPSettings.init("RestoreVideoEqualizer", false);
    QMPSettings.init("IgnorePlaybackError", false);
    QMPSettings.init("ApplyToASS/ColorsAndBorders", true);
    QMPSettings.init("ApplyToASS/MarginsAndAlignment", false);
    QMPSettings.init("ApplyToASS/FontsAndSpacing", false);
    QMPSettings.init("ApplyToASS/OverridePlayRes", false);
    QMPSettings.init("ApplyToASS/ApplyToASS", false);
    QMPSettings.init("OSD/Enabled", true);
    OSDSettingsW::init("Subtitles", 20, 0, 15, 15, 15, 7, 1.5, 1.5, QColor(0xFF, 0xA8, 0x58, 0xFF), Qt::black, Qt::black);
    OSDSettingsW::init("OSD",       32, 0, 0,  0,  0,  4, 1.5, 1.5, QColor(0xAA, 0xFF, 0x55, 0xFF), Qt::black, Qt::black);
    DeintSettingsW::init();
    applyProxy();
}
void SettingsWidget::SetAudioChannelsMenu()
{
    const bool forceChn = QMPlay2Core.getSettings().getBool("ForceChannels");
    const int chn = QMPlay2Core.getSettings().getInt("Channels");
    bool audioChannelsChecked = false;
    for (QAction *act : QMPlay2GUI.menuBar->playback->audioChannels->actions())
        if ((!forceChn && act->objectName() == "auto") || (forceChn && chn == act->objectName().toInt()))
        {
            act->setChecked(true);
            audioChannelsChecked = true;
        }
    if (!audioChannelsChecked)
        QMPlay2GUI.menuBar->playback->audioChannels->other->setChecked(true);
}
void SettingsWidget::SetAudioChannels(int chn)
{
    const bool forceChannels = chn >= 1 && chn <= 8;
    if (forceChannels)
        QMPlay2Core.getSettings().set("Channels", chn);
    QMPlay2Core.getSettings().set("ForceChannels", forceChannels);
}

SettingsWidget::SettingsWidget(int page, const QString &moduleName, QWidget *videoEq) :
    videoEq(videoEq), videoEqOriginalParent(videoEq->parentWidget()),
    wasShow(false),
    moduleIndex(0)
{
    setWindowFlags(Qt::Window);
    setWindowTitle(tr("Settings"));
    setAttribute(Qt::WA_DeleteOnClose);

    Settings &QMPSettings = QMPlay2Core.getSettings();

    tabW = new QTabWidget;

    QPushButton *applyB = new QPushButton;
    applyB->setText(tr("Apply"));
    connect(applyB, SIGNAL(clicked()), this, SLOT(apply()));

    QPushButton *closeB = new QPushButton;
    closeB->setText(tr("Close"));
    closeB->setShortcut(QKeySequence("Escape"));
    connect(closeB, SIGNAL(clicked()), this, SLOT(close()));

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(tabW, 0, 0, 1, 3);
    layout->addWidget(applyB, 1, 1, 1, 1);
    layout->addWidget(closeB, 1, 2, 1, 1);
    layout->setMargin(2);

    {
        QWidget *generalSettingsWidget = new QWidget;
        generalSettingsPage = new Ui::GeneralSettings;
        generalSettingsPage->setupUi(generalSettingsWidget);

        appendColon(generalSettingsPage->langL);
        appendColon(generalSettingsPage->styleL);
        appendColon(generalSettingsPage->encodingL);
        appendColon(generalSettingsPage->audioLangL);
        appendColon(generalSettingsPage->subsLangL);
        appendColon(generalSettingsPage->screenshotL);
        appendColon(generalSettingsPage->profileL);

        tabW->addTab(generalSettingsWidget, tr("General settings"));

        int idx;

        generalSettingsPage->langBox->addItem("English", "en");
        generalSettingsPage->langBox->setCurrentIndex(0);
        const QStringList langs = QMPlay2Core.getLanguages();
        for (int i = 0; i < langs.count(); i++)
        {
            generalSettingsPage->langBox->addItem(QMPlay2Core.getLongFromShortLanguage(langs[i]), langs[i]);
            if (QMPlay2Core.getLanguage() == langs[i])
                generalSettingsPage->langBox->setCurrentIndex(i + 1);
        }

        generalSettingsPage->styleBox->addItems(QStyleFactory::keys());
        idx = generalSettingsPage->styleBox->findText(QApplication::style()->objectName(), Qt::MatchFixedString);
        if (idx > -1 && idx < generalSettingsPage->styleBox->count())
            generalSettingsPage->styleBox->setCurrentIndex(idx);
        connect(generalSettingsPage->styleBox, SIGNAL(currentIndexChanged(int)), this, SLOT(chStyle()));

        QStringList encodings;
        for (const QByteArray &item : QTextCodec::availableCodecs())
            encodings += QTextCodec::codecForName(item)->name();
        encodings.removeDuplicates();
        generalSettingsPage->encodingB->addItems(encodings);
        idx = generalSettingsPage->encodingB->findText(QMPSettings.getByteArray("FallbackSubtitlesEncoding"));
        if (idx > -1)
            generalSettingsPage->encodingB->setCurrentIndex(idx);

        const QString audioLang = QMPSettings.getString("AudioLanguage");
        const QString subsLang = QMPSettings.getString("SubtitlesLanguage");
        generalSettingsPage->audioLangB->addItem(tr("Default or first stream"));
        generalSettingsPage->subsLangB->addItem(tr("Default or first stream"));
        for (const QString &lang : QMPlay2Core.getLanguagesMap())
        {
            generalSettingsPage->audioLangB->addItem(lang);
            generalSettingsPage->subsLangB->addItem(lang);
            if (lang == audioLang)
                generalSettingsPage->audioLangB->setCurrentIndex(generalSettingsPage->audioLangB->count() - 1);
            if (lang == subsLang)
                generalSettingsPage->subsLangB->setCurrentIndex(generalSettingsPage->subsLangB->count() - 1);
        }
        {
            const QString currentProfile = QSettings(QMPlay2Core.getSettingsDir() + "Profile.ini", QSettings::IniFormat).value("Profile").toString();
            generalSettingsPage->profileB->addItem(tr("Default"));
            for (const QString &profile : QDir(QMPlay2Core.getSettingsDir() + "Profiles/").entryList(QDir::Dirs | QDir::NoDotAndDotDot))
            {
                generalSettingsPage->profileB->addItem(profile);
                if (profile == currentProfile)
                    generalSettingsPage->profileB->setCurrentIndex(generalSettingsPage->profileB->count() - 1);
            }
            connect(generalSettingsPage->profileB, SIGNAL(currentIndexChanged(int)), this, SLOT(profileListIndexChanged(int)));

            generalSettingsPage->profileRemoveB->setIcon(QMPlay2Core.getIconFromTheme("list-remove"));
            generalSettingsPage->profileRemoveB->setEnabled(generalSettingsPage->profileB->currentIndex() != 0);
            connect(generalSettingsPage->profileRemoveB, SIGNAL(clicked()), this, SLOT(removeProfile()));
        }

        generalSettingsPage->screenshotE->setText(QMPSettings.getString("screenshotPth"));
        generalSettingsPage->screenshotFormatB->setCurrentIndex(generalSettingsPage->screenshotFormatB->findText(QMPSettings.getString("screenshotFormat")));
        generalSettingsPage->screenshotB->setIcon(QMPlay2Core.getIconFromTheme("folder-open"));
        connect(generalSettingsPage->screenshotB, SIGNAL(clicked()), this, SLOT(chooseScreenshotDir()));

        connect(generalSettingsPage->setAppearanceB, SIGNAL(clicked()), this, SLOT(setAppearance()));
        connect(generalSettingsPage->setKeyBindingsB, SIGNAL(clicked()), this, SLOT(setKeyBindings()));

#ifdef ICONS_FROM_THEME
        generalSettingsPage->iconsFromTheme->setChecked(QMPSettings.getBool("IconsFromTheme"));
#else
        delete generalSettingsPage->iconsFromTheme;
        generalSettingsPage->iconsFromTheme = nullptr;
#endif

        generalSettingsPage->showCoversGB->setChecked(QMPSettings.getBool("ShowCovers"));
        generalSettingsPage->blurCoversB->setChecked(QMPSettings.getBool("BlurCovers"));
        generalSettingsPage->showDirCoversB->setChecked(QMPSettings.getBool("ShowDirCovers"));
        generalSettingsPage->enlargeSmallCoversB->setChecked(QMPSettings.getBool("EnlargeCovers"));

        generalSettingsPage->autoOpenVideoWindowB->setChecked(QMPSettings.getBool("AutoOpenVideoWindow"));
        generalSettingsPage->autoRestoreMainWindowOnVideoB->setChecked(QMPSettings.getBool("AutoRestoreMainWindowOnVideo"));

#ifdef UPDATES
        generalSettingsPage->autoUpdatesB->setChecked(QMPSettings.getBool("AutoUpdates"));
# ifndef UPDATER
        generalSettingsPage->autoUpdatesB->setText(tr("Automatically check for updates"));
# endif
#else
        delete generalSettingsPage->autoUpdatesB;
        generalSettingsPage->autoUpdatesB = nullptr;
#endif

        if (Notifies::hasBoth())
            generalSettingsPage->trayNotifiesDefault->setChecked(QMPSettings.getBool("TrayNotifiesDefault"));
        else
        {
            delete generalSettingsPage->trayNotifiesDefault;
            generalSettingsPage->trayNotifiesDefault = nullptr;
        }

        generalSettingsPage->autoDelNonGroupEntries->setChecked(QMPSettings.getBool("AutoDelNonGroupEntries"));

        generalSettingsPage->tabsNorths->setChecked(QMPSettings.getBool("MainWidget/TabPositionNorth"));

#ifdef QMPLAY2_ALLOW_ONLY_ONE_INSTANCE
        generalSettingsPage->allowOnlyOneInstance->setChecked(QMPSettings.getBool("AllowOnlyOneInstance"));
#else
        delete generalSettingsPage->allowOnlyOneInstance;
        generalSettingsPage->allowOnlyOneInstance = nullptr;
#endif

        generalSettingsPage->hideArtistMetadata->setChecked(QMPSettings.getBool("HideArtistMetadata"));
        generalSettingsPage->displayOnlyFileName->setChecked(QMPSettings.getBool("DisplayOnlyFileName"));
        generalSettingsPage->skipPlaylistsWithinFiles->setChecked(QMPSettings.getBool("SkipPlaylistsWithinFiles"));
        generalSettingsPage->restoreRepeatMode->setChecked(QMPSettings.getBool("RestoreRepeatMode"));
        generalSettingsPage->stillImages->setChecked(QMPSettings.getBool("StillImages"));

        generalSettingsPage->proxyB->setChecked(QMPSettings.getBool("Proxy/Use"));
        generalSettingsPage->proxyHostE->setText(QMPSettings.getString("Proxy/Host"));
        generalSettingsPage->proxyPortB->setValue(QMPSettings.getInt("Proxy/Port"));
        generalSettingsPage->proxyLoginB->setChecked(QMPSettings.getBool("Proxy/Login"));
        generalSettingsPage->proxyUserE->setText(QMPSettings.getString("Proxy/User"));
        generalSettingsPage->proxyPasswordE->setText(QByteArray::fromBase64(QMPSettings.getByteArray("Proxy/Password")));

        const QIcon viewRefresh = QMPlay2Core.getIconFromTheme("view-refresh");
        generalSettingsPage->clearCoversCache->setIcon(viewRefresh);
        connect(generalSettingsPage->clearCoversCache, SIGNAL(clicked()), this, SLOT(clearCoversCache()));
#ifdef USE_YOUTUBEDL
        generalSettingsPage->removeYtDlB->setIcon(QMPlay2Core.getIconFromTheme("list-remove"));
        connect(generalSettingsPage->removeYtDlB, SIGNAL(clicked()), this, SLOT(removeYouTubeDl()));
#else
        generalSettingsPage->removeYtDlB->deleteLater();
        generalSettingsPage->removeYtDlB = nullptr;
#endif
        generalSettingsPage->resetSettingsB->setIcon(viewRefresh);
        connect(generalSettingsPage->resetSettingsB, SIGNAL(clicked()), this, SLOT(resetSettings()));
    }

    {
        const QString modulesListTitle[3] = {
            tr("Legacy video output priority"),
            tr("Audio output priority"),
            tr("Decoders priority")
        };
        for (int m = 0; m < 3; ++m)
        {
            auto groupB = new QGroupBox(modulesListTitle[m]);
            m_modulesListGroupBox[m] = groupB;

            auto ml = new Ui::ModulesList;
            modulesList[m] = ml;
            ml->setupUi(groupB);

            connect(ml->list, SIGNAL(itemDoubleClicked (QListWidgetItem *)), this, SLOT(openModuleSettings(QListWidgetItem *)));
            connect(ml->moveUp, SIGNAL(clicked()), this, SLOT(moveModule()));
            connect(ml->moveDown, SIGNAL(clicked()), this, SLOT(moveModule()));
            ml->moveUp->setProperty("idx", m);
            ml->moveDown->setProperty("idx", m);
        }
    }

    createRendererSettings();

    {
        QWidget *playbackSettingsWidget = new QWidget;
        playbackSettingsPage = new Ui::PlaybackSettings;
        playbackSettingsPage->setupUi(playbackSettingsWidget);

        appendColon(playbackSettingsPage->shortSeekL);
        appendColon(playbackSettingsPage->longSeekL);
        appendColon(playbackSettingsPage->bufferLocalL);
        appendColon(playbackSettingsPage->bufferNetworkL);
        appendColon(playbackSettingsPage->backwardBufferNetworkL);
        appendColon(playbackSettingsPage->playIfBufferedL);
        appendColon(playbackSettingsPage->maxVolL);
        appendColon(playbackSettingsPage->forceSamplerate);
        appendColon(playbackSettingsPage->forceChannels);

        playbackSettingsPage->shortSeekB->setSuffix(" " + playbackSettingsPage->shortSeekB->suffix());
        playbackSettingsPage->longSeekB->setSuffix(" " + playbackSettingsPage->longSeekB->suffix());
        playbackSettingsPage->playIfBufferedB->setSuffix(" " + playbackSettingsPage->playIfBufferedB->suffix());
        playbackSettingsPage->replayGainPreamp->setPrefix(playbackSettingsPage->replayGainPreamp->prefix() + ": ");
        playbackSettingsPage->replayGainPreampNoMetadata->setPrefix(playbackSettingsPage->replayGainPreampNoMetadata->prefix() + ": ");

        tabW->addTab(playbackSettingsWidget, tr("Playback settings"));

        playbackSettingsPage->shortSeekB->setValue(QMPSettings.getInt("ShortSeek"));
        playbackSettingsPage->longSeekB->setValue(QMPSettings.getInt("LongSeek"));
        playbackSettingsPage->bufferLocalB->setValue(QMPSettings.getInt("AVBufferLocal"));
        playbackSettingsPage->bufferNetworkB->setValue(QMPSettings.getInt("AVBufferNetwork"));
        playbackSettingsPage->backwardBufferNetworkB->setCurrentIndex(QMPSettings.getUInt("BackwardBuffer"));
        playbackSettingsPage->playIfBufferedB->setValue(QMPSettings.getDouble("PlayIfBuffered"));
        playbackSettingsPage->maxVolB->setValue(QMPSettings.getInt("MaxVol"));

        playbackSettingsPage->forceSamplerate->setChecked(QMPSettings.getBool("ForceSamplerate"));
        playbackSettingsPage->samplerateB->setValue(QMPSettings.getInt("Samplerate"));
        connect(playbackSettingsPage->forceSamplerate, SIGNAL(toggled(bool)), playbackSettingsPage->samplerateB, SLOT(setEnabled(bool)));
        playbackSettingsPage->samplerateB->setEnabled(playbackSettingsPage->forceSamplerate->isChecked());

        playbackSettingsPage->forceChannels->setChecked(QMPSettings.getBool("ForceChannels"));
        playbackSettingsPage->channelsB->setValue(QMPSettings.getInt("Channels"));
        connect(playbackSettingsPage->forceChannels, SIGNAL(toggled(bool)), playbackSettingsPage->channelsB, SLOT(setEnabled(bool)));
        playbackSettingsPage->channelsB->setEnabled(playbackSettingsPage->forceChannels->isChecked());

        playbackSettingsPage->resamplerFirst->setChecked(QMPSettings.getBool("ResamplerFirst"));

        playbackSettingsPage->replayGain->setChecked(QMPSettings.getBool("ReplayGain/Enabled"));
        playbackSettingsPage->replayGainAlbum->setChecked(QMPSettings.getBool("ReplayGain/Album"));
        playbackSettingsPage->replayGainPreventClipping->setChecked(QMPSettings.getBool("ReplayGain/PreventClipping"));
        playbackSettingsPage->replayGainPreamp->setValue(QMPSettings.getDouble("ReplayGain/Preamp"));
        playbackSettingsPage->replayGainPreampNoMetadata->setValue(QMPSettings.getDouble("ReplayGain/PreampNoMetadata"));

        playbackSettingsPage->wheelActionB->setChecked(QMPSettings.getBool("WheelAction"));
        playbackSettingsPage->wheelSeekB->setChecked(QMPSettings.getBool("WheelSeek"));
        playbackSettingsPage->wheelVolumeB->setChecked(QMPSettings.getBool("WheelVolume"));

        playbackSettingsPage->storeARatioAndZoomB->setChecked(QMPSettings.getBool("StoreARatioAndZoom"));
        connect(playbackSettingsPage->storeARatioAndZoomB, &QCheckBox::toggled, this, [this](bool checked) {
            if (checked)
            {
                playbackSettingsPage->keepZoom->setChecked(true);
                playbackSettingsPage->keepARatio->setChecked(true);
            }
        });

        playbackSettingsPage->keepZoom->setChecked(QMPSettings.getBool("KeepZoom"));
        connect(playbackSettingsPage->keepZoom, &QCheckBox::toggled, this, [this](bool checked) {
            if (!checked && !playbackSettingsPage->keepARatio->isChecked())
            {
                playbackSettingsPage->storeARatioAndZoomB->setChecked(false);
            }
        });

        playbackSettingsPage->keepARatio->setChecked(QMPSettings.getBool("KeepARatio"));
        connect(playbackSettingsPage->keepARatio, &QCheckBox::toggled, this, [this](bool checked) {
            if (!checked && !playbackSettingsPage->keepZoom->isChecked())
            {
                playbackSettingsPage->storeARatioAndZoomB->setChecked(false);
            }
        });

        playbackSettingsPage->showBufferedTimeOnSlider->setChecked(QMPSettings.getBool("ShowBufferedTimeOnSlider"));
        playbackSettingsPage->savePos->setChecked(QMPSettings.getBool("SavePos"));
        playbackSettingsPage->keepSubtitlesDelay->setChecked(QMPSettings.getBool("KeepSubtitlesDelay"));
        playbackSettingsPage->keepSubtitlesScale->setChecked(QMPSettings.getBool("KeepSubtitlesScale"));
        playbackSettingsPage->keepVideoDelay->setChecked(QMPSettings.getBool("KeepVideoDelay"));
        playbackSettingsPage->keepSpeed->setChecked(QMPSettings.getBool("KeepSpeed"));
        playbackSettingsPage->syncVtoA->setChecked(QMPSettings.getBool("SyncVtoA"));
        playbackSettingsPage->silence->setChecked(QMPSettings.getBool("Silence"));
        playbackSettingsPage->restoreVideoEq->setChecked(QMPSettings.getBool("RestoreVideoEqualizer"));
        playbackSettingsPage->ignorePlaybackError->setChecked(QMPSettings.getBool("IgnorePlaybackError"));
        playbackSettingsPage->leftMouseTogglePlay->setCheckState((Qt::CheckState)qBound(0, QMPSettings.getInt("LeftMouseTogglePlay"), 2));
        playbackSettingsPage->middleMouseToggleFullscreen->setChecked(QMPSettings.getBool("MiddleMouseToggleFullscreen"));

        playbackSettingsPage->accurateSeekB->setCheckState((Qt::CheckState)QMPSettings.getInt("AccurateSeek"));
        playbackSettingsPage->accurateSeekB->setToolTip(tr("Slower, but more accurate seeking.\nPartially checked doesn't affect seeking on slider."));

        playbackSettingsPage->unpauseWhenSeekingB->setChecked(QMPSettings.getBool("UnpauseWhenSeeking"));

        playbackSettingsPage->restoreAVSStateB->setChecked(QMPSettings.getBool("RestoreAVSState"));
        playbackSettingsPage->disableSubtitlesAtStartup->setChecked(QMPSettings.getBool("DisableSubtitlesAtStartup"));

        for (int m = 1; m < 3; ++m)
            playbackSettingsPage->modulesListLayout->addWidget(m_modulesListGroupBox[m]);
    }

    {
        page3 = new Page3;
        tabW->addTab(page3, tr("Modules"));

        page3->listW = new QListWidget;
        page3->listW->setIconSize({32, 32});
        page3->listW->setMinimumSize(200, 0);
        page3->listW->setMaximumSize(200, 16777215);
        for (Module *module : QMPlay2Core.getPluginsInstance())
        {
            QListWidgetItem *tWI = new QListWidgetItem(module->name());
            tWI->setData(Qt::UserRole, QVariant::fromValue((void *)module));
            QString toolTip = "<html>" + tr("Contains") + ":";
            for (const Module::Info &mod : module->getModulesInfo(true))
            {
                const QPixmap moduleIcon = Functions::getPixmapFromIcon(mod.icon, QSize(22, 22), this);
                toolTip += "<p>&nbsp;&nbsp;&nbsp;&nbsp;";
                bool hasIcon = false;
                if (!moduleIcon.isNull())
                {
                    QBuffer buffer;
                    if (buffer.open(QBuffer::WriteOnly) && moduleIcon.save(&buffer, "PNG"))
                    {
                        toolTip += "<img width='22' height='22' src='data:image/png;base64, " + buffer.data().toBase64() + "'/> ";
                        hasIcon = true;
                    }
                }
                if (!hasIcon)
                    toolTip += "- ";
                toolTip += mod.name + "</p>";
            }
            toolTip += "</html>";
            tWI->setToolTip(toolTip);
            tWI->setIcon(QMPlay2GUI.getIcon(module->icon()));
            page3->listW->addItem(tWI);
            if (page == 3 && !moduleName.isEmpty() && module->name() == moduleName)
                moduleIndex = page3->listW->count() - 1;
        }

        page3->scrollA = new QScrollArea;
        page3->scrollA->setWidgetResizable(true);
        page3->scrollA->setFrameShape(QFrame::NoFrame);

        QHBoxLayout *layout = new QHBoxLayout(page3);
        layout->setMargin(0);
        layout->setSpacing(1);
        layout->addWidget(page3->listW);
        layout->addWidget(page3->scrollA);
        connect(page3->listW, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(chModule(QListWidgetItem *)));
    }

    {
        page4 = new Page4;
        tabW->addTab(page4, tr("Subtitles"));

        page4->colorsAndBordersB = new QCheckBox(tr("Colors and borders"));
        page4->colorsAndBordersB->setChecked(QMPSettings.getBool("ApplyToASS/ColorsAndBorders"));

        page4->marginsAndAlignmentB = new QCheckBox(tr("Margins and alignment"));
        page4->marginsAndAlignmentB->setChecked(QMPSettings.getBool("ApplyToASS/MarginsAndAlignment"));

        page4->fontsB = new QCheckBox(tr("Fonts and spacing"));
        page4->fontsB->setChecked(QMPSettings.getBool("ApplyToASS/FontsAndSpacing"));

        page4->overridePlayResB = new QCheckBox(tr("Use the same size"));
        page4->overridePlayResB->setChecked(QMPSettings.getBool("ApplyToASS/OverridePlayRes"));

        page4->toAssGB = new QGroupBox(tr("Apply for ASS/SSA subtitles"));
        page4->toAssGB->setCheckable(true);
        page4->toAssGB->setChecked(QMPSettings.getBool("ApplyToASS/ApplyToASS"));

        QGridLayout *page4ToAssLayout = new QGridLayout(page4->toAssGB);
        page4ToAssLayout->addWidget(page4->colorsAndBordersB, 0, 0, 1, 1);
        page4ToAssLayout->addWidget(page4->marginsAndAlignmentB, 1, 0, 1, 1);
        page4ToAssLayout->addWidget(page4->fontsB, 0, 1, 1, 1);
        page4ToAssLayout->addWidget(page4->overridePlayResB, 1, 1, 1, 1);

        page4->addWidget(page4->toAssGB);
    }

    {
        page5 = new Page5;
        tabW->addTab(page5, tr("OSD"));

        page5->enabledB = new QCheckBox(tr("OSD enabled"));
        page5->enabledB->setChecked(QMPSettings.getBool("OSD/Enabled"));
        page5->addWidget(page5->enabledB);
    }

    {
        page6 = new Page6;
        tabW->addTab(page6, tr("Video filters"));

        QWidget *widget = new QWidget;

        QGridLayout *layout = new QGridLayout(widget);
        layout->setMargin(0);

        page6->deintSettingsW = new DeintSettingsW;
        layout->addWidget(page6->deintSettingsW, 0, 0, 1, 2);

        page6->videoEqContainer = new QGroupBox(videoEq->objectName());
        layout->addWidget(page6->videoEqContainer, 1, 0, 2, 1);

        page6->otherVFiltersW = new OtherVFiltersW(false);
        if (!page6->otherVFiltersW->count())
        {
            delete page6->otherVFiltersW;
            page6->otherVFiltersW = nullptr;
        }
        else
        {
            QGroupBox *otherVFiltersContainer = new QGroupBox(tr("Software video filters"));
            QGridLayout *otherVFiltersLayout = new QGridLayout(otherVFiltersContainer);
            otherVFiltersLayout->addWidget(page6->otherVFiltersW);
            connect(page6->otherVFiltersW, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(openModuleSettings(QListWidgetItem *)));
            layout->addWidget(otherVFiltersContainer, 1, 1, 1, 1);
        }

        OtherVFiltersW *otherHWVFiltersW = new OtherVFiltersW(true);
        if (!otherHWVFiltersW->count())
            delete otherHWVFiltersW;
        else
        {
            QGroupBox *otherHWVFiltersContainer = new QGroupBox(tr("Hardware accelerated video filters"));
            QGridLayout *otherHWVFiltersLayout = new QGridLayout(otherHWVFiltersContainer);
            otherHWVFiltersLayout->addWidget(otherHWVFiltersW);
            connect(otherHWVFiltersW, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(openModuleSettings(QListWidgetItem *)));
            layout->addWidget(otherHWVFiltersContainer, 2, 1, 1, 1);
        }

        page6->setWidget(widget);
    }

    connect(tabW, SIGNAL(currentChanged(int)), this, SLOT(tabCh(int)));
    tabW->setCurrentIndex(page);

    show();
}
SettingsWidget::~SettingsWidget()
{
    for (int i = 0; i < 3; ++i)
        delete modulesList[i];
    delete playbackSettingsPage;
    delete generalSettingsPage;
}

void SettingsWidget::setAudioChannels()
{
    playbackSettingsPage->forceChannels->setChecked(QMPlay2Core.getSettings().getBool("ForceChannels"));
    playbackSettingsPage->channelsB->setValue(QMPlay2Core.getSettings().getInt("Channels"));
}

void SettingsWidget::applyProxy()
{
    Settings &QMPSettings = QMPlay2Core.getSettings();
    if (!QMPSettings.getBool("Proxy/Use"))
    {
        qunsetenv("http_proxy");
    }
    else
    {
        const QString proxyHostName = QMPSettings.getString("Proxy/Host");
        const quint16 proxyPort = QMPSettings.getInt("Proxy/Port");
        QString proxyUser, proxyPassword;
        if (QMPSettings.getBool("Proxy/Login"))
        {
            proxyUser = QMPSettings.getString("Proxy/User");
            proxyPassword = QByteArray::fromBase64(QMPSettings.getByteArray("Proxy/Password"));
        }

        /* Proxy env for FFmpeg */
        QString proxyEnv = QString("http://" + proxyHostName + ':' + QString::number(proxyPort));
        if (!proxyUser.isEmpty())
        {
            QString auth = proxyUser;
            if (!proxyPassword.isEmpty())
                auth += ':' + proxyPassword;
            auth += '@';
            proxyEnv.insert(7, auth);
        }
        qputenv("http_proxy", proxyEnv.toLocal8Bit());
    }
}

void SettingsWidget::createRendererSettings()
{
    const auto currentRendererName = QMPlay2Core.rendererName();
    auto settings = &QMPlay2Core.getSettings();

    auto renderers = new QComboBox;
    renderers->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    renderers->addItem(tr("Legacy"), "legacy");
#ifdef USE_OPENGL
    renderers->addItem(tr("OpenGL"), "opengl");
#endif
#ifdef USE_VULKAN
    renderers->addItem(tr("Vulkan"), "vulkan");
#endif

    const auto chosenRenderer = settings->getString("Renderer");
    int activeRenderer = -1;

    for (int i = renderers->count() - 1; i >= 0; --i)
    {
        const bool active = (renderers->itemData(i).toString() == currentRendererName);
        if (active)
            activeRenderer = i;
        renderers->setItemText(i, renderers->itemText(i) + " (" + (active ? tr("active") : tr("inactive")) + ")");
    }

    auto rendererStacked = new QStackedWidget;

    m_rendererApplyFunctions.push_back([=](bool &initFilters) {
        Q_UNUSED(initFilters)
        const auto rendererName = renderers->currentData().toString();
        settings->set("Renderer", rendererName);
        if (currentRendererName != rendererName && chosenRenderer != rendererName)
        {
            QMessageBox::information(this, tr("Changing renderer"), tr("To set up a new renderer, the program will start again!"));
            restartApp();
        }
    });

#if defined(USE_OPENGL) || defined(USE_VULKAN)
    auto createVSync = [] {
        return new QCheckBox(tr("Vertical synchronization (V-Sync)"));
    };
    auto createBypassCompositor = [] {
        auto bypassCompositor = new QCheckBox(tr("Bypass compositor in full screen"));
        if (QGuiApplication::platformName() == "xcb")
        {
            bypassCompositor->setToolTip(tr("This can improve performance if X11 compositor supports it"));
        }
        return bypassCompositor;
    };
#endif

    {
        auto legacySetttings = new QWidget;

        auto layout = new QFormLayout(legacySetttings);
        layout->setMargin(3);
        layout->addRow(new QLabel(tr(
            "Use QMPlay2 video output modules. "
            "This will also be used if other renderers aren't available."
        )));

        rendererStacked->addWidget(legacySetttings);
    }

#ifdef USE_OPENGL
    {
        auto openglSettings = new QWidget;

        auto glOnWindow = new QCheckBox(tr("Use OpenGL on entire window"));
        auto vsync = createVSync();
        auto bypassCompositor = createBypassCompositor();

#ifdef Q_OS_WIN
        if (QSysInfo::windowsVersion() >= QSysInfo::WV_6_0)
        {
            bypassCompositor->setToolTip(tr("This can improve performance. Some video drivers can crash when enabled."));
        }
        else
#endif
        if (QGuiApplication::platformName() != "xcb")
        {
            bypassCompositor->setEnabled(false);
        }

        glOnWindow->setToolTip(tr(
            "Use QOpenGLWidget (render-to-texture), also enable OpenGL for visualizations. "
            "Use with caution, it can reduce performance of video playback."
        ));

        connect(glOnWindow, &QCheckBox::toggled,
                this, [=](bool checked) {
            vsync->setEnabled(!checked);
#ifdef Q_OS_WIN
            if (QSysInfo::windowsVersion() >= QSysInfo::WV_6_0)
            {
                bypassCompositor->setEnabled(!checked);
            }
#endif
        });

        glOnWindow->setChecked(settings->getBool("OpenGL/OnWindow"));
        vsync->setChecked(settings->getBool("OpenGL/VSync"));
        bypassCompositor->setChecked(settings->getBool("OpenGL/BypassCompositor"));

        auto layout = new QFormLayout(openglSettings);
        layout->setMargin(3);
        layout->addRow(glOnWindow);
        layout->addRow(vsync);
        layout->addRow(bypassCompositor);

        if (QMPlay2CoreClass::isGlOnWindowForced())
        {
            glOnWindow->hide();
            vsync->hide();
        }

        m_rendererApplyFunctions.push_back([=](bool &initFilters) {
            Q_UNUSED(initFilters)
            auto &settings = QMPlay2Core.getSettings();
            if (!glOnWindow->isHidden() && settings.getBool("OpenGL/OnWindow") != glOnWindow->isChecked())
            {
                settings.set("OpenGL/OnWindow", glOnWindow->isChecked());
                if (!QMPlay2GUI.restartApp)
                {
                    QMessageBox::information(this, tr("Changing OpenGL mode"), tr("To set up a new OpenGL mode, the program will start again!"));
                    restartApp();
                }
            }
            if (!vsync->isHidden())
                settings.set("OpenGL/VSync", vsync->isChecked());
            if (!bypassCompositor->isHidden())
                settings.set("OpenGL/BypassCompositor", bypassCompositor->isChecked());
        });

        rendererStacked->addWidget(openglSettings);
    }
#endif

#ifdef USE_VULKAN
    {
        auto vulkanSetttings = new QWidget;

        auto devices = new QComboBox;
        auto vsync = createVSync();
        auto gpuDeint = new QCheckBox(tr("Use GPU deinterlacing for CPU-decoded video"));
        auto forceYadif = new QCheckBox(tr("Force Vulkan Yadif deinterlacing for all hardware decoders"));
        auto hqDownscale = new QCheckBox(tr("High quality image scaling down"));
        auto hqUpscale = new QCheckBox(tr("High quality image scaling up"));
        auto bypassCompositor = createBypassCompositor();

#ifdef Q_OS_WIN
        auto noExclusiveFullScreenDevIDs = std::make_shared<QSet<QByteArray>>();
#endif
        auto noFiltersDevIDs = std::make_shared<QSet<QByteArray>>();

        connect(devices, qOverload<int>(&QComboBox::currentIndexChanged),
                this, [=](int idx) {
            if (devices->count() <= 1)
                return;
            if (idx == 0)
                idx = 1;

            const bool filtersEnabled = !noFiltersDevIDs->contains(devices->itemData(idx).toByteArray());
            gpuDeint->setEnabled(filtersEnabled);
            forceYadif->setEnabled(filtersEnabled);

#ifdef Q_OS_WIN
            bypassCompositor->setEnabled(!noExclusiveFullScreenDevIDs->contains(devices->itemData(idx).toByteArray()));
#endif
        });

        connect(rendererStacked, &QStackedWidget::currentChanged,
                this, [=](int idx) {
            if (devices->count() > 0 || idx != renderers->findData("vulkan"))
                return;

            const auto storedID = settings->getByteArray("Vulkan/Device");
            int idIdx = 0;

            devices->blockSignals(true);
            for (auto &&physicalDevice : QmVk::Instance::enumerateSupportedPhysicalDevices())
            {
                const auto &properties = physicalDevice->properties();
                const auto id = QmVk::Instance::getPhysicalDeviceID(properties);
#ifdef Q_OS_WIN
                if (bypassCompositor->isEnabled())
                {
                    if (!physicalDevice->checkExtension(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME))
                        noExclusiveFullScreenDevIDs->insert(id);
                }
#endif
                if (!QmVk::Instance::checkFiltersSupported(physicalDevice))
                    noFiltersDevIDs->insert(id);
                devices->addItem(static_cast<const char *>(properties.deviceName), id);
                if (idIdx == 0 && !storedID.isEmpty() && storedID == id)
                    idIdx = devices->count();
            }
            devices->setCurrentIndex(-1);
            devices->blockSignals(false);

            if (devices->count() > 0)
            {
                devices->insertItem(0, tr("First available device"));
                devices->setCurrentIndex(idIdx);
            }
            else
            {
                devices->addItem(tr("No supported devices found"));
                vulkanSetttings->setEnabled(false);
            }
        });

        vsync->setTristate(true);
        vsync->setToolTip(tr(
            "Partially checked (default):\n"
            "  - MAILBOX (tear-free) is the preferred present mode\n"
            "  - FIFO (V-Sync) should not be used in windowed mode"
        ));

#ifdef Q_OS_WIN
        if (QSysInfo::windowsVersion() >= QSysInfo::WV_6_0)
        {
            bypassCompositor->setToolTip(tr("Allow for exclusive fullscreen. This can improve performance."));
        }
        else
#endif
        if (QGuiApplication::platformName() != "xcb")
        {
            bypassCompositor->setEnabled(false);
        }

        vsync->setCheckState(settings->getWithBounds("Vulkan/VSync", Qt::Unchecked, Qt::Checked));
        gpuDeint->setChecked(settings->getBool("Vulkan/AlwaysGPUDeint"));
        forceYadif->setChecked(settings->getBool("Vulkan/ForceVulkanYadif"));
        hqDownscale->setChecked(settings->getBool("Vulkan/HQScaleDown"));
        hqUpscale->setChecked(settings->getBool("Vulkan/HQScaleUp"));
        bypassCompositor->setChecked(settings->getBool("Vulkan/BypassCompositor"));

        hqUpscale->setToolTip(tr("Very slow if used with sharpness"));

        auto layout = new QFormLayout(vulkanSetttings);
        layout->setMargin(3);
        layout->addRow(tr("Device:"), devices);
        layout->addRow(vsync);
        layout->addRow(gpuDeint);
        layout->addRow(forceYadif);
        layout->addRow(hqDownscale);
        layout->addRow(hqUpscale);
        layout->addRow(bypassCompositor);

        m_rendererApplyFunctions.push_back([=](bool &initFilters) {
            const bool alwaysGPUDeint = gpuDeint->isChecked();
            if (QMPlay2Core.isVulkanRenderer() && settings->getBool("Vulkan/AlwaysGPUDeint") != alwaysGPUDeint)
                initFilters = true;

            if (devices->isEnabled() && devices->count() > 0)
            {
                const auto vulkanDeviceID = devices->currentData().toByteArray();
                if (vulkanDeviceID != settings->getByteArray("Vulkan/Device"))
                {
                    settings->set("Vulkan/Device", vulkanDeviceID.constData());
                    if (QMPlay2Core.isVulkanRenderer())
                        std::static_pointer_cast<QmVk::Instance>(QMPlay2Core.gpuInstance())->obtainPhysicalDevice();
                }
            }
            settings->set("Vulkan/VSync", vsync->checkState());
            settings->set("Vulkan/AlwaysGPUDeint", alwaysGPUDeint);
            settings->set("Vulkan/ForceVulkanYadif", forceYadif->isChecked());
            settings->set("Vulkan/HQScaleDown", hqDownscale->isChecked());
            settings->set("Vulkan/HQScaleUp", hqUpscale->isChecked());
            settings->set("Vulkan/BypassCompositor", bypassCompositor->isChecked());
        });

        rendererStacked->addWidget(vulkanSetttings);
    }
#endif

    connect(renderers, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [=](int idx) {
        rendererStacked->setCurrentIndex(idx);
    });

    if (activeRenderer > -1)
    {
        m_setRenderersCurrentIndexFn = [=] {
            renderers->setCurrentIndex(activeRenderer);
            m_setRenderersCurrentIndexFn = nullptr;
        };
    }

    auto widget = new QWidget;
    auto layout = new QGridLayout(widget);
    layout->setMargin(1);
    layout->addWidget(new QLabel(tr("Renderer:")), 0, 0);
    layout->addWidget(renderers, 0, 1);
    layout->addWidget(rendererStacked, 1, 0, 1, 2);
    layout->addWidget(m_modulesListGroupBox[0], 2, 0, 1, 2);

    auto scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(widget);
    tabW->addTab(scrollArea, tr("Renderer settings"));
}

void SettingsWidget::restoreVideoEq()
{
    if (videoEq->parentWidget() != videoEqOriginalParent)
    {
        videoEq->setParent(videoEqOriginalParent);
        videoEq->setGeometry(videoEqOriginalParent->rect());
        videoEqOriginalParent->setEnabled(true);
    }
    if (QLayout *videoEqLayout = page6->videoEqContainer->layout())
        videoEqLayout->deleteLater();
}

void SettingsWidget::restartApp()
{
    QMPlay2GUI.restartApp = true;
    close();
    QMPlay2GUI.mainW->close();
}

inline QString SettingsWidget::getSelectedProfile()
{
    return !generalSettingsPage->profileB->currentIndex() ? "/" : generalSettingsPage->profileB->currentText();
}

void SettingsWidget::showEvent(QShowEvent *)
{
    if (!wasShow)
    {
        QMPlay2GUI.restoreGeometry("SettingsWidget/Geometry", this, 65);
        page3->listW->setCurrentRow(moduleIndex);
        wasShow = true;
    }
}
void SettingsWidget::closeEvent(QCloseEvent *)
{
    QMPlay2Core.getSettings().set("SettingsWidget/Geometry", geometry());
    restoreVideoEq();
}

void SettingsWidget::chStyle()
{
    const QString newStyle = generalSettingsPage->styleBox->currentText().toLower();
    if (QApplication::style()->objectName() != newStyle)
    {
        QMPlay2Core.getSettings().set("Style", newStyle);
        QMPlay2GUI.setStyle();
    }
}

void SettingsWidget::apply()
{
    Settings &QMPSettings = QMPlay2Core.getSettings();
    const int page = tabW->currentIndex();
    bool forceRestartPlayback = false;
    bool initFilters = false;
    switch (page)
    {
        case 0:
        {
            if (QMPlay2Core.getLanguage() != generalSettingsPage->langBox->itemData(generalSettingsPage->langBox->currentIndex()).toString())
            {
                QMPSettings.set("Language", generalSettingsPage->langBox->itemData(generalSettingsPage->langBox->currentIndex()).toString());
                QMPlay2Core.setLanguage();
                QMessageBox::information(this, tr("New language"), tr("To set up a new language, the program will start again!"));
                restartApp();
            }
#ifdef ICONS_FROM_THEME
            if (generalSettingsPage->iconsFromTheme->isChecked() != QMPSettings.getBool("IconsFromTheme"))
            {
                QMPSettings.set("IconsFromTheme", generalSettingsPage->iconsFromTheme->isChecked());
                if (!QMPlay2GUI.restartApp)
                {
                    QMessageBox::information(this, tr("Changing icons"), tr("To apply the icons change, the program will start again!"));
                    restartApp();
                }
            }
#endif
            QMPSettings.set("FallbackSubtitlesEncoding", generalSettingsPage->encodingB->currentText().toUtf8());
            QMPSettings.set("AudioLanguage", generalSettingsPage->audioLangB->currentIndex() > 0 ? generalSettingsPage->audioLangB->currentText() : QString());
            QMPSettings.set("SubtitlesLanguage", generalSettingsPage->subsLangB->currentIndex() > 0 ? generalSettingsPage->subsLangB->currentText() : QString());
            QMPSettings.set("screenshotPth", generalSettingsPage->screenshotE->text());
            QMPSettings.set("screenshotFormat", generalSettingsPage->screenshotFormatB->currentText());
            QMPSettings.set("ShowCovers", generalSettingsPage->showCoversGB->isChecked());
            QMPSettings.set("BlurCovers", generalSettingsPage->blurCoversB->isChecked());
            QMPSettings.set("ShowDirCovers", generalSettingsPage->showDirCoversB->isChecked());
            QMPSettings.set("EnlargeCovers", generalSettingsPage->enlargeSmallCoversB->isChecked());
            QMPSettings.set("AutoOpenVideoWindow", generalSettingsPage->autoOpenVideoWindowB->isChecked());
            QMPSettings.set("AutoRestoreMainWindowOnVideo", generalSettingsPage->autoRestoreMainWindowOnVideoB->isChecked());
#ifdef UPDATES
            QMPSettings.set("AutoUpdates", generalSettingsPage->autoUpdatesB->isChecked());
#endif
            QMPSettings.set("MainWidget/TabPositionNorth", generalSettingsPage->tabsNorths->isChecked());
#ifdef QMPLAY2_ALLOW_ONLY_ONE_INSTANCE
            QMPSettings.set("AllowOnlyOneInstance", generalSettingsPage->allowOnlyOneInstance->isChecked());
#endif
            QMPSettings.set("HideArtistMetadata", generalSettingsPage->hideArtistMetadata->isChecked());
            QMPSettings.set("DisplayOnlyFileName", generalSettingsPage->displayOnlyFileName->isChecked());
            QMPSettings.set("SkipPlaylistsWithinFiles", generalSettingsPage->skipPlaylistsWithinFiles->isChecked());
            QMPSettings.set("RestoreRepeatMode", generalSettingsPage->restoreRepeatMode->isChecked());
            QMPSettings.set("StillImages", generalSettingsPage->stillImages->isChecked());
            if (generalSettingsPage->trayNotifiesDefault)
                QMPSettings.set("TrayNotifiesDefault", generalSettingsPage->trayNotifiesDefault->isChecked());
            QMPSettings.set("AutoDelNonGroupEntries", generalSettingsPage->autoDelNonGroupEntries->isChecked());
            QMPSettings.set("Proxy/Use", generalSettingsPage->proxyB->isChecked() && !generalSettingsPage->proxyHostE->text().isEmpty());
            QMPSettings.set("Proxy/Host", generalSettingsPage->proxyHostE->text());
            QMPSettings.set("Proxy/Port", generalSettingsPage->proxyPortB->value());
            QMPSettings.set("Proxy/Login", generalSettingsPage->proxyLoginB->isChecked() && !generalSettingsPage->proxyUserE->text().isEmpty());
            QMPSettings.set("Proxy/User", generalSettingsPage->proxyUserE->text());
            QMPSettings.set("Proxy/Password", generalSettingsPage->proxyPasswordE->text().toUtf8().toBase64());
            generalSettingsPage->proxyB->setChecked(QMPSettings.getBool("Proxy/Use"));
            generalSettingsPage->proxyLoginB->setChecked(QMPSettings.getBool("Proxy/Login"));
            qobject_cast<QMainWindow *>(QMPlay2GUI.mainW)->setTabPosition(Qt::AllDockWidgetAreas, generalSettingsPage->tabsNorths->isChecked() ? QTabWidget::North : QTabWidget::South);
            applyProxy();

            if (generalSettingsPage->trayNotifiesDefault)
                Notifies::setNativeFirst(!generalSettingsPage->trayNotifiesDefault->isChecked());

            QSettings profileSettings(QMPlay2Core.getSettingsDir() + "Profile.ini", QSettings::IniFormat);
            const QString selectedProfile = getSelectedProfile();
            if (selectedProfile != profileSettings.value("Profile", "/").toString())
            {
                profileSettings.setValue("Profile", selectedProfile);
                restartApp();
            }
        } break;
        case 1:
        {
            for (auto &&fn : m_rendererApplyFunctions)
                fn(initFilters);

            QStringList videoWriters;
            for (QListWidgetItem *wI : modulesList[0]->list->findItems(QString(), Qt::MatchContains))
                videoWriters += wI->text();
            QMPSettings.set("videoWriters", videoWriters);

            if (initFilters)
                page6->deintSettingsW->setSoftwareDeintEnabledDisabled();

            break;
        }
        case 2:
        {
            QMPSettings.set("ShortSeek", playbackSettingsPage->shortSeekB->value());
            QMPSettings.set("LongSeek", playbackSettingsPage->longSeekB->value());
            QMPSettings.set("AVBufferLocal", playbackSettingsPage->bufferLocalB->value());
            QMPSettings.set("AVBufferNetwork", playbackSettingsPage->bufferNetworkB->value());
            QMPSettings.set("BackwardBuffer", playbackSettingsPage->backwardBufferNetworkB->currentIndex());
            QMPSettings.set("PlayIfBuffered", playbackSettingsPage->playIfBufferedB->value());
            QMPSettings.set("ForceSamplerate", playbackSettingsPage->forceSamplerate->isChecked());
            QMPSettings.set("Samplerate", playbackSettingsPage->samplerateB->value());
            QMPSettings.set("MaxVol", playbackSettingsPage->maxVolB->value());
            QMPSettings.set("ForceChannels", playbackSettingsPage->forceChannels->isChecked());
            QMPSettings.set("Channels", playbackSettingsPage->channelsB->value());
            QMPSettings.set("ResamplerFirst", playbackSettingsPage->resamplerFirst->isChecked());
            QMPSettings.set("ReplayGain/Enabled", playbackSettingsPage->replayGain->isChecked());
            QMPSettings.set("ReplayGain/Album", playbackSettingsPage->replayGainAlbum->isChecked());
            QMPSettings.set("ReplayGain/PreventClipping", playbackSettingsPage->replayGainPreventClipping->isChecked());
            QMPSettings.set("ReplayGain/Preamp", playbackSettingsPage->replayGainPreamp->value());
            QMPSettings.set("ReplayGain/PreampNoMetadata", playbackSettingsPage->replayGainPreampNoMetadata->value());
            QMPSettings.set("WheelAction", playbackSettingsPage->wheelActionB->isChecked());
            QMPSettings.set("WheelSeek", playbackSettingsPage->wheelSeekB->isChecked());
            QMPSettings.set("WheelVolume", playbackSettingsPage->wheelVolumeB->isChecked());
            QMPSettings.set("ShowBufferedTimeOnSlider", playbackSettingsPage->showBufferedTimeOnSlider->isChecked());
            QMPSettings.set("SavePos", playbackSettingsPage->savePos->isChecked());
            QMPSettings.set("KeepZoom", playbackSettingsPage->keepZoom->isChecked());
            QMPSettings.set("KeepARatio", playbackSettingsPage->keepARatio->isChecked());
            QMPSettings.set("KeepSubtitlesDelay", playbackSettingsPage->keepSubtitlesDelay->isChecked());
            QMPSettings.set("KeepSubtitlesScale", playbackSettingsPage->keepSubtitlesScale->isChecked());
            QMPSettings.set("KeepVideoDelay", playbackSettingsPage->keepVideoDelay->isChecked());
            QMPSettings.set("KeepSpeed", playbackSettingsPage->keepSpeed->isChecked());
            QMPSettings.set("SyncVtoA", playbackSettingsPage->syncVtoA->isChecked());
            QMPSettings.set("Silence", playbackSettingsPage->silence->isChecked());
            QMPSettings.set("RestoreVideoEqualizer", playbackSettingsPage->restoreVideoEq->isChecked());
            QMPSettings.set("IgnorePlaybackError", playbackSettingsPage->ignorePlaybackError->isChecked());
            QMPSettings.set("LeftMouseTogglePlay", playbackSettingsPage->leftMouseTogglePlay->checkState());
            QMPSettings.set("MiddleMouseToggleFullscreen", playbackSettingsPage->middleMouseToggleFullscreen->isChecked());
            QMPSettings.set("AccurateSeek", playbackSettingsPage->accurateSeekB->checkState());
            QMPSettings.set("UnpauseWhenSeeking", playbackSettingsPage->unpauseWhenSeekingB->isChecked());
            QMPSettings.set("RestoreAVSState", playbackSettingsPage->restoreAVSStateB->isChecked());
            QMPSettings.set("DisableSubtitlesAtStartup", playbackSettingsPage->disableSubtitlesAtStartup->isChecked());
            QMPSettings.set("StoreARatioAndZoom", playbackSettingsPage->storeARatioAndZoomB->isChecked());

            QStringList audioWriters, decoders;
            for (QListWidgetItem *wI : modulesList[1]->list->findItems(QString(), Qt::MatchContains))
                audioWriters += wI->text();
            for (QListWidgetItem *wI : modulesList[2]->list->findItems(QString(), Qt::MatchContains))
                decoders += wI->text();
            QMPSettings.set("audioWriters", audioWriters);
            QMPSettings.set("decoders", decoders);

            emit setWheelStep(playbackSettingsPage->shortSeekB->value());
            emit setVolMax(playbackSettingsPage->maxVolB->value());

            SetAudioChannelsMenu();
        } break;
        case 3:
            if (page3->module && page3->scrollA->widget())
            {
                Module::SettingsWidget *settingsWidget = (Module::SettingsWidget *)page3->scrollA->widget();
                settingsWidget->saveSettings();
                settingsWidget->flushSettings();
                page3->module->setInstances(forceRestartPlayback);
                chModule(page3->listW->currentItem());
            }
            break;
        case 4:
            page4->writeSettings();
            QMPSettings.set("ApplyToASS/ColorsAndBorders", page4->colorsAndBordersB->isChecked());
            QMPSettings.set("ApplyToASS/MarginsAndAlignment", page4->marginsAndAlignmentB->isChecked());
            QMPSettings.set("ApplyToASS/FontsAndSpacing", page4->fontsB->isChecked());
            QMPSettings.set("ApplyToASS/OverridePlayRes", page4->overridePlayResB->isChecked());
            QMPSettings.set("ApplyToASS/ApplyToASS", page4->toAssGB->isChecked());
            break;
        case 5:
            QMPSettings.set("OSD/Enabled", page5->enabledB->isChecked());
            page5->writeSettings();
            break;
        case 6:
            page6->deintSettingsW->writeSettings();
            if (page6->otherVFiltersW)
                page6->otherVFiltersW->writeSettings();
            initFilters = true;
            break;
    }
    if (page != 3)
        QMPSettings.flush();
    if (!QMPlay2GUI.restartApp)
        emit settingsChanged(page, forceRestartPlayback, initFilters);
}
void SettingsWidget::chModule(QListWidgetItem *w)
{
    if (w)
    {
        page3->module = (Module *)w->data(Qt::UserRole).value<void *>();
        QWidget *w = page3->module->getSettingsWidget();
        if (w)
        {
            QLayout *layout = w->layout();
            layout->setMargin(2);
            if (QFormLayout *fLayout = qobject_cast<QFormLayout *>(layout))
                fLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
            else if (QGridLayout *gLayout = qobject_cast<QGridLayout *>(layout))
            {
                if (!gLayout->property("NoVHSpacer").toBool())
                {
                    gLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), gLayout->rowCount(), 0); //vSpacer
                    gLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, gLayout->columnCount()); //hSpacer
                }
            }
            page3->scrollA->setWidget(w);
            w->setAutoFillBackground(false);
        }
        else if (page3->scrollA->widget())
            page3->scrollA->widget()->close(); //ustawi się na nullptr po usunięciu (WA_DeleteOnClose)
    }
}
void SettingsWidget::tabCh(int idx)
{
    if (idx == 1 && m_setRenderersCurrentIndexFn)
        m_setRenderersCurrentIndexFn();

    if ((idx == 1 || idx == 2) && !modulesList[0]->list->count() && !modulesList[1]->list->count() && !modulesList[2]->list->count())
    {
        const QStringList writers[3] = {QMPlay2Core.getModules("videoWriters", 5), QMPlay2Core.getModules("audioWriters", 5), QMPlay2Core.getModules("decoders", 7)};
        QVector<QPair<Module *, Module::Info>> pluginsInstances[3];
        for (int m = 0; m < 3; ++m)
            pluginsInstances[m].fill(QPair<Module *, Module::Info >(), writers[m].size());
        for (Module *module : QMPlay2Core.getPluginsInstance())
            for (const Module::Info &moduleInfo : module->getModulesInfo())
                for (int m = 0; m < 3; ++m)
                {
                    const int mIdx = writers[m].indexOf(moduleInfo.name);
                    if (mIdx > -1)
                        pluginsInstances[m][mIdx] = {module, moduleInfo};
                }
        for (int m = 0; m < 3; ++m)
        {
            for (int i = 0; i < pluginsInstances[m].size(); i++)
            {
                QListWidgetItem *wI = new QListWidgetItem(writers[m][i]);
                wI->setData(Qt::UserRole, pluginsInstances[m][i].first->name());
                wI->setIcon(QMPlay2GUI.getIcon(pluginsInstances[m][i].second.icon.isNull() ? pluginsInstances[m][i].first->icon() : pluginsInstances[m][i].second.icon));
                modulesList[m]->list->addItem(wI);
                if (writers[m][i] == lastM[m])
                    modulesList[m]->list->setCurrentItem(wI);
            }
            if (modulesList[m]->list->currentRow() < 0)
                modulesList[m]->list->setCurrentRow(0);
        }
    }
    else if (idx == 3)
    {
        for (int m = 0; m < 3; ++m)
        {
            QListWidgetItem *currI = modulesList[m]->list->currentItem();
            if (currI)
                lastM[m] = currI->text();
            modulesList[m]->list->clear();
        }
    }

    if (idx != 6)
    {
        restoreVideoEq();
    }
    else
    {
        QGridLayout *videoEqLayout = new QGridLayout(page6->videoEqContainer);
        videoEqLayout->setMargin(0);
        videoEqLayout->addWidget(videoEq);
        videoEqOriginalParent->setDisabled(true);
        videoEq->show();
    }
}
void SettingsWidget::openModuleSettings(QListWidgetItem *wI)
{
    const QList<QListWidgetItem *> items = page3->listW->findItems(wI->data(Qt::UserRole).toString(), Qt::MatchExactly);
    if (!items.isEmpty())
    {
        page3->listW->setCurrentItem(items[0]);
        tabW->setCurrentIndex(3);
    }
}
void SettingsWidget::moveModule()
{
    if (QToolButton *tB = qobject_cast<QToolButton *>(sender()))
    {
        const bool moveDown = tB->arrowType() == Qt::DownArrow;
        const int idx = tB->property("idx").toInt();
        QListWidget *mL = modulesList[idx]->list;
        int row = mL->currentRow();
        if (row > -1)
        {
            QListWidgetItem *item = mL->takeItem(row);
            mL->clearSelection();
            if (moveDown)
                ++row;
            else
                --row;
            mL->insertItem(row, item);
            mL->setCurrentItem(item);
        }
    }
}
void SettingsWidget::chooseScreenshotDir()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Choose directory"), generalSettingsPage->screenshotE->text());
    if (!dir.isEmpty())
        generalSettingsPage->screenshotE->setText(dir);
}
void SettingsWidget::setAppearance()
{
    Appearance(this).exec();
}
void SettingsWidget::setKeyBindings()
{
    KeyBindingsDialog(this).exec();
}
void SettingsWidget::clearCoversCache()
{
    if (QMessageBox::question(this, tr("Confirm clearing the cached covers"), tr("Do you want to delete all cached covers?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
    {
        QDir dir(QMPlay2Core.getSettingsDir());
        if (dir.cd("Covers"))
        {
            for (const QString &fileName : dir.entryList(QDir::Files))
                QFile::remove(dir.absolutePath() + "/" + fileName);
            if (dir.cdUp())
                dir.rmdir("Covers");
        }
    }
}
#ifdef USE_YOUTUBEDL
void SettingsWidget::removeYouTubeDl()
{
    const QString filePath = YouTubeDL::getFilePath();
    while (QFile::exists(filePath))
    {
        if (QMessageBox::question(this, tr("Confirm \"youtube-dl\" deletion"), tr("Do you want to remove \"youtube-dl\" software?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
            break;
        QFile::remove(filePath);
    }
}
#endif
void SettingsWidget::resetSettings()
{
    if (QMessageBox::question(this, tr("Confirm settings deletion"), tr("Do you really want to clear all settings?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
    {
        QMPlay2GUI.removeSettings = true;
        restartApp();
    }
}
void SettingsWidget::profileListIndexChanged(int index)
{
    generalSettingsPage->profileRemoveB->setEnabled(index != 0);
}
void SettingsWidget::removeProfile()
{
    const QString selectedProfileName = getSelectedProfile();
    const QString selectedProfile = "Profiles/" + selectedProfileName + "/";
    if (selectedProfile == QMPlay2Core.getSettingsProfile())
    {
        QSettings(QMPlay2Core.getSettingsDir() + "Profile.ini", QSettings::IniFormat).setValue("Profile", "/");
        QMPlay2GUI.removeSettings = true;
        restartApp();
    }
    else
    {
        const QString settingsDir = QMPlay2Core.getSettingsDir() + selectedProfile;
        for (const QString &fName : QDir(settingsDir).entryList({"*.ini"}))
            QFile::remove(settingsDir + fName);
        QDir(QMPlay2Core.getSettingsDir()).rmdir(selectedProfile);

        generalSettingsPage->profileB->removeItem(generalSettingsPage->profileB->currentIndex());
        for (QAction *act : QMPlay2GUI.menuBar->options->profilesGroup->actions())
            if (act->text() == selectedProfileName)
            {
                delete act;
                break;
            }
    }
}
