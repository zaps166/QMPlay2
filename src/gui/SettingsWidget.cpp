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
#include <QImageWriter>
#include <QRadioButton>
#include <QActionGroup>
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
#include <QScrollBar>
#include <QBoxLayout>
#include <QTextCodec>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QBuffer>
#include <QLabel>
#ifdef Q_OS_WIN
#   include <QOperatingSystemVersion>
#endif

#include <KeyBindingsDialog.hpp>
#include <Appearance.hpp>
#include <Settings.hpp>
#include <Version.hpp>
#include <MenuBar.hpp>
#include <Module.hpp>

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
    QCheckBox *colorsAndBordersB, *marginsAndAlignmentB, *fontsB;
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

/**/

void SettingsWidget::InitSettings()
{
    Settings &QMPSettings = QMPlay2Core.getSettings();

    auto getInitialOutpuitFilePath = [] {
        QString outputFilePath = Settings("Downloader").getString("DownloadsDirPath");
        while (outputFilePath.endsWith("/"))
            outputFilePath.chop(1);
        if (outputFilePath.isEmpty())
            outputFilePath = QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).value(0, QDir::homePath());
        return outputFilePath;
    };

    QTextCodec *codec = QTextCodec::codecForLocale();
    QMPSettings.init("FallbackSubtitlesEncoding", codec ? codec->name().constData() : "System");

    QMPSettings.init("AudioEnabled", true);
    QMPSettings.init("VideoEnabled", true);
    QMPSettings.init("SubtitlesEnabled", true);

    QMPSettings.init("AudioLanguage", QString());
    QMPSettings.init("SubtitlesLanguage", QString());
    QMPSettings.init("screenshotPth", []{return QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).value(0, QDir::homePath());});
    QMPSettings.init("OutputFilePath", getInitialOutpuitFilePath);
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
    QMPSettings.init("AutoUpdates", []{return !QFile::exists(QMPlay2Core.getShareDir() + "noautoupdates");});
#endif
    QMPSettings.init("MainWidget/KeepDocksSize", false);
    QMPSettings.init("MainWidget/TabPositionNorth", false);
    QMPSettings.init("FullscreenPanelsRightSide", false);
    QMPSettings.init("HideLogo", false);
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
    QMPSettings.init("SkipYtDlpUpdate", false);
    QMPSettings.init("NoCoversCache", false);
    QMPSettings.init("Proxy/Use", false);
    QMPSettings.init("Proxy/Host", QString());
    QMPSettings.init("Proxy/Port", 80);
    QMPSettings.init("Proxy/Login", false);
    QMPSettings.init("Proxy/User", QString());
    QMPSettings.init("Proxy/Password", QString());

#ifdef USE_YOUTUBEDL
    QMPSettings.init("YtDl/CookiesFromBrowserEnabled", false);
    QMPSettings.init("YtDl/CookiesFromBrowser", QString());
# if defined(Q_OS_HAIKU)
    QMPSettings.init("YtDl/CustomPathEnabled", true);
    QMPSettings.init("YtDl/CustomPath", "/bin/yt-dlp");
    QMPSettings.init("YtDl/DontAutoUpdate", true);
# else
    QMPSettings.init("YtDl/CustomPathEnabled", false);
    QMPSettings.init("YtDl/CustomPath", QString());
    QMPSettings.init("YtDl/DontAutoUpdate", false);
# endif
    QMPSettings.init("YtDl/DefaultQualityEnabled", false);
    QMPSettings.init("YtDl/DefaultQuality", QString());
    QMPSettings.init("YtDl/AdditionalParamsEnabled", false);
    QMPSettings.init("YtDl/AdditionalParams", QString());
#endif

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
    QMPSettings.init("Vulkan/HDR", QGuiApplication::platformName().contains(QStringLiteral("wayland")));

    QMPSettings.init("ShortSeek", 5);
    QMPSettings.init("LongSeek", 30);
    QMPSettings.init("AVBufferLocal", 100);
    QMPSettings.init("AVBufferTimeNetwork", 500.0);
    QMPSettings.init("BackwardBuffer", 1);
    QMPSettings.init("AVBufferTimeNetworkLive", 5.0);
    QMPSettings.init("PlayIfBuffered", 1.75);
    QMPSettings.init("MaxVol", 100);
    QMPSettings.init("VolumeL", 100);
    QMPSettings.init("VolumeR", 100);
    QMPSettings.init("ForceSamplerate", false);
    QMPSettings.init("Samplerate", 48000);
    QMPSettings.init("ForceChannels", Qt::Unchecked);
    QMPSettings.init("Channels", 2);
    QMPSettings.init("ResamplerFirst", true);
    QMPSettings.init("ReplayGain/Enabled", false);
    QMPSettings.init("ReplayGain/Album", false);
    QMPSettings.init("ReplayGain/PreventClipping", true);
    QMPSettings.init("ReplayGain/Preamp", 0.0);
    QMPSettings.init("ShowBufferedTimeOnSlider", true);
    QMPSettings.init("WheelAction", true);
    QMPSettings.init("WheelSeek", true);
    QMPSettings.init("LeftMouseTogglePlay", static_cast<int>(0));
    QMPSettings.init("MiddleMouseToggleFullscreen", false);
    QMPSettings.init("AccurateSeek", Qt::Checked);
    QMPSettings.init("UnpauseWhenSeeking", false);
    QMPSettings.init("RestoreAVSState", false);
    QMPSettings.init("DisableSubtitlesAtStartup", false);
    QMPSettings.init("StoreUrlPos", true);
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
    QMPSettings.init("ApplyToASS/ApplyToASS", false);
    QMPSettings.init("OSD/Enabled", true);
    OSDSettingsW::init("Subtitles", 20, 0, 15, 15, 15, 7, 1.0, 0.5, QColor(0xFF, 0xFF, 0xFF, 0xFF), Qt::black, Qt::black, false, false);
    OSDSettingsW::init("OSD",       32, 0, 0,  0,  0,  4, 1.5, 1.5, QColor(0xAA, 0xFF, 0x55, 0xFF), Qt::black, Qt::black, false, false);
    DeintSettingsW::init();
    applyProxy();
}
void SettingsWidget::SetAudioChannelsMenu()
{
    const bool forceChn = QMPlay2Core.getSettings().getInt("ForceChannels");
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
    QMPlay2Core.getSettings().set("ForceChannels", forceChannels ? Qt::Checked : Qt::Unchecked);
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
    layout->setContentsMargins(2, 2, 2, 2);

    {
        QWidget *generalSettingsWidget = new QWidget;
        setupGeneralUi(generalSettingsWidget);

        for (const auto &fmt : QImageWriter::supportedImageFormats())
            m_screenshotFormatB->addItem(QString("." + fmt));

        tabW->addTab(generalSettingsWidget, tr("General settings"));

        int idx;

        m_langBox->addItem("English", "en");
        m_langBox->setCurrentIndex(0);
        const QStringList langs = QMPlay2Core.getLanguages();
        for (int i = 0; i < langs.count(); i++)
        {
            m_langBox->addItem(QMPlay2Core.getLongFromShortLanguage(langs[i]), langs[i]);
            if (QMPlay2Core.getLanguage() == langs[i])
                m_langBox->setCurrentIndex(i + 1);
        }
        m_langBox->model()->sort(0);

        const auto currStyle = QApplication::style()->objectName();
        auto styles = QStyleFactory::keys();
#if defined(Q_OS_WIN) && QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        if (QOperatingSystemVersion::current() < QOperatingSystemVersion::Windows11 && currStyle != "windows11")
            styles.removeOne("windows11");
#endif
        m_styleBox->addItems(styles);
        idx = m_styleBox->findText(currStyle, Qt::MatchFixedString);
        if (idx > -1 && idx < m_styleBox->count())
            m_styleBox->setCurrentIndex(idx);
        connect(m_styleBox, SIGNAL(currentIndexChanged(int)), this, SLOT(chStyle()));

        QStringList encodings;
        for (const QByteArray &item : QTextCodec::availableCodecs())
            encodings += QTextCodec::codecForName(item)->name();
        encodings.removeDuplicates();
        m_encodingB->addItems(encodings);
        idx = m_encodingB->findText(QMPSettings.getByteArray("FallbackSubtitlesEncoding"));
        if (idx > -1)
            m_encodingB->setCurrentIndex(idx);

        const QString audioLang = QMPSettings.getString("AudioLanguage");
        const QString subsLang = QMPSettings.getString("SubtitlesLanguage");
        m_audioLangB->addItem(tr("Default or first stream"));
        m_subsLangB->addItem(tr("Default or first stream"));
        for (const QString &lang : QMPlay2Core.getLanguagesMap())
        {
            m_audioLangB->addItem(lang);
            m_subsLangB->addItem(lang);
            if (lang == audioLang)
                m_audioLangB->setCurrentIndex(m_audioLangB->count() - 1);
            if (lang == subsLang)
                m_subsLangB->setCurrentIndex(m_subsLangB->count() - 1);
        }
        {
            const QString currentProfile = QSettings(QMPlay2Core.getSettingsDir() + "Profile.ini", QSettings::IniFormat).value("Profile").toString();
            m_profileB->addItem(tr("Default"));
            for (const QString &profile : QDir(QMPlay2Core.getSettingsDir() + "Profiles/").entryList(QDir::Dirs | QDir::NoDotAndDotDot))
            {
                m_profileB->addItem(profile);
                if (profile == currentProfile)
                    m_profileB->setCurrentIndex(m_profileB->count() - 1);
            }
            connect(m_profileB, SIGNAL(currentIndexChanged(int)), this, SLOT(profileListIndexChanged(int)));

            m_profileRemoveB->setIcon(QMPlay2Core.getIconFromTheme("list-remove"));
            m_profileRemoveB->setEnabled(m_profileB->currentIndex() != 0);
            connect(m_profileRemoveB, SIGNAL(clicked()), this, SLOT(removeProfile()));
        }

        m_screenshotE->setText(QMPSettings.getString("screenshotPth"));
        m_screenshotFormatB->setCurrentIndex(m_screenshotFormatB->findText(QMPSettings.getString("screenshotFormat")));
        m_screenshotB->setIcon(QMPlay2Core.getIconFromTheme("folder-open"));
        connect(m_screenshotB, SIGNAL(clicked()), this, SLOT(chooseScreenshotDir()));

        m_outputFileE->setText(QMPSettings.getString("OutputFilePath"));
        m_outputFileB->setIcon(QMPlay2Core.getIconFromTheme("folder-open"));
        connect(m_outputFileB, &QAbstractButton::clicked, this, &SettingsWidget::chooseOutputFileDir);

        connect(m_setAppearanceB, SIGNAL(clicked()), this, SLOT(setAppearance()));
        connect(m_setKeyBindingsB, SIGNAL(clicked()), this, SLOT(setKeyBindings()));

#ifdef ICONS_FROM_THEME
        m_iconsFromTheme->setChecked(QMPSettings.getBool("IconsFromTheme"));
#else
        delete m_iconsFromTheme;
        m_iconsFromTheme = nullptr;
#endif

        m_showCoversGB->setChecked(QMPSettings.getBool("ShowCovers"));
        m_blurCoversB->setChecked(QMPSettings.getBool("BlurCovers"));
        m_showDirCoversB->setChecked(QMPSettings.getBool("ShowDirCovers"));
        m_enlargeSmallCoversB->setChecked(QMPSettings.getBool("EnlargeCovers"));

        m_autoOpenVideoWindowB->setChecked(QMPSettings.getBool("AutoOpenVideoWindow"));
        m_autoRestoreMainWindowOnVideoB->setChecked(QMPSettings.getBool("AutoRestoreMainWindowOnVideo"));

#ifdef UPDATES
        m_autoUpdatesB->setChecked(QMPSettings.getBool("AutoUpdates"));
# ifndef UPDATER
        m_autoUpdatesB->setText(tr("Automatically check for updates"));
# endif
#else
        delete m_autoUpdatesB;
        m_autoUpdatesB = nullptr;
#endif

        m_keepDocksSizeB->setChecked(QMPSettings.getBool("MainWidget/KeepDocksSize"));
        m_fullscreenPanelsRightSideB->setChecked(QMPSettings.getBool("FullscreenPanelsRightSide"));
        m_hideLogoB->setChecked(QMPSettings.getBool("HideLogo"));

        if (Notifies::hasBoth())
            m_trayNotifiesDefault->setChecked(QMPSettings.getBool("TrayNotifiesDefault"));
        else
        {
            delete m_trayNotifiesDefault;
            m_trayNotifiesDefault = nullptr;
        }

        m_autoDelNonGroupEntries->setChecked(QMPSettings.getBool("AutoDelNonGroupEntries"));

        m_tabsNorths->setChecked(QMPSettings.getBool("MainWidget/TabPositionNorth"));

#ifdef QMPLAY2_ALLOW_ONLY_ONE_INSTANCE
        m_allowOnlyOneInstance->setChecked(QMPSettings.getBool("AllowOnlyOneInstance"));
#else
        delete m_allowOnlyOneInstance;
        m_allowOnlyOneInstance = nullptr;
#endif

        m_hideArtistMetadata->setChecked(QMPSettings.getBool("HideArtistMetadata"));
        m_displayOnlyFileName->setChecked(QMPSettings.getBool("DisplayOnlyFileName"));
        m_skipPlaylistsWithinFiles->setChecked(QMPSettings.getBool("SkipPlaylistsWithinFiles"));
        m_restoreRepeatMode->setChecked(QMPSettings.getBool("RestoreRepeatMode"));
        m_stillImages->setChecked(QMPSettings.getBool("StillImages"));

        m_proxyB->setChecked(QMPSettings.getBool("Proxy/Use"));
        m_proxyHostE->setText(QMPSettings.getString("Proxy/Host"));
        m_proxyPortB->setValue(QMPSettings.getInt("Proxy/Port"));
        m_proxyLoginB->setChecked(QMPSettings.getBool("Proxy/Login"));
        m_proxyUserE->setText(QMPSettings.getString("Proxy/User"));
        m_proxyPasswordE->setText(QByteArray::fromBase64(QMPSettings.getByteArray("Proxy/Password")));

        const QIcon viewRefresh = QMPlay2Core.getIconFromTheme("view-refresh");
        m_clearCoversCache->setIcon(viewRefresh);
        connect(m_clearCoversCache, SIGNAL(clicked()), this, SLOT(clearCoversCache()));

        m_resetSettingsB->setIcon(viewRefresh);
        connect(m_resetSettingsB, SIGNAL(clicked()), this, SLOT(resetSettings()));

#ifdef USE_YOUTUBEDL
        m_cookiesFromBrowserCB->setChecked(QMPSettings.getBool("YtDl/CookiesFromBrowserEnabled"));
        m_cookiesFromBrowserE->setText(QMPSettings.getString("YtDl/CookiesFromBrowser"));

        m_customYtDlCB->setChecked(QMPSettings.getBool("YtDl/CustomPathEnabled"));
        m_customYtDlE->setText(QMPSettings.getString("YtDl/CustomPath"));
        m_customYtDlB->setIcon(QMPlay2Core.getIconFromTheme("folder-open"));

        m_dfltYtDlQualCB->setChecked(QMPSettings.getBool("YtDl/DefaultQualityEnabled"));
        m_dfltYtDlQualE->setText(QMPSettings.getString("YtDl/DefaultQuality"));

        m_additionalYtDlParamsCB->setChecked(QMPSettings.getBool("YtDl/AdditionalParamsEnabled"));
        m_additionalYtDlParamsE->setText(QMPSettings.getString("YtDl/AdditionalParams"));

        m_dontUpdateYtDlCB->setChecked(QMPSettings.getBool("YtDl/DontAutoUpdate"));

        connect(m_cookiesFromBrowserCB, &QCheckBox::toggled, this, [this](bool checked) {
            m_cookiesFromBrowserE->setEnabled(checked);
        });
        connect(m_customYtDlCB, &QCheckBox::toggled, this, [this](bool checked) {
            m_customYtDlE->setEnabled(checked);
            m_customYtDlB->setEnabled(checked);
            m_removeYtDlB->setEnabled(!checked);
        });
        connect(m_customYtDlB, &QToolButton::clicked, this, [this] {
            auto path = QFileDialog::getOpenFileName(this, tr("Choose youtube-dl script or executable"), m_customYtDlE->text());
            if (!path.isEmpty())
                m_customYtDlE->setText(path);
        });
        connect(m_dfltYtDlQualCB, &QCheckBox::toggled, this, [this](bool checked) {
            m_dfltYtDlQualE->setEnabled(checked);
        });
        connect(m_additionalYtDlParamsCB, &QCheckBox::toggled, this, [this](bool checked) {
            m_additionalYtDlParamsE->setEnabled(checked);
        });

        emit m_cookiesFromBrowserCB->toggled(m_cookiesFromBrowserCB->isChecked());
        emit m_customYtDlCB->toggled(m_customYtDlCB->isChecked());
        emit m_dfltYtDlQualCB->toggled(m_dfltYtDlQualCB->isChecked());
        emit m_additionalYtDlParamsCB->toggled(m_additionalYtDlParamsCB->isChecked());

        connect(m_customYtDlCB, &QCheckBox::toggled, this, [this](bool checked) {
            m_dontUpdateYtDlCB->setChecked(checked);
        });

        m_removeYtDlB->setIcon(QMPlay2Core.getIconFromTheme("list-remove"));
        connect(m_removeYtDlB, &QPushButton::clicked, this, &SettingsWidget::removeYouTubeDl);
#else
        m_ytDlGB->deleteLater();
        m_ytDlGB = nullptr;
#endif
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

            setupModulesListUi(groupB, modulesList[m]);

            connect(modulesList[m].list, SIGNAL(itemDoubleClicked (QListWidgetItem *)), this, SLOT(openModuleSettings(QListWidgetItem *)));
            connect(modulesList[m].moveUp, SIGNAL(clicked()), this, SLOT(moveModule()));
            connect(modulesList[m].moveDown, SIGNAL(clicked()), this, SLOT(moveModule()));
            modulesList[m].moveUp->setProperty("idx", m);
            modulesList[m].moveDown->setProperty("idx", m);
        }
    }

    createRendererSettings();

    {
        auto getDesiredVideoHeightIndex = [&] {
            const int desiredVideoHeight = QMPSettings.getInt("DesiredVideoHeight");
            if (desiredVideoHeight <= 0)
                return 0;
            if (desiredVideoHeight < 720)
                return 1;
            if (desiredVideoHeight < 1080)
                return 2;
            if (desiredVideoHeight < 2160)
                return 3;
            return 4;
        };

        QWidget *playbackSettingsWidget = new QWidget;
        setupPlaybackUi(playbackSettingsWidget);

        m_shortSeekB->setSuffix(" " + m_shortSeekB->suffix());
        m_longSeekB->setSuffix(" " + m_longSeekB->suffix());
        m_bufferNetworkB->setSuffix(" " + m_bufferNetworkB->suffix());
        m_playIfBufferedB->setSuffix(" " + m_playIfBufferedB->suffix());
        m_bufferLiveB->setSuffix(" " + m_bufferLiveB->suffix());
        m_replayGainPreamp->setPrefix(m_replayGainPreamp->prefix() + ": ");
        m_replayGainPreampNoMetadata->setPrefix(m_replayGainPreampNoMetadata->prefix() + ": ");

        tabW->addTab(playbackSettingsWidget, tr("Playback settings"));

        m_shortSeekB->setValue(QMPSettings.getInt("ShortSeek"));
        m_longSeekB->setValue(QMPSettings.getInt("LongSeek"));
        m_bufferLocalB->setValue(QMPSettings.getInt("AVBufferLocal"));
        m_bufferNetworkB->setValue(QMPSettings.getDouble("AVBufferTimeNetwork"));
        m_backwardBufferNetworkB->setCurrentIndex(QMPSettings.getUInt("BackwardBuffer"));
        m_bufferLiveB->setValue(QMPSettings.getDouble("AVBufferTimeNetworkLive"));
        m_desiredVideoHeightB->setCurrentIndex(getDesiredVideoHeightIndex());
        m_playIfBufferedB->setValue(QMPSettings.getDouble("PlayIfBuffered"));
        m_maxVolB->setValue(QMPSettings.getInt("MaxVol"));

        m_forceSamplerate->setChecked(QMPSettings.getBool("ForceSamplerate"));
        m_samplerateB->setValue(QMPSettings.getInt("Samplerate"));
        connect(m_forceSamplerate, SIGNAL(toggled(bool)), m_samplerateB, SLOT(setEnabled(bool)));
        m_samplerateB->setEnabled(m_forceSamplerate->isChecked());

        m_forceChannels->setCheckState(QMPSettings.getWithBounds("ForceChannels", Qt::Unchecked, Qt::Checked));
        m_forceChannels->setToolTip(tr("Force audio content to use the specified number of channels.\n"
            "Partially checked only if the content has less channels than the specified value\n"
            "\t(e.g. upgrade mono to stereo but do not degrade quadrophonic to stereo)"));
        m_channelsB->setValue(QMPSettings.getInt("Channels"));
        connect(m_forceChannels, SIGNAL(toggled(bool)), m_channelsB, SLOT(setEnabled(bool)));
        m_channelsB->setEnabled(m_forceChannels->checkState());

        m_resamplerFirst->setChecked(QMPSettings.getBool("ResamplerFirst"));

        m_replayGain->setChecked(QMPSettings.getBool("ReplayGain/Enabled"));
        m_replayGainAlbum->setChecked(QMPSettings.getBool("ReplayGain/Album"));
        m_replayGainPreventClipping->setChecked(QMPSettings.getBool("ReplayGain/PreventClipping"));
        m_replayGainPreamp->setValue(QMPSettings.getDouble("ReplayGain/Preamp"));
        m_replayGainPreampNoMetadata->setValue(QMPSettings.getDouble("ReplayGain/PreampNoMetadata"));

        m_wheelActionB->setChecked(QMPSettings.getBool("WheelAction"));
        m_wheelSeekB->setChecked(QMPSettings.getBool("WheelSeek"));
        m_wheelVolumeB->setChecked(QMPSettings.getBool("WheelVolume"));

        m_storeARatioAndZoomB->setChecked(QMPSettings.getBool("StoreARatioAndZoom"));
        connect(m_storeARatioAndZoomB, &QCheckBox::toggled, this, [this](bool checked) {
            if (checked)
            {
                m_keepZoom->setChecked(true);
                m_keepARatio->setChecked(true);
            }
        });

        m_keepZoom->setChecked(QMPSettings.getBool("KeepZoom"));
        connect(m_keepZoom, &QCheckBox::toggled, this, [this](bool checked) {
            if (!checked && !m_keepARatio->isChecked())
            {
                m_storeARatioAndZoomB->setChecked(false);
            }
        });

        m_keepARatio->setChecked(QMPSettings.getBool("KeepARatio"));
        connect(m_keepARatio, &QCheckBox::toggled, this, [this](bool checked) {
            if (!checked && !m_keepZoom->isChecked())
            {
                m_storeARatioAndZoomB->setChecked(false);
            }
        });

        m_showBufferedTimeOnSlider->setChecked(QMPSettings.getBool("ShowBufferedTimeOnSlider"));
        m_savePos->setChecked(QMPSettings.getBool("SavePos"));
        m_keepSubtitlesDelay->setChecked(QMPSettings.getBool("KeepSubtitlesDelay"));
        m_keepSubtitlesScale->setChecked(QMPSettings.getBool("KeepSubtitlesScale"));
        m_keepVideoDelay->setChecked(QMPSettings.getBool("KeepVideoDelay"));
        m_keepSpeed->setChecked(QMPSettings.getBool("KeepSpeed"));
        m_syncVtoA->setChecked(QMPSettings.getBool("SyncVtoA"));
        m_silence->setChecked(QMPSettings.getBool("Silence"));
        m_restoreVideoEq->setChecked(QMPSettings.getBool("RestoreVideoEqualizer"));
        m_ignorePlaybackError->setChecked(QMPSettings.getBool("IgnorePlaybackError"));
        m_leftMouseTogglePlay->setCheckState((Qt::CheckState)qBound(0, QMPSettings.getInt("LeftMouseTogglePlay"), 2));
        m_middleMouseToggleFullscreen->setChecked(QMPSettings.getBool("MiddleMouseToggleFullscreen"));

        m_accurateSeekB->setCheckState((Qt::CheckState)QMPSettings.getInt("AccurateSeek"));
        m_accurateSeekB->setToolTip(tr("Slower, but more accurate seeking.\nPartially checked doesn't affect seeking on slider."));

        m_unpauseWhenSeekingB->setChecked(QMPSettings.getBool("UnpauseWhenSeeking"));

        m_restoreAVSStateB->setChecked(QMPSettings.getBool("RestoreAVSState"));
        m_disableSubtitlesAtStartup->setChecked(QMPSettings.getBool("DisableSubtitlesAtStartup"));

        m_storeUrlPosB->setChecked(QMPSettings.getBool("StoreUrlPos"));

        for (int m = 1; m < 3; ++m)
            m_modulesListLayout->addWidget(m_modulesListGroupBox[m]);
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
        layout->setContentsMargins(0, 0, 0, 0);
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

        page4->toAssGB = new QGroupBox(tr("Apply for ASS/SSA subtitles"));
        page4->toAssGB->setCheckable(true);
        page4->toAssGB->setChecked(QMPSettings.getBool("ApplyToASS/ApplyToASS"));

        QGridLayout *page4ToAssLayout = new QGridLayout(page4->toAssGB);
        page4ToAssLayout->addWidget(page4->colorsAndBordersB, 0, 0, 1, 1);
        page4ToAssLayout->addWidget(page4->marginsAndAlignmentB, 1, 0, 1, 1);
        page4ToAssLayout->addWidget(page4->fontsB, 0, 1, 1, 1);

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
        layout->setContentsMargins(0, 0, 0, 0);

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
            QGroupBox *otherHWVFiltersContainer = new QGroupBox(tr("Hardware-accelerated video filters"));
            QGridLayout *otherHWVFiltersLayout = new QGridLayout(otherHWVFiltersContainer);
            otherHWVFiltersLayout->addWidget(otherHWVFiltersW);
            connect(otherHWVFiltersW, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(openModuleSettings(QListWidgetItem *)));
            layout->addWidget(otherHWVFiltersContainer, 2, 1, 1, 1);
        }

        page6->setWidget(widget);
    }

    connect(tabW, SIGNAL(currentChanged(int)), this, SLOT(tabCh(int)));
    tabW->setCurrentIndex(page);
}
SettingsWidget::~SettingsWidget()
{
}

void SettingsWidget::setAudioChannels()
{
    m_forceChannels->setCheckState(QMPlay2Core.getSettings().getWithBounds("ForceChannels", Qt::Unchecked, Qt::Checked));
    m_channelsB->setValue(QMPlay2Core.getSettings().getInt("Channels"));
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

void SettingsWidget::setupGeneralUi(QWidget *GeneralSettings)
{
    QVBoxLayout *generalLayout = new QVBoxLayout(GeneralSettings);
    generalLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *generalScrollArea = new QScrollArea(GeneralSettings);
    generalScrollArea->setFrameShape(QFrame::NoFrame);
    generalScrollArea->setFrameShadow(QFrame::Plain);
    generalScrollArea->setLineWidth(0);
    generalScrollArea->setWidgetResizable(true);

    QWidget *generalScrollAreaWidgetContents = new QWidget();
    generalScrollAreaWidgetContents->setGeometry(QRect(0, 0, 798, 1076));
    QGridLayout *generalGridLayout = new QGridLayout(generalScrollAreaWidgetContents);
    generalGridLayout->setSpacing(1);
    generalGridLayout->setContentsMargins(1, 1, 1, 1);

    QFormLayout *generalFormLayout = new QFormLayout();
    generalFormLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    QLabel *langL = new QLabel(generalScrollAreaWidgetContents);
    generalFormLayout->setWidget(0, QFormLayout::ItemRole::LabelRole, langL);
    m_langBox = new QComboBox(generalScrollAreaWidgetContents);
    generalFormLayout->setWidget(0, QFormLayout::ItemRole::FieldRole, m_langBox);

    QLabel *styleL = new QLabel(generalScrollAreaWidgetContents);
    generalFormLayout->setWidget(1, QFormLayout::ItemRole::LabelRole, styleL);
    m_styleBox = new QComboBox(generalScrollAreaWidgetContents);
    generalFormLayout->setWidget(1, QFormLayout::ItemRole::FieldRole, m_styleBox);

    QLabel *encodingL = new QLabel(generalScrollAreaWidgetContents);
    generalFormLayout->setWidget(2, QFormLayout::ItemRole::LabelRole, encodingL);
    m_encodingB = new QComboBox(generalScrollAreaWidgetContents);
    generalFormLayout->setWidget(2, QFormLayout::ItemRole::FieldRole, m_encodingB);

    QLabel *audioLangL = new QLabel(generalScrollAreaWidgetContents);
    generalFormLayout->setWidget(3, QFormLayout::ItemRole::LabelRole, audioLangL);
    m_audioLangB = new QComboBox(generalScrollAreaWidgetContents);
    generalFormLayout->setWidget(3, QFormLayout::ItemRole::FieldRole, m_audioLangB);

    QLabel *subsLangL = new QLabel(generalScrollAreaWidgetContents);
    generalFormLayout->setWidget(4, QFormLayout::ItemRole::LabelRole, subsLangL);
    m_subsLangB = new QComboBox(generalScrollAreaWidgetContents);
    generalFormLayout->setWidget(4, QFormLayout::ItemRole::FieldRole, m_subsLangB);

    QLabel *screenshotL = new QLabel(generalScrollAreaWidgetContents);
    generalFormLayout->setWidget(5, QFormLayout::ItemRole::LabelRole, screenshotL);
    QHBoxLayout *generalHorizontalLayout = new QHBoxLayout();
    generalHorizontalLayout->setSpacing(1);
    m_screenshotE = new QLineEdit(generalScrollAreaWidgetContents);
    m_screenshotE->setReadOnly(true);
    generalHorizontalLayout->addWidget(m_screenshotE);
    m_screenshotFormatB = new QComboBox(generalScrollAreaWidgetContents);
    QSizePolicy sizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(m_screenshotFormatB->sizePolicy().hasHeightForWidth());
    m_screenshotFormatB->setSizePolicy(sizePolicy);
    generalHorizontalLayout->addWidget(m_screenshotFormatB);
    m_screenshotB = new QToolButton(generalScrollAreaWidgetContents);
    generalHorizontalLayout->addWidget(m_screenshotB);
    generalFormLayout->setLayout(5, QFormLayout::ItemRole::FieldRole, generalHorizontalLayout);

    QLabel *outputFileL = new QLabel(generalScrollAreaWidgetContents);
    generalFormLayout->setWidget(6, QFormLayout::ItemRole::LabelRole, outputFileL);
    QHBoxLayout *generalHorizontalLayout4 = new QHBoxLayout();
    m_outputFileE = new QLineEdit(generalScrollAreaWidgetContents);
    m_outputFileE->setReadOnly(true);
    generalHorizontalLayout4->addWidget(m_outputFileE);
    m_outputFileB = new QToolButton(generalScrollAreaWidgetContents);
    generalHorizontalLayout4->addWidget(m_outputFileB);
    generalFormLayout->setLayout(6, QFormLayout::ItemRole::FieldRole, generalHorizontalLayout4);

    m_iconsFromTheme = new QCheckBox(generalScrollAreaWidgetContents);
    generalFormLayout->setWidget(7, QFormLayout::ItemRole::LabelRole, m_iconsFromTheme);
    m_setAppearanceB = new QPushButton(generalScrollAreaWidgetContents);
    generalFormLayout->setWidget(7, QFormLayout::ItemRole::FieldRole, m_setAppearanceB);

    QLabel *profileL = new QLabel(generalScrollAreaWidgetContents);
    generalFormLayout->setWidget(8, QFormLayout::ItemRole::LabelRole, profileL);
    QHBoxLayout *generalHorizontalLayout5 = new QHBoxLayout();
    m_profileB = new QComboBox(generalScrollAreaWidgetContents);
    generalHorizontalLayout5->addWidget(m_profileB);
    m_profileRemoveB = new QToolButton(generalScrollAreaWidgetContents);
    generalHorizontalLayout5->addWidget(m_profileRemoveB);
    generalFormLayout->setLayout(8, QFormLayout::ItemRole::FieldRole, generalHorizontalLayout5);

    m_showCoversGB = new QGroupBox(generalScrollAreaWidgetContents);
    m_showCoversGB->setCheckable(true);
    QGridLayout *showCoversGridLayout = new QGridLayout(m_showCoversGB);
    showCoversGridLayout->setSpacing(3);
    showCoversGridLayout->setContentsMargins(3, 3, 3, 3);
    m_blurCoversB = new QCheckBox(m_showCoversGB);
    showCoversGridLayout->addWidget(m_blurCoversB, 0, 0, 1, 2);
    m_showDirCoversB = new QCheckBox(m_showCoversGB);
    showCoversGridLayout->addWidget(m_showDirCoversB, 1, 0, 1, 2);
    m_enlargeSmallCoversB = new QCheckBox(m_showCoversGB);
    showCoversGridLayout->addWidget(m_enlargeSmallCoversB, 2, 0, 1, 2);

    QSpacerItem *generalHorizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

    m_autoUpdatesB = new QCheckBox(generalScrollAreaWidgetContents);
    m_autoOpenVideoWindowB = new QCheckBox(generalScrollAreaWidgetContents);
    m_autoRestoreMainWindowOnVideoB = new QCheckBox(generalScrollAreaWidgetContents);
    m_keepDocksSizeB = new QCheckBox(generalScrollAreaWidgetContents);
    m_tabsNorths = new QCheckBox(generalScrollAreaWidgetContents);
    m_allowOnlyOneInstance = new QCheckBox(generalScrollAreaWidgetContents);
    m_hideArtistMetadata = new QCheckBox(generalScrollAreaWidgetContents);
    m_displayOnlyFileName = new QCheckBox(generalScrollAreaWidgetContents);
    m_skipPlaylistsWithinFiles = new QCheckBox(generalScrollAreaWidgetContents);
    m_restoreRepeatMode = new QCheckBox(generalScrollAreaWidgetContents);
    m_stillImages = new QCheckBox(generalScrollAreaWidgetContents);
    m_trayNotifiesDefault = new QCheckBox(generalScrollAreaWidgetContents);
    m_autoDelNonGroupEntries = new QCheckBox(generalScrollAreaWidgetContents);
    m_fullscreenPanelsRightSideB = new QCheckBox(generalScrollAreaWidgetContents);
    m_hideLogoB = new QCheckBox(generalScrollAreaWidgetContents);

    m_proxyB = new QGroupBox(generalScrollAreaWidgetContents);
    m_proxyB->setCheckable(true);
    QVBoxLayout *proxyLayout = new QVBoxLayout(m_proxyB);
    m_proxyLoginB = new QGroupBox(m_proxyB);
    m_proxyLoginB->setCheckable(true);
    QVBoxLayout *proxyLoginLayout = new QVBoxLayout(m_proxyLoginB);
    m_proxyUserE = new QLineEdit(m_proxyLoginB);
    proxyLoginLayout->addWidget(m_proxyUserE);
    m_proxyPasswordE = new QLineEdit(m_proxyLoginB);
    m_proxyPasswordE->setEchoMode(QLineEdit::Password);
    proxyLoginLayout->addWidget(m_proxyPasswordE);
    proxyLayout->addWidget(m_proxyLoginB);
    QHBoxLayout *generalHorizontalLayout2 = new QHBoxLayout();
    m_proxyHostE = new QLineEdit(m_proxyB);
    generalHorizontalLayout2->addWidget(m_proxyHostE);
    m_proxyPortB = new QSpinBox(m_proxyB);
    m_proxyPortB->setMinimum(1);
    m_proxyPortB->setMaximum(65535);
    generalHorizontalLayout2->addWidget(m_proxyPortB);
    proxyLayout->addLayout(generalHorizontalLayout2);

    QHBoxLayout *generalHorizontalLayout3 = new QHBoxLayout();
    generalHorizontalLayout3->setSpacing(3);
    generalHorizontalLayout3->setContentsMargins(-1, 3, -1, 3);
    m_clearCoversCache = new QPushButton(generalScrollAreaWidgetContents);
    generalHorizontalLayout3->addWidget(m_clearCoversCache);
    m_setKeyBindingsB = new QPushButton(generalScrollAreaWidgetContents);
    generalHorizontalLayout3->addWidget(m_setKeyBindingsB);
    m_resetSettingsB = new QPushButton(generalScrollAreaWidgetContents);
    generalHorizontalLayout3->addWidget(m_resetSettingsB);

    m_ytDlGB = new QGroupBox(generalScrollAreaWidgetContents);
    m_ytDlGB->setCheckable(false);
    QGridLayout *ytDlGridLayout = new QGridLayout(m_ytDlGB);
    m_cookiesFromBrowserCB = new QCheckBox(m_ytDlGB);
    ytDlGridLayout->addWidget(m_cookiesFromBrowserCB, 0, 0, 1, 1);
    m_cookiesFromBrowserE = new QLineEdit(m_ytDlGB);
    m_cookiesFromBrowserE->setMaxLength(255);
    ytDlGridLayout->addWidget(m_cookiesFromBrowserE, 0, 1, 1, 2);
    m_customYtDlCB = new QCheckBox(m_ytDlGB);
    ytDlGridLayout->addWidget(m_customYtDlCB, 1, 0, 1, 1);
    m_customYtDlE = new QLineEdit(m_ytDlGB);
    ytDlGridLayout->addWidget(m_customYtDlE, 1, 1, 1, 1);
    m_customYtDlB = new QToolButton(m_ytDlGB);
    ytDlGridLayout->addWidget(m_customYtDlB, 1, 2, 1, 1);
    m_dfltYtDlQualCB = new QCheckBox(m_ytDlGB);
    ytDlGridLayout->addWidget(m_dfltYtDlQualCB, 2, 0, 1, 1);
    m_dfltYtDlQualE = new QLineEdit(m_ytDlGB);
    m_dfltYtDlQualE->setMaxLength(255);
    ytDlGridLayout->addWidget(m_dfltYtDlQualE, 2, 1, 1, 2);
    m_additionalYtDlParamsCB = new QCheckBox(m_ytDlGB);
    ytDlGridLayout->addWidget(m_additionalYtDlParamsCB, 3, 0, 1, 1);
    m_additionalYtDlParamsE = new QLineEdit(m_ytDlGB);
    m_additionalYtDlParamsE->setMaxLength(999);
    ytDlGridLayout->addWidget(m_additionalYtDlParamsE, 3, 1, 1, 2);
    m_dontUpdateYtDlCB = new QCheckBox(m_ytDlGB);
    ytDlGridLayout->addWidget(m_dontUpdateYtDlCB, 4, 0, 1, 1);
    m_removeYtDlB = new QPushButton(m_ytDlGB);
    ytDlGridLayout->addWidget(m_removeYtDlB, 5, 0, 1, 1);

    QSpacerItem *generalVerticalSpacer = new QSpacerItem(20, 17, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

    generalGridLayout->addLayout(generalFormLayout, 0, 0, 1, 2);
    generalGridLayout->addWidget(m_showCoversGB, 1, 0, 1, 1);
    generalGridLayout->addItem(generalHorizontalSpacer, 1, 1, 19, 1);
    generalGridLayout->addWidget(m_autoUpdatesB, 2, 0, 1, 1);
    generalGridLayout->addWidget(m_autoOpenVideoWindowB, 3, 0, 1, 1);
    generalGridLayout->addWidget(m_autoRestoreMainWindowOnVideoB, 4, 0, 1, 1);
    generalGridLayout->addWidget(m_keepDocksSizeB, 5, 0, 1, 1);
    generalGridLayout->addWidget(m_tabsNorths, 6, 0, 1, 1);
    generalGridLayout->addWidget(m_allowOnlyOneInstance, 7, 0, 1, 1);
    generalGridLayout->addWidget(m_hideArtistMetadata, 8, 0, 1, 1);
    generalGridLayout->addWidget(m_displayOnlyFileName, 9, 0, 1, 1);
    generalGridLayout->addWidget(m_skipPlaylistsWithinFiles, 10, 0, 1, 1);
    generalGridLayout->addWidget(m_restoreRepeatMode, 11, 0, 1, 1);
    generalGridLayout->addWidget(m_stillImages, 12, 0, 1, 1);
    generalGridLayout->addWidget(m_trayNotifiesDefault, 13, 0, 1, 1);
    generalGridLayout->addWidget(m_autoDelNonGroupEntries, 14, 0, 1, 1);
    generalGridLayout->addWidget(m_fullscreenPanelsRightSideB, 15, 0, 1, 1);
    generalGridLayout->addWidget(m_hideLogoB, 16, 0, 1, 1);
    generalGridLayout->addWidget(m_proxyB, 17, 0, 1, 1);
    generalGridLayout->addLayout(generalHorizontalLayout3, 18, 0, 1, 1);
    generalGridLayout->addWidget(m_ytDlGB, 19, 0, 1, 1);
    generalGridLayout->addItem(generalVerticalSpacer, 20, 0, 1, 2);

    generalScrollArea->setWidget(generalScrollAreaWidgetContents);
    generalLayout->addWidget(generalScrollArea);

    m_showCoversGB->setTitle(tr("Show covers"));
    m_showDirCoversB->setText(tr("Show covers from directory if they aren't in the music file"));
    m_enlargeSmallCoversB->setText(tr("Enlarge small covers"));
    m_blurCoversB->setText(tr("Blurred covers as background"));
    m_clearCoversCache->setText(tr("Clear covers cache"));
    m_setKeyBindingsB->setText(tr("Set key bindings"));
    m_resetSettingsB->setText(tr("Reset settings"));
    langL->setText(tr("Language: "));
    styleL->setText(tr("Style: "));
    encodingL->setText(tr("Subtitles and tags encoding: "));
    audioLangL->setText(tr("Default audio language: "));
    subsLangL->setText(tr("Default subtitles language: "));
    screenshotL->setText(tr("Screenshots path: "));
    m_screenshotB->setToolTip(tr("Browse"));
    outputFileL->setText(tr("Output file path: "));
    m_outputFileB->setToolTip(tr("Browse"));
    m_iconsFromTheme->setText(tr("Use system icon set"));
    m_setAppearanceB->setText(tr("Set appearance"));
    profileL->setText(tr("Selected Profile: "));
    m_autoUpdatesB->setText(tr("Automatically check and download updates"));
    m_autoOpenVideoWindowB->setText(tr("Automatically open video window"));
    m_tabsNorths->setText(tr("Show tabs at the top of the main window"));
    m_allowOnlyOneInstance->setText(tr("Allow only one instance"));
    m_displayOnlyFileName->setText(tr("Always only display file names in playlist"));
    m_restoreRepeatMode->setText(tr("Remember repeat mode"));
    m_proxyB->setTitle(tr("Use proxy server"));
    m_proxyLoginB->setTitle(tr("Proxy server needs login"));
    m_proxyUserE->setPlaceholderText(tr("User name"));
    m_proxyPasswordE->setPlaceholderText(tr("Password"));
    m_proxyHostE->setPlaceholderText(tr("Proxy server address"));
    m_proxyPortB->setToolTip(tr("Proxy server port"));
    m_stillImages->setText(tr("Read and display still images"));
    m_trayNotifiesDefault->setText(tr("Use tray notifications as default"));
    m_autoDelNonGroupEntries->setText(tr("Automatically delete ungrouped entries"));
    m_hideArtistMetadata->setText(tr("Hide artist metadata"));
    m_autoRestoreMainWindowOnVideoB->setText(tr("Automatically restore main window when new video file is loaded"));
    m_skipPlaylistsWithinFiles->setText(tr("Don't load playlist files within other files"));
    m_ytDlGB->setTitle(tr("youtube-dl settings"));
    m_cookiesFromBrowserCB->setText(tr("Cookies from browser"));
    m_customYtDlCB->setText(tr("Custom path"));
    m_customYtDlB->setToolTip(tr("Browse"));
    m_dontUpdateYtDlCB->setText(tr("Don't auto-update"));
    m_removeYtDlB->setText(tr("Remove youtube-dl"));
    m_cookiesFromBrowserE->setToolTip(tr("Please refer to yt-dlp documentation"));
    m_dfltYtDlQualCB->setText(tr("Default quality"));
    m_dfltYtDlQualE->setToolTip(tr("\"-f\" parameter, please refer to yt-dlp documentation"));
    m_additionalYtDlParamsCB->setText(tr("Additional params"));
    m_additionalYtDlParamsE->setToolTip(tr("Please refer to yt-dlp documentation"));
    m_keepDocksSizeB->setText(tr("Maintain panels size when resizing the main window (experimental)"));
    m_fullscreenPanelsRightSideB->setText(tr("Fullscreen panels on the right side"));
    m_hideLogoB->setText(tr("Hide QMPlay2 logo"));
}
void SettingsWidget::setupModulesListUi(QGroupBox *ModulesList, UiModulesList &ml)
{
    ml.horizontalLayout = new QHBoxLayout(ModulesList);
    ml.horizontalLayout->setSpacing(3);
    ml.horizontalLayout->setContentsMargins(3, 3, 3, 3);

    ml.list = new QListWidget(ModulesList);
    QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(ml.list->sizePolicy().hasHeightForWidth());
    ml.list->setSizePolicy(sizePolicy);
    ml.list->setDragDropMode(QAbstractItemView::InternalMove);
    ml.list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ml.list->setIconSize(QSize(32, 32));
    ml.horizontalLayout->addWidget(ml.list);

    ml.buttonsW = new QVBoxLayout();
    ml.buttonsW->setSpacing(3);
    ml.moveUp = new QToolButton(ModulesList);
    QSizePolicy sizePolicy1(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(ml.moveUp->sizePolicy().hasHeightForWidth());
    ml.moveUp->setSizePolicy(sizePolicy1);
    ml.moveUp->setArrowType(Qt::UpArrow);
    ml.buttonsW->addWidget(ml.moveUp);

    ml.moveDown = new QToolButton(ModulesList);
    sizePolicy1.setHeightForWidth(ml.moveDown->sizePolicy().hasHeightForWidth());
    ml.moveDown->setSizePolicy(sizePolicy1);
    ml.moveDown->setArrowType(Qt::DownArrow);
    ml.buttonsW->addWidget(ml.moveDown);

    ml.horizontalLayout->addLayout(ml.buttonsW);

    ml.moveUp->setToolTip(tr("Move up"));
    ml.moveUp->setText(QStringLiteral("..."));
    ml.moveDown->setToolTip(tr("Move down"));
    ml.moveDown->setText(QStringLiteral("..."));
}

void SettingsWidget::setupPlaybackUi(QWidget *PlaybackSettings)
{
    QVBoxLayout *playbackLayout = new QVBoxLayout(PlaybackSettings);
    playbackLayout->setSpacing(0);
    playbackLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *playbackScrollArea = new QScrollArea(PlaybackSettings);
    playbackScrollArea->setFrameShape(QFrame::NoFrame);
    playbackScrollArea->setFrameShadow(QFrame::Plain);
    playbackScrollArea->setLineWidth(0);
    playbackScrollArea->setWidgetResizable(true);

    QWidget *playbackScrollAreaWidgetContents = new QWidget();
    playbackScrollAreaWidgetContents->setGeometry(QRect(0, 0, 1086, 1002));
    QGridLayout *playbackGridLayout = new QGridLayout(playbackScrollAreaWidgetContents);
    playbackGridLayout->setContentsMargins(1, 1, 1, 1);
    playbackGridLayout->setVerticalSpacing(1);

    QFormLayout *playbackFormLayout = new QFormLayout();
    playbackFormLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    playbackFormLayout->setVerticalSpacing(1);

    QLabel *shortSeekL = new QLabel(playbackScrollAreaWidgetContents);
    playbackFormLayout->setWidget(0, QFormLayout::ItemRole::LabelRole, shortSeekL);
    m_shortSeekB = new QSpinBox(playbackScrollAreaWidgetContents);
    m_shortSeekB->setMinimum(2);
    m_shortSeekB->setMaximum(20);
    playbackFormLayout->setWidget(0, QFormLayout::ItemRole::FieldRole, m_shortSeekB);

    QLabel *longSeekL = new QLabel(playbackScrollAreaWidgetContents);
    playbackFormLayout->setWidget(1, QFormLayout::ItemRole::LabelRole, longSeekL);
    m_longSeekB = new QSpinBox(playbackScrollAreaWidgetContents);
    m_longSeekB->setMinimum(30);
    m_longSeekB->setMaximum(300);
    playbackFormLayout->setWidget(1, QFormLayout::ItemRole::FieldRole, m_longSeekB);

    QLabel *bufferLocalL = new QLabel(playbackScrollAreaWidgetContents);
    playbackFormLayout->setWidget(2, QFormLayout::ItemRole::LabelRole, bufferLocalL);
    m_bufferLocalB = new QSpinBox(playbackScrollAreaWidgetContents);
    m_bufferLocalB->setMinimum(10);
    m_bufferLocalB->setMaximum(1000);
    playbackFormLayout->setWidget(2, QFormLayout::ItemRole::FieldRole, m_bufferLocalB);

    QLabel *bufferNetworkL = new QLabel(playbackScrollAreaWidgetContents);
    playbackFormLayout->setWidget(3, QFormLayout::ItemRole::LabelRole, bufferNetworkL);
    m_bufferNetworkB = new QDoubleSpinBox(playbackScrollAreaWidgetContents);
    m_bufferNetworkB->setDecimals(1);
    m_bufferNetworkB->setMinimum(0.1);
    m_bufferNetworkB->setMaximum(10000.0);
    m_bufferNetworkB->setSingleStep(0.1);
    playbackFormLayout->setWidget(3, QFormLayout::ItemRole::FieldRole, m_bufferNetworkB);

    QLabel *backwardBufferNetworkL = new QLabel(playbackScrollAreaWidgetContents);
    playbackFormLayout->setWidget(4, QFormLayout::ItemRole::LabelRole, backwardBufferNetworkL);
    m_backwardBufferNetworkB = new QComboBox(playbackScrollAreaWidgetContents);
    m_backwardBufferNetworkB->addItem(QString::fromUtf8("10 %"));
    m_backwardBufferNetworkB->addItem(QString::fromUtf8("25 %"));
    m_backwardBufferNetworkB->addItem(QString::fromUtf8("50 %"));
    m_backwardBufferNetworkB->addItem(QString::fromUtf8("75 %"));
    m_backwardBufferNetworkB->addItem(QString::fromUtf8("100 %"));
    playbackFormLayout->setWidget(4, QFormLayout::ItemRole::FieldRole, m_backwardBufferNetworkB);

    QLabel *bufferLiveL = new QLabel(playbackScrollAreaWidgetContents);
    playbackFormLayout->setWidget(5, QFormLayout::ItemRole::LabelRole, bufferLiveL);
    m_bufferLiveB = new QDoubleSpinBox(playbackScrollAreaWidgetContents);
    m_bufferLiveB->setDecimals(1);
    m_bufferLiveB->setMinimum(0.1);
    m_bufferLiveB->setMaximum(10000.0);
    m_bufferLiveB->setSingleStep(0.1);
    playbackFormLayout->setWidget(5, QFormLayout::ItemRole::FieldRole, m_bufferLiveB);

    QLabel *playIfBufferedL = new QLabel(playbackScrollAreaWidgetContents);
    playbackFormLayout->setWidget(6, QFormLayout::ItemRole::LabelRole, playIfBufferedL);
    m_playIfBufferedB = new QDoubleSpinBox(playbackScrollAreaWidgetContents);
    m_playIfBufferedB->setMaximum(5.0);
    m_playIfBufferedB->setSingleStep(0.25);
    playbackFormLayout->setWidget(6, QFormLayout::ItemRole::FieldRole, m_playIfBufferedB);

    QLabel *desiredVideoHeightL = new QLabel(playbackScrollAreaWidgetContents);
    playbackFormLayout->setWidget(7, QFormLayout::ItemRole::LabelRole, desiredVideoHeightL);
    m_desiredVideoHeightB = new QComboBox(playbackScrollAreaWidgetContents);
    m_desiredVideoHeightB->addItem(QString());
    m_desiredVideoHeightB->addItem(QString());
    m_desiredVideoHeightB->addItem(QString());
    m_desiredVideoHeightB->addItem(QString());
    m_desiredVideoHeightB->addItem(QString());
    playbackFormLayout->setWidget(7, QFormLayout::ItemRole::FieldRole, m_desiredVideoHeightB);

    QLabel *maxVolL = new QLabel(playbackScrollAreaWidgetContents);
    playbackFormLayout->setWidget(8, QFormLayout::ItemRole::LabelRole, maxVolL);
    m_maxVolB = new QSpinBox(playbackScrollAreaWidgetContents);
    m_maxVolB->setSuffix(QString::fromUtf8(" %"));
    m_maxVolB->setMinimum(100);
    m_maxVolB->setMaximum(1000);
    m_maxVolB->setSingleStep(50);
    playbackFormLayout->setWidget(8, QFormLayout::ItemRole::FieldRole, m_maxVolB);

    m_forceSamplerate = new QCheckBox(playbackScrollAreaWidgetContents);
    playbackFormLayout->setWidget(9, QFormLayout::ItemRole::LabelRole, m_forceSamplerate);
    m_samplerateB = new QSpinBox(playbackScrollAreaWidgetContents);
    m_samplerateB->setMinimum(4000);
    m_samplerateB->setMaximum(192000);
    playbackFormLayout->setWidget(9, QFormLayout::ItemRole::FieldRole, m_samplerateB);

    m_forceChannels = new QCheckBox(playbackScrollAreaWidgetContents);
    m_forceChannels->setTristate(true);
    playbackFormLayout->setWidget(10, QFormLayout::ItemRole::LabelRole, m_forceChannels);
    m_channelsB = new QSpinBox(playbackScrollAreaWidgetContents);
    m_channelsB->setMinimum(1);
    m_channelsB->setMaximum(8);
    playbackFormLayout->setWidget(10, QFormLayout::ItemRole::FieldRole, m_channelsB);

    m_resamplerFirst = new QCheckBox(playbackScrollAreaWidgetContents);
    playbackFormLayout->setWidget(11, QFormLayout::ItemRole::SpanningRole, m_resamplerFirst);

    m_replayGain = new QGroupBox(playbackScrollAreaWidgetContents);
    m_replayGain->setCheckable(true);
    QVBoxLayout *replayGainLayout = new QVBoxLayout(m_replayGain);
    replayGainLayout->setSpacing(1);
    replayGainLayout->setContentsMargins(3, 3, 3, 3);
    m_replayGainAlbum = new QCheckBox(m_replayGain);
    replayGainLayout->addWidget(m_replayGainAlbum);
    m_replayGainPreventClipping = new QCheckBox(m_replayGain);
    replayGainLayout->addWidget(m_replayGainPreventClipping);
    m_replayGainPreamp = new QDoubleSpinBox(m_replayGain);
    m_replayGainPreamp->setSuffix(QString::fromUtf8(" dB"));
    m_replayGainPreamp->setMinimum(-15.0);
    m_replayGainPreamp->setMaximum(15.0);
    replayGainLayout->addWidget(m_replayGainPreamp);
    QFrame *replayGainLine = new QFrame(m_replayGain);
    replayGainLine->setFrameShape(QFrame::Shape::HLine);
    replayGainLine->setFrameShadow(QFrame::Shadow::Sunken);
    replayGainLayout->addWidget(replayGainLine);
    m_replayGainPreampNoMetadata = new QDoubleSpinBox(m_replayGain);
    m_replayGainPreampNoMetadata->setSuffix(QString::fromUtf8(" dB"));
    m_replayGainPreampNoMetadata->setMinimum(-15.0);
    m_replayGainPreampNoMetadata->setMaximum(0.0);
    replayGainLayout->addWidget(m_replayGainPreampNoMetadata);

    m_wheelActionB = new QGroupBox(playbackScrollAreaWidgetContents);
    m_wheelActionB->setCheckable(true);
    QVBoxLayout *wheelActionLayout = new QVBoxLayout(m_wheelActionB);
    wheelActionLayout->setSpacing(1);
    wheelActionLayout->setContentsMargins(3, 3, 3, 3);
    m_wheelSeekB = new QRadioButton(m_wheelActionB);
    wheelActionLayout->addWidget(m_wheelSeekB);
    m_wheelVolumeB = new QRadioButton(m_wheelActionB);
    wheelActionLayout->addWidget(m_wheelVolumeB);

    QSpacerItem *playbackHorizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

    m_storeARatioAndZoomB = new QCheckBox(playbackScrollAreaWidgetContents);
    m_keepZoom = new QCheckBox(playbackScrollAreaWidgetContents);
    m_keepARatio = new QCheckBox(playbackScrollAreaWidgetContents);
    m_showBufferedTimeOnSlider = new QCheckBox(playbackScrollAreaWidgetContents);
    m_savePos = new QCheckBox(playbackScrollAreaWidgetContents);
    m_keepSubtitlesDelay = new QCheckBox(playbackScrollAreaWidgetContents);
    m_keepSubtitlesScale = new QCheckBox(playbackScrollAreaWidgetContents);
    m_keepSpeed = new QCheckBox(playbackScrollAreaWidgetContents);
    m_keepVideoDelay = new QCheckBox(playbackScrollAreaWidgetContents);
    m_syncVtoA = new QCheckBox(playbackScrollAreaWidgetContents);
    m_silence = new QCheckBox(playbackScrollAreaWidgetContents);
    m_restoreVideoEq = new QCheckBox(playbackScrollAreaWidgetContents);
    m_ignorePlaybackError = new QCheckBox(playbackScrollAreaWidgetContents);
    m_leftMouseTogglePlay = new QCheckBox(playbackScrollAreaWidgetContents);
    m_leftMouseTogglePlay->setTristate(true);
    m_middleMouseToggleFullscreen = new QCheckBox(playbackScrollAreaWidgetContents);
    m_accurateSeekB = new QCheckBox(playbackScrollAreaWidgetContents);
    m_accurateSeekB->setTristate(true);
    m_unpauseWhenSeekingB = new QCheckBox(playbackScrollAreaWidgetContents);
    m_restoreAVSStateB = new QCheckBox(playbackScrollAreaWidgetContents);
    m_disableSubtitlesAtStartup = new QCheckBox(playbackScrollAreaWidgetContents);

    QSpacerItem *playbackVerticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

    m_storeUrlPosB = new QCheckBox(playbackScrollAreaWidgetContents);

    playbackGridLayout->addLayout(playbackFormLayout, 0, 0, 1, 2);
    playbackGridLayout->addWidget(m_replayGain, 1, 0, 1, 1);
    playbackGridLayout->addWidget(m_wheelActionB, 1, 1, 1, 1);
    playbackGridLayout->addItem(playbackHorizontalSpacer, 1, 2, 1, 1);
    playbackGridLayout->addWidget(m_storeARatioAndZoomB, 2, 0, 1, 1);
    playbackGridLayout->addWidget(m_keepZoom, 3, 0, 1, 2);
    playbackGridLayout->addWidget(m_keepARatio, 5, 0, 1, 2);
    playbackGridLayout->addWidget(m_showBufferedTimeOnSlider, 7, 0, 1, 1);
    playbackGridLayout->addWidget(m_savePos, 8, 0, 1, 2);
    playbackGridLayout->addWidget(m_keepSubtitlesDelay, 9, 0, 1, 2);
    playbackGridLayout->addWidget(m_keepSubtitlesScale, 10, 0, 1, 2);
    playbackGridLayout->addWidget(m_keepSpeed, 11, 0, 1, 2);
    playbackGridLayout->addWidget(m_keepVideoDelay, 12, 0, 1, 2);
    playbackGridLayout->addWidget(m_syncVtoA, 13, 0, 1, 2);
    playbackGridLayout->addWidget(m_silence, 14, 0, 1, 2);
    playbackGridLayout->addWidget(m_restoreVideoEq, 15, 0, 1, 2);
    playbackGridLayout->addWidget(m_ignorePlaybackError, 17, 0, 1, 2);
    playbackGridLayout->addWidget(m_leftMouseTogglePlay, 18, 0, 1, 1);
    playbackGridLayout->addWidget(m_middleMouseToggleFullscreen, 19, 0, 1, 1);
    playbackGridLayout->addWidget(m_accurateSeekB, 20, 0, 1, 1);
    playbackGridLayout->addWidget(m_unpauseWhenSeekingB, 21, 0, 1, 1);
    playbackGridLayout->addWidget(m_restoreAVSStateB, 22, 0, 1, 1);
    playbackGridLayout->addWidget(m_disableSubtitlesAtStartup, 23, 0, 1, 1);
    playbackGridLayout->addItem(playbackVerticalSpacer, 24, 1, 1, 1);
    playbackGridLayout->addWidget(m_storeUrlPosB, 25, 0, 1, 1);

    playbackScrollArea->setWidget(playbackScrollAreaWidgetContents);
    playbackLayout->addWidget(playbackScrollArea);

    m_modulesListLayout = new QHBoxLayout();
    playbackLayout->addLayout(m_modulesListLayout);

    QObject::connect(m_restoreAVSStateB, &QCheckBox::toggled, m_disableSubtitlesAtStartup, &QCheckBox::setDisabled);

    m_replayGain->setTitle(tr("Use available replay gain"));
    m_replayGainAlbum->setText(tr("Album mode for replay gain"));
    m_replayGainPreventClipping->setText(tr("Prevent clipping"));
    m_replayGainPreamp->setPrefix(tr("Amplify"));
    m_replayGainPreampNoMetadata->setPrefix(tr("Amplify (no metadata)"));
    m_leftMouseTogglePlay->setToolTip(tr("Partially checked means that there is a delay between click and pausing"));
    m_leftMouseTogglePlay->setText(tr("Primary mouse button on video dock toggles playback"));
    m_middleMouseToggleFullscreen->setText(tr("Middle mouse button on video dock toggles fullscreen"));
    m_keepVideoDelay->setText(tr("Keep video delay"));
    m_silence->setText(tr("Fade sound"));
    m_keepSpeed->setText(tr("Keep speed"));
    m_keepZoom->setText(tr("Keep zoom"));
    m_keepSubtitlesScale->setText(tr("Keep subtitles scale"));
    m_keepSubtitlesDelay->setText(tr("Keep subtitles delay"));
    m_syncVtoA->setText(tr("Video to audio sync (frame skipping)"));
    shortSeekL->setText(tr("Short seeking (left and right arrows): "));
    m_shortSeekB->setSuffix(tr("sec"));
    longSeekL->setText(tr("Long seeking (up and down arrows): "));
    m_longSeekB->setSuffix(tr("sec"));
    bufferLocalL->setText(tr("Local buffer size (A/V packages count): "));
    bufferNetworkL->setText(tr("Network buffer length: "));
    m_bufferNetworkB->setSuffix(tr("sec"));
    backwardBufferNetworkL->setText(tr("Percent of packages for backwards rewinding: "));
    bufferLiveL->setText(tr("Live stream buffer length: "));
    m_bufferLiveB->setSuffix(tr("sec"));
    playIfBufferedL->setText(tr("Start playback internet stream if it is buffered: "));
    m_playIfBufferedB->setSuffix(tr("sec"));
    desiredVideoHeightL->setText(tr("Desired video stream quality: "));
    m_desiredVideoHeightB->setItemText(0, tr("Default"));
    m_desiredVideoHeightB->setItemText(1, tr("SD"));
    m_desiredVideoHeightB->setItemText(2, tr("HD"));
    m_desiredVideoHeightB->setItemText(3, tr("Full HD"));
    m_desiredVideoHeightB->setItemText(4, tr("4K"));
    maxVolL->setText(tr("Maximum volume: "));
    m_forceSamplerate->setText(tr("Force samplerate: "));
    m_forceChannels->setText(tr("Force channels conversion: "));
    m_resamplerFirst->setText(tr("Use audio resampler and channel conversion before filters and visualizations"));
    m_keepARatio->setText(tr("Keep aspect ratio"));
    m_accurateSeekB->setText(tr("Accurate seeking"));
    m_ignorePlaybackError->setText(tr("Play next entry after playback error"));
    m_savePos->setText(tr("Continue last playback when program starts"));
    m_wheelActionB->setTitle(tr("Mouse wheel action on video dock"));
    m_wheelSeekB->setText(tr("Mouse wheel scrolls music/movie"));
    m_wheelVolumeB->setText(tr("Mouse wheel changes the volume"));
    m_restoreVideoEq->setText(tr("Remember video equalizer settings"));
    m_showBufferedTimeOnSlider->setText(tr("Show buffered data indicator on slider"));
    m_storeARatioAndZoomB->setText(tr("Store aspect ratio and zoom in config file"));
    m_unpauseWhenSeekingB->setText(tr("Unpause when seeking"));
    m_disableSubtitlesAtStartup->setText(tr("Disable subtitles at program startup"));
    m_restoreAVSStateB->setText(tr("Remember audio/video/subtitles enabled state"));
    m_storeUrlPosB->setText(tr("Remember playback position for each playlist entry"));
    m_storeUrlPosB->setToolTip(tr("The length must be at least 8 minutes. Your playback position must be in [1% - 99%] of the playback range. You can continue playback by pressing the icon next to the full screen button."));
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

    auto canRestart = std::make_shared<bool>(false);

    m_rendererApplyFunctions.push_back([=](bool &initFilters) {
        Q_UNUSED(initFilters)
        const auto rendererName = renderers->currentData().toString();
        settings->set("Renderer", rendererName);
        if (currentRendererName != rendererName && (*canRestart || chosenRenderer != rendererName))
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
        layout->setContentsMargins(3, 3, 3, 3);
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
        bypassCompositor->setToolTip(tr("This can improve performance. Some video drivers can crash when enabled."));
#else
        if (QGuiApplication::platformName() != "xcb")
            bypassCompositor->setEnabled(false);
#endif

        glOnWindow->setToolTip(tr(
            "Use QOpenGLWidget (render-to-texture), also enable OpenGL for visualizations. "
            "Use with caution, it can reduce performance of video playback."
        ));

        connect(glOnWindow, &QCheckBox::toggled,
                this, [=](bool checked) {
            vsync->setEnabled(!checked);
#ifdef Q_OS_WIN
            bypassCompositor->setEnabled(!checked);
#endif
        });

        glOnWindow->setChecked(settings->getBool("OpenGL/OnWindow"));
        vsync->setChecked(settings->getBool("OpenGL/VSync"));
        bypassCompositor->setChecked(settings->getBool("OpenGL/BypassCompositor"));

        auto layout = new QFormLayout(openglSettings);
        layout->setContentsMargins(3, 3, 3, 3);
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
        auto nearestScaling = new QCheckBox(tr("Low quality image scaling (nearest neighbor)"));
        auto hqDownscale = new QCheckBox(tr("High quality image scaling down"));
        auto hqUpscale = new QCheckBox(tr("High quality image scaling up"));
        auto bypassCompositor = createBypassCompositor();
        auto hdr = new QCheckBox(tr("Try to display HDR10 videos in HDR mode (experimental)"));

        auto selectedDeviceFiltersEnabled = std::make_shared<int>(-1);

        connect(nearestScaling, &QCheckBox::toggled,
                this, [=](bool checked) {
            hqDownscale->setEnabled(!checked);
            hqUpscale->setEnabled(!checked);
        });

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

            if (*selectedDeviceFiltersEnabled == -1)
                *selectedDeviceFiltersEnabled = filtersEnabled;

#ifdef Q_OS_WIN
            bypassCompositor->setEnabled(!noExclusiveFullScreenDevIDs->contains(devices->itemData(idx).toByteArray()));
            hdr->setVisible(QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10);
#else
            hdr->setVisible(QGuiApplication::platformName().contains("wayland"));
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
                *canRestart = true;
            }
            else
            {
                devices->addItem(tr("No supported devices found"));
                vulkanSetttings->setEnabled(false);
            }
        });

        vsync->setTristate(true);
        vsync->setToolTip(tr(
            "Partially checked:\n"
            "  - MAILBOX (tear-free) is the preferred present mode\n"
            "  - FIFO (V-Sync) should not be used in windowed mode"
        ));

#ifdef Q_OS_WIN
        bypassCompositor->setToolTip(tr("Allow for exclusive fullscreen. This can improve performance."));
#else
        if (QGuiApplication::platformName() != "xcb")
            bypassCompositor->setEnabled(false);
#endif

        vsync->setCheckState(settings->getWithBounds("Vulkan/VSync", Qt::Unchecked, Qt::Checked));
        gpuDeint->setChecked(settings->getBool("Vulkan/AlwaysGPUDeint"));
        forceYadif->setChecked(settings->getBool("Vulkan/ForceVulkanYadif"));
        nearestScaling->setChecked(settings->getBool("Vulkan/NearestScaling"));
        hqDownscale->setChecked(settings->getBool("Vulkan/HQScaleDown"));
        hqUpscale->setChecked(settings->getBool("Vulkan/HQScaleUp"));
        bypassCompositor->setChecked(settings->getBool("Vulkan/BypassCompositor"));
        hdr->setChecked(settings->getBool("Vulkan/HDR"));

        nearestScaling->setToolTip(tr("Useful for retro scaling. Can also be used for software Vulkan implementation to lower the CPU overhead."));
        hqUpscale->setToolTip(tr("Very slow if used with sharpness"));

        auto layout = new QFormLayout(vulkanSetttings);
        layout->setContentsMargins(3, 3, 3, 3);
        layout->addRow(tr("Device:"), devices);
        layout->addRow(vsync);
        layout->addRow(gpuDeint);
        layout->addRow(forceYadif);
        layout->addRow(nearestScaling);
        layout->addRow(hqDownscale);
        layout->addRow(hqUpscale);
        layout->addRow(bypassCompositor);
        layout->addRow(hdr);

        m_rendererApplyFunctions.push_back([=](bool &initFilters) {
            const bool filtersEnabled = gpuDeint->isEnabled();
            const bool alwaysGPUDeint = gpuDeint->isChecked();

            if (QMPlay2Core.isVulkanRenderer() && (*selectedDeviceFiltersEnabled == true && settings->getBool("Vulkan/AlwaysGPUDeint")) != (alwaysGPUDeint && filtersEnabled))
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
            settings->set("Vulkan/NearestScaling", nearestScaling->isChecked());
            settings->set("Vulkan/HQScaleDown", hqDownscale->isChecked());
            settings->set("Vulkan/HQScaleUp", hqUpscale->isChecked());
            settings->set("Vulkan/BypassCompositor", bypassCompositor->isChecked());
            settings->set("Vulkan/HDR", hdr->isChecked());
            if (renderers->currentData().toString() == "vulkan")
                settings->set("Vulkan/UserApplied", true);

            *selectedDeviceFiltersEnabled = filtersEnabled;
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
    layout->setContentsMargins(1, 1, 1, 1);
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
        if (videoEqOriginalParent)
        {
            videoEq->setGeometry(videoEqOriginalParent->rect());
            videoEqOriginalParent->setEnabled(true);
        }
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

QString SettingsWidget::chooseDir(const QString &currPth)
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Choose directory"), currPth);
    if (!dir.isEmpty())
    {
        const QFileInfo dirInfo(dir);
#if !defined(Q_OS_WIN) && !defined(Q_OS_ANDROID)
        if (dirInfo.isDir() && dirInfo.isWritable())
#else
        if (dirInfo.isDir())
#endif
        {
            if (Version::isPortable() && dirInfo.isAbsolute())
            {
                const auto appDirPath = QCoreApplication::applicationDirPath();
                if (auto dirRelative = QDir::cleanPath(dir); dirRelative.startsWith(appDirPath))
                {
                    dirRelative.remove(0, appDirPath.size());
                    dirRelative.prepend(".");
                    return dirRelative;
                }
            }
            return dir;
        }
        QMessageBox::warning(this, QString(), tr("Cannot change the directory"));
    }
    return QString();
}

inline QString SettingsWidget::getSelectedProfile()
{
    return !m_profileB->currentIndex() ? "/" : m_profileB->currentText();
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
    const QString newStyle = m_styleBox->currentText().toLower();
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
            if (QMPlay2Core.getLanguage() != m_langBox->itemData(m_langBox->currentIndex()).toString())
            {
                QMPSettings.set("Language", m_langBox->itemData(m_langBox->currentIndex()).toString());
                QMPlay2Core.setLanguage();
                QMessageBox::information(this, tr("New language"), tr("To set up a new language, the program will start again!"));
                restartApp();
            }
#ifdef ICONS_FROM_THEME
            if (m_iconsFromTheme->isChecked() != QMPSettings.getBool("IconsFromTheme"))
            {
                QMPSettings.set("IconsFromTheme", m_iconsFromTheme->isChecked());
                if (!QMPlay2GUI.restartApp)
                {
                    QMessageBox::information(this, tr("Changing icons"), tr("To apply the icons change, the program will start again!"));
                    restartApp();
                }
            }
#endif
            QMPSettings.set("FallbackSubtitlesEncoding", m_encodingB->currentText().toUtf8());
            QMPSettings.set("AudioLanguage", m_audioLangB->currentIndex() > 0 ? m_audioLangB->currentText() : QString());
            QMPSettings.set("SubtitlesLanguage", m_subsLangB->currentIndex() > 0 ? m_subsLangB->currentText() : QString());
            QMPSettings.set("screenshotPth", m_screenshotE->text());
            QMPSettings.set("screenshotFormat", m_screenshotFormatB->currentText());
            QMPSettings.set("OutputFilePath", m_outputFileE->text());
            QMPSettings.set("ShowCovers", m_showCoversGB->isChecked());
            QMPSettings.set("BlurCovers", m_blurCoversB->isChecked());
            QMPSettings.set("ShowDirCovers", m_showDirCoversB->isChecked());
            QMPSettings.set("EnlargeCovers", m_enlargeSmallCoversB->isChecked());
            QMPSettings.set("AutoOpenVideoWindow", m_autoOpenVideoWindowB->isChecked());
            QMPSettings.set("AutoRestoreMainWindowOnVideo", m_autoRestoreMainWindowOnVideoB->isChecked());
#ifdef UPDATES
            QMPSettings.set("AutoUpdates", m_autoUpdatesB->isChecked());
#endif
            if (auto checked = m_keepDocksSizeB->isChecked(); checked != QMPSettings.getBool("MainWidget/KeepDocksSize"))
            {
                QMPSettings.set("MainWidget/KeepDocksSize", checked);
                emit keepDocksSizeChanged(checked);
            }
            QMPSettings.set("HideLogo", m_hideLogoB->isChecked());
            if (auto checked = m_fullscreenPanelsRightSideB->isChecked(); checked != QMPSettings.getBool("FullscreenPanelsRightSide"))
            {
                QMPSettings.set("FullscreenPanelsRightSide", checked);
                QMPSettings.remove("MainWidget/FullScreenDockWidgetState");
                QMPSettings.remove("MainWidget/CompactViewDockWidgetState");
                emit fullscreenPanelsRightSideChanged(checked);
            }
            QMPSettings.set("MainWidget/TabPositionNorth", m_tabsNorths->isChecked());
#ifdef QMPLAY2_ALLOW_ONLY_ONE_INSTANCE
            QMPSettings.set("AllowOnlyOneInstance", m_allowOnlyOneInstance->isChecked());
#endif
            QMPSettings.set("HideArtistMetadata", m_hideArtistMetadata->isChecked());
            QMPSettings.set("DisplayOnlyFileName", m_displayOnlyFileName->isChecked());
            QMPSettings.set("SkipPlaylistsWithinFiles", m_skipPlaylistsWithinFiles->isChecked());
            QMPSettings.set("RestoreRepeatMode", m_restoreRepeatMode->isChecked());
            QMPSettings.set("StillImages", m_stillImages->isChecked());
            if (m_trayNotifiesDefault)
                QMPSettings.set("TrayNotifiesDefault", m_trayNotifiesDefault->isChecked());
            QMPSettings.set("AutoDelNonGroupEntries", m_autoDelNonGroupEntries->isChecked());
            QMPSettings.set("Proxy/Use", m_proxyB->isChecked() && !m_proxyHostE->text().isEmpty());
            QMPSettings.set("Proxy/Host", m_proxyHostE->text());
            QMPSettings.set("Proxy/Port", m_proxyPortB->value());
            QMPSettings.set("Proxy/Login", m_proxyLoginB->isChecked() && !m_proxyUserE->text().isEmpty());
            QMPSettings.set("Proxy/User", m_proxyUserE->text());
            QMPSettings.set("Proxy/Password", m_proxyPasswordE->text().toUtf8().toBase64());
            m_proxyB->setChecked(QMPSettings.getBool("Proxy/Use"));
            m_proxyLoginB->setChecked(QMPSettings.getBool("Proxy/Login"));
            qobject_cast<QMainWindow *>(QMPlay2GUI.mainW)->setTabPosition(Qt::AllDockWidgetAreas, m_tabsNorths->isChecked() ? QTabWidget::North : QTabWidget::South);
            applyProxy();

#ifdef USE_YOUTUBEDL
            QMPSettings.set("YtDl/CookiesFromBrowserEnabled", m_cookiesFromBrowserCB->isChecked() && !m_cookiesFromBrowserE->text().simplified().isEmpty());
            QMPSettings.set("YtDl/CookiesFromBrowser", m_cookiesFromBrowserE->text());
            QMPSettings.set("YtDl/CustomPathEnabled", m_customYtDlCB->isChecked() && !m_customYtDlE->text().trimmed().isEmpty());
            QMPSettings.set("YtDl/CustomPath", m_customYtDlE->text());
            QMPSettings.set("YtDl/DefaultQualityEnabled", m_dfltYtDlQualCB->isChecked() && !m_dfltYtDlQualE->text().simplified().isEmpty());
            QMPSettings.set("YtDl/DefaultQuality", m_dfltYtDlQualE->text());
            QMPSettings.set("YtDl/AdditionalParamsEnabled", m_additionalYtDlParamsCB->isChecked() && !m_additionalYtDlParamsE->text().simplified().isEmpty());
            QMPSettings.set("YtDl/AdditionalParams", m_additionalYtDlParamsE->text());
            QMPSettings.set("YtDl/DontAutoUpdate", m_dontUpdateYtDlCB->isChecked());
#endif

            if (m_trayNotifiesDefault)
                Notifies::setNativeFirst(!m_trayNotifiesDefault->isChecked());

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
            for (QListWidgetItem *wI : modulesList[0].list->findItems(QString(), Qt::MatchContains))
                videoWriters += wI->text();
            QMPSettings.set("videoWriters", videoWriters);

            page6->deintSettingsW->setDeintEnabledDisabled();

            break;
        }
        case 2:
        {
            auto getDesiredVideoHeight = [this] {
                switch (m_desiredVideoHeightB->currentIndex())
                {
                    case 1:
                        return 480;
                    case 2:
                        return 720;
                    case 3:
                        return 1080;
                    case 4:
                        return 2160;
                    default:
                        return 0;
                }
            };

            QMPSettings.set("ShortSeek", m_shortSeekB->value());
            QMPSettings.set("LongSeek", m_longSeekB->value());
            QMPSettings.set("AVBufferLocal", m_bufferLocalB->value());
            QMPSettings.set("AVBufferTimeNetwork", m_bufferNetworkB->value());
            QMPSettings.set("BackwardBuffer", m_backwardBufferNetworkB->currentIndex());
            QMPSettings.set("AVBufferTimeNetworkLive", m_bufferLiveB->value());
            QMPSettings.set("PlayIfBuffered", m_playIfBufferedB->value());
            QMPSettings.set("DesiredVideoHeight", getDesiredVideoHeight());
            QMPSettings.set("ForceSamplerate", m_forceSamplerate->isChecked());
            QMPSettings.set("Samplerate", m_samplerateB->value());
            QMPSettings.set("MaxVol", m_maxVolB->value());
            QMPSettings.set("ForceChannels", m_forceChannels->checkState());
            QMPSettings.set("Channels", m_channelsB->value());
            QMPSettings.set("ResamplerFirst", m_resamplerFirst->isChecked());
            QMPSettings.set("ReplayGain/Enabled", m_replayGain->isChecked());
            QMPSettings.set("ReplayGain/Album", m_replayGainAlbum->isChecked());
            QMPSettings.set("ReplayGain/PreventClipping", m_replayGainPreventClipping->isChecked());
            QMPSettings.set("ReplayGain/Preamp", m_replayGainPreamp->value());
            QMPSettings.set("ReplayGain/PreampNoMetadata", m_replayGainPreampNoMetadata->value());
            QMPSettings.set("WheelAction", m_wheelActionB->isChecked());
            QMPSettings.set("WheelSeek", m_wheelSeekB->isChecked());
            QMPSettings.set("WheelVolume", m_wheelVolumeB->isChecked());
            QMPSettings.set("ShowBufferedTimeOnSlider", m_showBufferedTimeOnSlider->isChecked());
            QMPSettings.set("SavePos", m_savePos->isChecked());
            QMPSettings.set("KeepZoom", m_keepZoom->isChecked());
            QMPSettings.set("KeepARatio", m_keepARatio->isChecked());
            QMPSettings.set("KeepSubtitlesDelay", m_keepSubtitlesDelay->isChecked());
            QMPSettings.set("KeepSubtitlesScale", m_keepSubtitlesScale->isChecked());
            QMPSettings.set("KeepVideoDelay", m_keepVideoDelay->isChecked());
            QMPSettings.set("KeepSpeed", m_keepSpeed->isChecked());
            QMPSettings.set("SyncVtoA", m_syncVtoA->isChecked());
            QMPSettings.set("Silence", m_silence->isChecked());
            QMPSettings.set("RestoreVideoEqualizer", m_restoreVideoEq->isChecked());
            QMPSettings.set("IgnorePlaybackError", m_ignorePlaybackError->isChecked());
            QMPSettings.set("LeftMouseTogglePlay", m_leftMouseTogglePlay->checkState());
            QMPSettings.set("MiddleMouseToggleFullscreen", m_middleMouseToggleFullscreen->isChecked());
            QMPSettings.set("AccurateSeek", m_accurateSeekB->checkState());
            QMPSettings.set("UnpauseWhenSeeking", m_unpauseWhenSeekingB->isChecked());
            QMPSettings.set("RestoreAVSState", m_restoreAVSStateB->isChecked());
            QMPSettings.set("DisableSubtitlesAtStartup", m_disableSubtitlesAtStartup->isChecked());
            QMPSettings.set("StoreUrlPos", m_storeUrlPosB->isChecked());
            QMPSettings.set("StoreARatioAndZoom", m_storeARatioAndZoomB->isChecked());

            QStringList audioWriters, decoders;
            for (QListWidgetItem *wI : modulesList[1].list->findItems(QString(), Qt::MatchContains))
                audioWriters += wI->text();
            for (QListWidgetItem *wI : modulesList[2].list->findItems(QString(), Qt::MatchContains))
                decoders += wI->text();
            QMPSettings.set("audioWriters", audioWriters);
            QMPSettings.set("decoders", decoders);

            emit setWheelStep(m_shortSeekB->value());
            emit setVolMax(m_maxVolB->value());

            SetAudioChannelsMenu();
        } break;
        case 3:
            if (page3->module && page3->scrollA->widget())
            {
                Module::SettingsWidget *settingsWidget = (Module::SettingsWidget *)page3->scrollA->widget();
                settingsWidget->saveSettings();
                settingsWidget->flushSettings();
                page3->module->setInstances(forceRestartPlayback);
                const int scroll = page3->scrollA->verticalScrollBar()->value();
                chModule(page3->listW->currentItem());
                page3->scrollA->verticalScrollBar()->setValue(scroll);
            }
            break;
        case 4:
            page4->writeSettings();
            QMPSettings.set("ApplyToASS/ColorsAndBorders", page4->colorsAndBordersB->isChecked());
            QMPSettings.set("ApplyToASS/MarginsAndAlignment", page4->marginsAndAlignmentB->isChecked());
            QMPSettings.set("ApplyToASS/FontsAndSpacing", page4->fontsB->isChecked());
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
            layout->setContentsMargins(2, 2, 2, 2);
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

    if ((idx == 1 || idx == 2) && !modulesList[0].list->count() && !modulesList[1].list->count() && !modulesList[2].list->count())
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
                modulesList[m].list->addItem(wI);
                if (writers[m][i] == lastM[m])
                    modulesList[m].list->setCurrentItem(wI);
            }
            if (modulesList[m].list->currentRow() < 0)
                modulesList[m].list->setCurrentRow(0);
        }
    }
    else if (idx == 3)
    {
        for (int m = 0; m < 3; ++m)
        {
            QListWidgetItem *currI = modulesList[m].list->currentItem();
            if (currI)
                lastM[m] = currI->text();
            modulesList[m].list->clear();
        }
    }

    if (idx != 6)
    {
        restoreVideoEq();
    }
    else
    {
        QGridLayout *videoEqLayout = new QGridLayout(page6->videoEqContainer);
        videoEqLayout->setContentsMargins(0, 0, 0, 0);
        videoEqLayout->addWidget(videoEq);
        if (videoEqOriginalParent)
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
        QListWidget *mL = modulesList[idx].list;
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
    const QString dir = chooseDir(m_screenshotE->text());
    if (!dir.isEmpty())
        m_screenshotE->setText(dir);
}
void SettingsWidget::chooseOutputFileDir()
{
    const QString dir = chooseDir(m_outputFileE->text());
    if (!dir.isEmpty())
        m_outputFileE->setText(dir);
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
    m_profileRemoveB->setEnabled(index != 0);
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

        m_profileB->removeItem(m_profileB->currentIndex());
        for (QAction *act : QMPlay2GUI.menuBar->options->profilesGroup->actions())
            if (act->text() == selectedProfileName)
            {
                delete act;
                break;
            }
    }
}
