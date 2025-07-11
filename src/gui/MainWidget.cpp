/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2025  Błażej Szczygieł

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

#include <QApplication>
#include <QToolBar>
#include <QFrame>
#include <QSlider>
#include <QToolButton>
#include <QStatusBar>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QShortcut>
#include <QFileDialog>
#include <QTreeWidget>
#include <QListWidget>
#include <QWindow>
#include <QScreen>
#include <QActionGroup>
#include <QProxyStyle>
#ifdef Q_OS_MACOS
    #include <QProcess>
#endif
#if defined(Q_OS_WIN)
    #include <QOperatingSystemVersion>
    #include <QtWinExtras/qwinthumbnailtoolbutton.h>
    #include <QtWinExtras/qwinthumbnailtoolbar.h>
    #include <QtWinExtras/qwintaskbarprogress.h>
    #include <QtWinExtras/qwintaskbarbutton.h>
#endif
#if defined(Q_OS_WIN) && QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
    #include <dwmapi.h>
#endif
#include <qevent.h>

/* QMPlay2 gui */
#include <Main.hpp>
#include <Notifies.hpp>
#include <Functions.hpp>
#include <Appearance.hpp>
#include <MainWidget.hpp>
#include <SettingsWidget.hpp>
#include <VideoDock.hpp>
#include <InfoDock.hpp>
#include <PlaylistDock.hpp>
#include <Slider.hpp>
#include <Playlist.hpp>
#include <AboutWidget.hpp>
#include <AddressDialog.hpp>
#include <VideoAdjustmentW.hpp>
#include <ShortcutHandler.hpp>
#include <VolWidget.hpp>
#include <ScreenSaver.hpp>
#ifdef Q_OS_MACOS
    #include <QMPlay2MacExtensions.hpp>
#endif

using Functions::timeToStr;

/* QMPlay2 lib */
#include <QMPlay2Extensions.hpp>
#include <Settings.hpp>
#include <MenuBar.hpp>
#include <SubsDec.hpp>
#include <IPC.hpp>

#include <cmath>

/* MainWidgetTmpStyle -  dock widget separator extent must be larger for touch screens */
class MainWidgetTmpStyle final : public QProxyStyle
{
public:
    MainWidgetTmpStyle(QObject *parent)
    {
        setParent(parent);
    }
    ~MainWidgetTmpStyle() = default;

    int pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const override
    {
        const int pM = QProxyStyle::pixelMetric(metric, option, widget);
        if (metric == QStyle::PM_DockWidgetSeparatorExtent)
            return pM * 5 / 2;
        return pM;
    }
};

#ifndef Q_OS_MACOS
static void copyMenu(QMenu *dest, QMenu *src, const QSet<QMenu *> &dontCopy = {}, const QSet<QMenu *> &dontAdd = {})
{
    QMenu *newMenu = new QMenu(src->title(), dest);
    for (QAction *act : src->actions())
    {
        QMenu *menu = act->menu();
        if (!menu)
        {
            if (!act->isSeparator())
            {
                newMenu->addAction(act);
            }
            else
            {
                const auto actions = newMenu->actions();
                if (!actions.isEmpty() && !actions.constLast()->isSeparator())
                    newMenu->addAction(act);
            }
        }
        else if (dontAdd.contains(menu))
        {
            continue;
        }
        else if (!dontCopy.contains(menu))
        {
            copyMenu(newMenu, menu, dontCopy, dontAdd);
        }
        else
        {
            newMenu->addMenu(menu);
        }
    }
    dest->addMenu(newMenu);
}
#endif

/* MainWidget */
MainWidget::MainWidget(QList<QPair<QString, QString>> &arguments)
#ifdef UPDATES
    : updater(this)
#endif
{
    QMPlay2GUI.videoAdjustment = new VideoAdjustmentW;
    QMPlay2GUI.shortcutHandler = new ShortcutHandler(this);
    QMPlay2GUI.mainW = this;

    /* Looking for touch screen */
    if (Functions::hasTouchScreen())
    {
        setStyle(new MainWidgetTmpStyle(this));
    }

    setObjectName("MainWidget");

    settingsW = nullptr;
    aboutW = nullptr;

    isCompactView = wasShow = fullScreen = seekSFocus = false;

    if (QMPlay2GUI.pipe)
        connect(QMPlay2GUI.pipe.get(), SIGNAL(newConnection(IPCSocket *)), this, SLOT(newConnection(IPCSocket *)));

    Settings &settings = QMPlay2Core.getSettings();

    if (!settings.getBool("IconsFromTheme"))
        setIconSize({22, 22});

    SettingsWidget::InitSettings();
    settings.init("MainWidget/WidgetsLocked", true);

    QMPlay2GUI.menuBar = new MenuBar;

#if !defined Q_OS_MACOS && !defined Q_OS_ANDROID
    tray = new QSystemTrayIcon(this);
    tray->setIcon(QMPlay2Core.getIconFromTheme("QMPlay2-panel", QMPlay2Core.getQMPlay2Icon()));
    tray->setVisible(settings.getBool("TrayVisible", true));
    tray->installEventFilter(this);
#else
    tray = nullptr;
#endif
    Notifies::initialize(tray);
    if (Notifies::hasBoth())
        Notifies::setNativeFirst(!settings.getBool("TrayNotifiesDefault"));

    setDockOptions(AllowNestedDocks | AnimatedDocks | AllowTabbedDocks);
    setMouseTracking(true);
    updateWindowTitle();

    statusBar = new QStatusBar;
#ifdef Q_OS_WIN
    statusBar->setSizeGripEnabled(true);
#else
    statusBar->setSizeGripEnabled(false);
#endif
    timeL = new QLabel;
    statusBar->addPermanentWidget(timeL);
    QLabel *stateL = new QLabel(tr("Stopped"));
    statusBar->addWidget(stateL);
    setStatusBar(statusBar);

    videoDock = new VideoDock;
    videoDock->setObjectName("videoDock");

    playlistDock = new PlaylistDock;
    playlistDock->setObjectName("playlistDock");

    infoDock = new InfoDock;
    infoDock->setObjectName("infoDock");

    QMPlay2Extensions::openExtensions();

#ifndef Q_OS_ANDROID
    addDockWidget(Qt::LeftDockWidgetArea, videoDock);
    addDockWidget(Qt::RightDockWidgetArea, infoDock);
    addDockWidget(Qt::RightDockWidgetArea, playlistDock);
    DockWidget *firstVisualization = nullptr;
    for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList()) //GUI Extensions
    {
        if (DockWidget *dw = QMPlay2Ext->getDockWidget())
        {
            dw->setVisible(false);
            if (QMPlay2Ext->isVisualization())
            {
                dw->setMinimumSize(125, 125);
                QMPlay2Ext->connectDoubleClick(this, SLOT(visualizationFullScreen()));
                if (firstVisualization)
                    tabifyDockWidget(firstVisualization, dw);
                else
                    addDockWidget(Qt::LeftDockWidgetArea, firstVisualization = dw);
            }
            else if (QMPlay2Ext->canConvertAddress())
            {
                tabifyDockWidget(playlistDock, dw);
                dw->setVisible(true);
            }
            else
            {
                tabifyDockWidget(videoDock, dw);
                dw->setVisible(true);
            }
        }
    }
#else //On Android tabify docks (usually screen is too small)
    addDockWidget(Qt::TopDockWidgetArea, videoDock);
    addDockWidget(Qt::BottomDockWidgetArea, playlistDock);
    tabifyDockWidget(playlistDock, infoDock);
    for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList()) //GUI Extensions
        if (DockWidget *dw = QMPlay2Ext->getDockWidget())
        {
            dw->setVisible(false);
            if (QMPlay2Ext->isVisualization())
            {
                dw->setMinimumSize(125, 125);
                QMPlay2Ext->connectDoubleClick(this, SLOT(visualizationFullScreen()));
            }
            tabifyDockWidget(infoDock, dw);
        }
#endif
    infoDock->show();

    m_keepDocksSize = settings.getBool("MainWidget/KeepDocksSize");

    for (auto &&dock : getDockWidgets())
    {
        connect(dock, &DockWidget::shouldStoreSizes, this, [this] {
            m_storeDockSizesTimer.start();
        });
    }

    m_storeDockSizesTimer.setSingleShot(true);
    m_storeDockSizesTimer.setInterval(0);
    connect(&m_storeDockSizesTimer, &QTimer::timeout, this, &MainWidget::storeDockSizes);

    /* ToolBar and MenuBar */
    createMenuBar();

    mainTB = new QToolBar;
    mainTB->setObjectName("mainTB");
    mainTB->setWindowTitle(tr("Main toolbar"));
    mainTB->setAllowedAreas(Qt::BottomToolBarArea | Qt::TopToolBarArea);
    addToolBar(Qt::BottomToolBarArea, mainTB);

    mainTB->addAction(menuBar->player->togglePlay);
    mainTB->addAction(menuBar->player->stop);
    mainTB->addAction(menuBar->player->prev);
    mainTB->addAction(menuBar->player->next);
    mainTB->addAction(menuBar->window->toggleFullScreen);

    mainTB->addAction(menuBar->player->continuePlayback);

    seekS = new Slider;
    seekS->setMaximum(0);
    seekS->setWheelStep(settings.getInt("ShortSeek"));
    mainTB->addWidget(seekS);
    updatePos(0.0);

    vLine = new QFrame;
    vLine->setFrameShape(QFrame::VLine);
    vLine->setFrameShadow(QFrame::Sunken);
    mainTB->addWidget(vLine);

    mainTB->addAction(menuBar->player->toggleMute);

    volW = new VolWidget(settings.getInt("MaxVol"));
    mainTB->addWidget(volW);

    if (auto menu = QMainWindow::createPopupMenu())
    {
        auto shortcuts = QMPlay2GUI.shortcutHandler;
        for (QAction *act : menu->actions())
        {
            auto actParent = act->parent();
            act->setAutoRepeat(false);
            if (actParent == playlistDock)
            {
                shortcuts->appendAction(act, "KeyBindings/Widgets-playlistDock", "Shift+P");
                addAction(act);
            }
            else if (actParent == infoDock)
            {
                shortcuts->appendAction(act, "KeyBindings/Widgets-infoDock", "Shift+I");
                addAction(act);
            }
        }
        menu->deleteLater();
    }

    /**/

    Appearance::init();

    /* Connects */
    connect(qApp, SIGNAL(focusChanged(QWidget *, QWidget *)), this, SLOT(focusChanged(QWidget *, QWidget *)));

    connect(infoDock, SIGNAL(seek(double)), this, SLOT(seek(double)));
    connect(infoDock, SIGNAL(chStream(const QString &)), &playC, SLOT(chStream(const QString &)));
    connect(infoDock, SIGNAL(saveCover()), &playC, SLOT(saveCover()));

    connect(videoDock, &VideoDock::resized, &playC, &PlayClass::videoResized);
    connect(videoDock, SIGNAL(itemDropped(const QString &, bool)), this, SLOT(itemDropped(const QString &, bool)));

    connect(playlistDock, SIGNAL(play(const QString &)), &playC, SLOT(play(const QString &)));
    connect(playlistDock, SIGNAL(repeatEntry(bool)), &playC, SLOT(repeatEntry(bool)));
    connect(playlistDock, SIGNAL(stop()), &playC, SLOT(stop()));
    connect(playlistDock, &PlaylistDock::addAndPlayRestoreWindow, this, [this] {
        m_restoreWindowOnVideo = true;
    });

    connect(seekS, SIGNAL(valueChanged(int)), this, SLOT(seek(int)));
    connect(seekS, SIGNAL(mousePosition(int)), this, SLOT(mousePositionOnSlider(int)));

    connect(volW, SIGNAL(volumeChanged(int, int)), &playC, SLOT(volume(int, int)));

    if (tray)
        connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconClicked(QSystemTrayIcon::ActivationReason)));

    connect(&QMPlay2Core, SIGNAL(sendMessage(const QString &, const QString &, int, int)), this, SLOT(showMessage(const QString &, const QString &, int, int)));
    connect(&QMPlay2Core, SIGNAL(processParam(const QString &, const QString &)), this, SLOT(processParam(const QString &, const QString &)));
    connect(&QMPlay2Core, SIGNAL(statusBarMessage(const QString &, int)), this, SLOT(statusBarMessage(const QString &, int)));
    connect(&QMPlay2Core, SIGNAL(showSettings(const QString &)), this, SLOT(showSettings(const QString &)));

    connect(QMPlay2GUI.videoAdjustment, SIGNAL(videoAdjustmentChanged(const QString &)), &playC, SLOT(videoAdjustmentChanged(const QString &)));

    connect(&playC, SIGNAL(chText(const QString &)), stateL, SLOT(setText(const QString &)));
    connect(&playC, SIGNAL(updateLength(int)), this, SLOT(setSeekSMaximum(int)));
    connect(&playC, SIGNAL(updatePos(double)), this, SLOT(updatePos(double)));
    connect(&playC, SIGNAL(playStateChanged(bool)), this, SLOT(playStateChanged(bool)));
    connect(&playC, SIGNAL(setCurrentPlaying()), playlistDock, SLOT(setCurrentPlaying()));
    connect(&playC, SIGNAL(setInfo(const QString &, bool, bool)), infoDock, SLOT(setInfo(const QString &, bool, bool)));
    connect(&playC, &PlayClass::setStreamsMenu, this, &MainWidget::setStreamsMenu);
    connect(&playC, SIGNAL(updateCurrentEntry(const QString &, double)), playlistDock, SLOT(updateCurrentEntry(const QString &, double)));
    connect(&playC, SIGNAL(playNext(bool)), playlistDock, SLOT(next(bool)));
    connect(&playC, SIGNAL(clearCurrentPlaying()), playlistDock, SLOT(clearCurrentPlaying()));
    connect(&playC, &PlayClass::clearInfo, this, [this] {
        infoDock->clear();
        setStreamsMenu({}, {}, {}, {}, {});
    });
    connect(&playC, SIGNAL(quit()), this, SLOT(deleteLater()));
    connect(&playC, SIGNAL(resetARatio()), this, SLOT(resetARatio()));
    connect(&playC, SIGNAL(updateBitrateAndFPS(int, int, double, double, bool)), infoDock, SLOT(updateBitrateAndFPS(int, int, double, double, bool)));
    connect(&playC, SIGNAL(updateBuffered(qint64, qint64, double, double)), infoDock, SLOT(updateBuffered(qint64, qint64, double, double)));
    connect(&playC, SIGNAL(updateBufferedRange(int, int)), seekS, SLOT(drawRange(int, int)));
    connect(&playC, SIGNAL(updateWindowTitle(const QString &)), this, SLOT(updateWindowTitle(const QString &)));
    connect(&playC, SIGNAL(updateImage(const QImage &)), videoDock, SLOT(updateImage(const QImage &)));
    connect(&playC, &PlayClass::videoStarted, this, &MainWidget::videoStarted);
    connect(&playC, &PlayClass::videoNotStarted, this, [this] {
        m_restoreWindowOnVideo = false;
    });
    connect(&playC, SIGNAL(uncheckSuspend()), this, SLOT(uncheckSuspend()));
    connect(&playC, &PlayClass::setVideoCheckState, this, [this](bool rotate90, bool hFlip, bool vFlip, bool spherical) {
        menuBar->playback->videoFilters->rotate90->setChecked(rotate90);
        menuBar->playback->videoFilters->hFlip->setChecked(hFlip);
        menuBar->playback->videoFilters->vFlip->setChecked(vFlip);
        menuBar->playback->videoFilters->spherical->setChecked(spherical);
    });
    connect(&playC, &PlayClass::continuePos, this, [this](double pos, bool canSetVar) {
        const bool enabled = (pos > 0.0);
        menuBar->player->continuePlayback->setProperty("Pos", pos);
        menuBar->player->continuePlayback->setEnabled(enabled);
        if (canSetVar)
            playC.setDontResetContinuePlayback(enabled);
    });
    connect(&playC, &PlayClass::allowRecording, this, [this](bool allow) {
        menuBar->player->rec->setEnabled(allow);
    });
    connect(&playC, &PlayClass::recording, this, [this](bool status) {
        QSignalBlocker blocker(menuBar->player->rec);
        menuBar->player->rec->setChecked(status);
    });

    /**/

    if (settings.getBool("MainWidget/TabPositionNorth"))
        setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

#if !defined Q_OS_MACOS && !defined Q_OS_ANDROID
    const bool menuHidden = settings.getBool("MainWidget/MenuHidden", false);
    menuBar->setVisible(!menuHidden);
    hideMenuAct = new QAction(tr("&Hide menu bar"), menuBar);
    hideMenuAct->setCheckable(true);
    hideMenuAct->setAutoRepeat(false);
    hideMenuAct->setChecked(menuHidden);
    connect(hideMenuAct, SIGNAL(triggered(bool)), this, SLOT(hideMenu(bool)));
    addAction(hideMenuAct);
    QMPlay2GUI.menuBar->widgets->hideMenuAct = hideMenuAct;
#else
    QMPlay2GUI.menuBar->widgets->hideMenuAct = nullptr;
#endif

    const bool widgetsLocked = settings.getBool("MainWidget/WidgetsLocked");
    lockWidgets(widgetsLocked);
    lockWidgetsAct = new QAction(tr("&Lock widgets"), menuBar);
    lockWidgetsAct->setCheckable(true);
    lockWidgetsAct->setAutoRepeat(false);
    lockWidgetsAct->setChecked(widgetsLocked);
    connect(lockWidgetsAct, SIGNAL(triggered(bool)), this, SLOT(lockWidgets(bool)));
    addAction(lockWidgetsAct);
    QMPlay2GUI.menuBar->widgets->lockWidgetsAct = lockWidgetsAct;

    QMPlay2GUI.menuBar->setKeyShortcuts();
    QMPlay2GUI.videoAdjustment->setKeyShortcuts();
    addChooseNextStreamKeyShortcuts();

    volW->setVolume(settings.getInt("VolumeL"), settings.getInt("VolumeR"), true);
    if (settings.getBool("Mute"))
        menuBar->player->toggleMute->trigger();
    else
        onMuteIconToggled();

    RepeatMode repeatMode = RepeatNormal;
    if (settings.getBool("RestoreRepeatMode"))
        repeatMode = settings.getWithBounds("RepeatMode", RepeatNormal, RepeatStopAfter);
    menuBar->player->repeat->repeatActions[repeatMode]->trigger();

    if (settings.getBool("RestoreAVSState"))
    {
        if (!settings.getBool("AudioEnabled"))
            menuBar->playback->toggleAudio->trigger();
        if (!settings.getBool("VideoEnabled"))
            menuBar->playback->toggleVideo->trigger();
        if (!settings.getBool("SubtitlesEnabled"))
            menuBar->playback->toggleSubtitles->trigger();
    }
    else if (settings.getBool("DisableSubtitlesAtStartup"))
    {
        menuBar->playback->toggleSubtitles->trigger();
    }

    if (settings.getBool("RestoreVideoEqualizer"))
        QMPlay2GUI.videoAdjustment->restoreValues();

    fullScreenDockWidgetState = settings.getByteArray("MainWidget/FullScreenDockWidgetState");
    m_compactViewDockWidgetState = settings.getByteArray("MainWidget/CompactViewDockWidgetState");
    if (menuBar->window->alwaysOnTop && settings.getBool("MainWidget/AlwaysOnTop"))
        menuBar->window->alwaysOnTop->trigger();

    if (testAttribute(Qt::WA_TranslucentBackground) || testAttribute(Qt::WA_NoSystemBackground))
    {
        // If we are about to create RGBA widget, remove translucent flags (e.g. kvantum-theme-matcha enforces it).
        setAttribute(Qt::WA_TranslucentBackground, false);
        setAttribute(Qt::WA_NoSystemBackground, false);
    }

#if defined Q_OS_MACOS || defined Q_OS_ANDROID
    show();
#else
    setVisible(!QMPlay2GUI.startInvisible && settings.getBool("MainWidget/isVisible", true) ? true : !isTrayVisible());
#endif

    for (QObject *obj : children())
    {
        QTabBar *tabBar = qobject_cast<QTabBar *>(obj);
        if (tabBar)
        {
            tabBar->setAcceptDrops(true);
            tabBar->setChangeCurrentOnDrag(true);
        }
    }

    m_hideDocksTimer.setSingleShot(true);
    connect(&m_hideDocksTimer, &QTimer::timeout, this, [this] {
        if (fullScreen || isCompactView)
        {
            if (playlistDock->isVisible())
                hideDocks();
            else
                showToolBar(false);
        }
    });

    playlistDock->load(QMPlay2Core.getSettingsDir() + "Playlist.pls");

    bool noplay = false;
    for (const auto &argument : std::as_const(arguments))
    {
        const QString &param = argument.first;
        const QString &data  = argument.second;
        noplay |= (param == "open" || param == "noplay");
        processParam(param, data);
    }
    arguments.clear();

    playStateChanged(false);

    if (!noplay)
    {
        const QString url = settings.getString("Url");
        if (!url.isEmpty() && url == playlistDock->getUrl())
        {
            playC.setDoSilenceOnStart();
            playlistDock->start();
            const double pos = settings.getDouble("Pos", 0.0);
            if (pos > 0.0)
            {
                double urlPos = 0.0;
                if (settings.getBool("StoreUrlPos"))
                    urlPos = QMPlay2Core.getUrlPosSets().getDouble(url.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
                if (urlPos <= 0.0 || qAbs(urlPos - pos) <= 0.75)
                    seek(pos);
            }
        }
    }

    qApp->installEventFilter(this);

#ifdef Q_OS_MACOS
    fileOpenTimer.setSingleShot(true);
    connect(&fileOpenTimer, &QTimer::timeout, this, &MainWidget::fileOpenTimerTimeout);
    if (QMPlay2GUI.pipe) // Register media keys only for first QMPlay2 instance
        QMPlay2MacExtensions::registerMacOSMediaKeys(std::bind(&MainWidget::processParam, this, std::placeholders::_1, QString()));
#endif

#ifdef UPDATES
    if (settings.getBool("AutoUpdates"))
        updater.downloadUpdate();
#endif
    m_loaded = true;
}
MainWidget::~MainWidget()
{
#ifdef Q_OS_MACOS
    QMPlay2MacExtensions::unregisterMacOSMediaKeys();
#endif
    QMPlay2Extensions::closeExtensions();
    emit QMPlay2Core.restoreCursor();
    Notifies::finalize();
    QMPlay2GUI.mainW = nullptr;
    QCoreApplication::quit();
}

void MainWidget::detachFromPipe()
{
    if (QMPlay2GUI.pipe)
    {
        QMPlay2GUI.pipe->deleteLater();
        QMPlay2GUI.pipe = nullptr;
    }
    if (menuBar->player->detach)
    {
        menuBar->player->detach->deleteLater();
        menuBar->player->detach = nullptr;
    }
}

void MainWidget::focusChanged(QWidget *old, QWidget *now)
{
    const auto focusChangedHelper = [this](QWidget *w) {
        if (w)
        {
            QWidget *tlw = w->window();
            if (tlw && (tlw == settingsW || tlw == aboutW))
                return true;
        }
        return (qobject_cast<QAbstractItemView *>(w) || qobject_cast<QTextEdit *>(w) || qobject_cast<QAbstractSlider *>(w) || qobject_cast<QComboBox *>(w) || qobject_cast<QLineEdit *>(w) || qobject_cast<QMenu *>(w));
    };

    if (focusChangedHelper(old) && old != seekS)
        menuBar->player->seekActionsEnable(true);
    if (focusChangedHelper(now) && now != seekS)
        menuBar->player->seekActionsEnable(false);

    if (qobject_cast<QAbstractButton *>(old))
        menuBar->player->playActionEnable(true);
    if (qobject_cast<QAbstractButton *>(now))
        menuBar->player->playActionEnable(false);

    seekSFocus = (now == seekS);
}

void MainWidget::processParam(const QString &param, const QString &data)
{
    auto getItemsToPlay = [&] {
        auto items = data.split('\n', Qt::SkipEmptyParts);
        for (auto &&item : items)
            item = Functions::maybeExtensionAddress(item);
        return items;
    };

    if (param == "open")
    {
        QMPlay2Core.getSettings().remove("Pos");
        QMPlay2Core.getSettings().remove("Url");
        if (data.contains('\n'))
            playlistDock->addAndPlay(getItemsToPlay());
        else
            playlistDock->addAndPlay(Functions::maybeExtensionAddress(data));
    }
    else if (param == "enqueue")
    {
        if (data.contains('\n'))
            playlistDock->add(getItemsToPlay());
        else
            playlistDock->add(Functions::maybeExtensionAddress(data));
    }
    else if (param == "remove")
    {
        if (data.contains('\n'))
            playlistDock->remove(getItemsToPlay());
        else
            playlistDock->remove(Functions::maybeExtensionAddress(data));
    }
    else if (param == "play")
        playlistDock->start();
    else if (param == "toggle")
        togglePlay();
    else if (param == "show")
    {
        if (!isVisible())
        {
            toggleVisibility();
        }
        else
        {
            if (isMinimized())
                showNormal();
            activateWindow();
            raise();
        }
    }
    else if (param == "stop")
        playC.stop();
    else if (param == "next")
        playlistDock->next();
    else if (param == "prev")
        playlistDock->prev();
    else if (param == "quit")
        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
    else if (param == "volume")
    {
        const int vol = data.toInt();
        volW->setVolume(vol, vol);
        if (menuBar->player->toggleMute->isChecked() != !vol)
            menuBar->player->toggleMute->trigger();
    }
    else if (param == "fullscreen")
        QTimer::singleShot(100, this, SLOT(toggleFullScreen()));
    else if (param == "speed")
        playC.setSpeed(data.toDouble());
    else if (param == "seek")
        seek(data.toDouble());
    else if (param == "RestartPlaying")
        playC.restart();
}

void MainWidget::audioChannelsChanged()
{
    SettingsWidget::SetAudioChannels(sender()->objectName().toInt());
    if (settingsW)
        settingsW->setAudioChannels();
    playC.restart();
}

void MainWidget::updateWindowTitle(const QString &t)
{
    QString title = QCoreApplication::applicationName() + (t.isEmpty() ? QString() : " - " + t);
    if (tray)
        tray->setToolTip(title);
    title.replace('\n', ' ');
    setWindowTitle(title);
}
void MainWidget::videoStarted(bool noVideo)
{
    const bool autoRestoreMainWindowOnVideo = m_restoreWindowOnVideo ? QMPlay2Core.getSettings().getBool("AutoRestoreMainWindowOnVideo") : false;
    const bool autoOpenVideoWindow = QMPlay2Core.getSettings().getBool("AutoOpenVideoWindow");
    if (autoRestoreMainWindowOnVideo || (noVideo && autoOpenVideoWindow))
    {
        if (!videoDock->isVisible())
            videoDock->show();
        videoDock->raise();
    }
    if (autoRestoreMainWindowOnVideo)
    {
        if (!isVisible())
        {
            toggleVisibility();
        }
        else
        {
            activateWindow();
            raise();
        }
    }
    m_restoreWindowOnVideo = false;
}

void MainWidget::togglePlay()
{
    if (playC.isPlaying())
        playC.togglePause();
    else
        playlistDock->start();
}
void MainWidget::seek(int pos)
{
    seek(pos / 10.0);
}
void MainWidget::seek(double pos)
{
    if (!seekS->ignoringValueChanged() && playC.isPlaying())
    {
        auto sender = this->sender();
        playC.seek(pos, (!sender || sender == infoDock || qobject_cast<QAction *>(sender)));
    }
}
void MainWidget::playStateChanged(bool b)
{
    if (b)
    {
        menuBar->player->togglePlay->setIcon(QMPlay2Core.getIconFromTheme("media-playback-pause"));
        menuBar->player->togglePlay->setText(tr("&Pause"));
    }
    else
    {
        menuBar->player->togglePlay->setIcon(QMPlay2Core.getIconFromTheme("media-playback-start"));
        menuBar->player->togglePlay->setText(tr("&Play"));
    }
#if defined(Q_OS_WIN)
    if (m_taskBarProgress)
        m_taskBarProgress->setPaused(!b);
#endif
    emit QMPlay2Core.playStateChanged(playC.isPlaying() ? (b ? "Playing" : "Paused") : "Stopped");
}

void MainWidget::volUpDown()
{
    if (sender() == menuBar->player->volUp)
        volW->changeVolume(5);
    else if (sender() == menuBar->player->volDown)
        volW->changeVolume(-5);
}
void MainWidget::onMuteIconToggled()
{
    const bool muted = menuBar->player->toggleMute->isChecked();
    menuBar->player->toggleMute->setIcon(QMPlay2Core.getIconFromTheme(muted ? "audio-volume-muted" : "audio-volume-high"));
    menuBar->player->toggleMute->setText(muted ? MenuBar::Player::tr("Un&mute") : MenuBar::Player::tr("&Mute"));
}
void MainWidget::actionSeek()
{
    double seekTo = 0.0;
    if (sender() == menuBar->player->seekB)
        seekTo = playC.getPos() - QMPlay2Core.getSettings().getInt("ShortSeek");
    else if (sender() == menuBar->player->seekF)
        seekTo = playC.getPos() + QMPlay2Core.getSettings().getInt("ShortSeek");
    else if (sender() == menuBar->player->lSeekB)
        seekTo = playC.getPos() - QMPlay2Core.getSettings().getInt("LongSeek");
    else if (sender() == menuBar->player->lSeekF)
        seekTo = playC.getPos() + QMPlay2Core.getSettings().getInt("LongSeek");
    seek(seekTo);

    if (!mainTB->isVisible() && !statusBar->isVisible())
    {
        const int max = seekS->maximum() / 10;
        if (max > 0)
        {
            const int count = 50;
            const int pos = qMax<qint64>(0, seekTo * count / max);
            QByteArray osd_pos = "[";
            for (int i = 0; i < count; i++)
                osd_pos += (i == pos) ? "|" : "-";
            osd_pos += "]";
            playC.messageAndOSD(osd_pos, false);
        }
    }
}
void MainWidget::switchARatio()
{
    QAction *checked = menuBar->player->aRatio->choice->checkedAction();
    if (!checked)
        return;
    const QList<QAction *> actions = menuBar->player->aRatio->choice->actions();
    const int idx = actions.indexOf(checked);
    const int checkNewIdx = (idx == actions.count() - 1) ? 0 : (idx + 1);
    if (actions[checkNewIdx]->objectName().startsWith("custom:"))
        actions[checkNewIdx]->setProperty("SkipDialog", true);
    actions[checkNewIdx]->trigger();
}
void MainWidget::resetARatio()
{
    QAction *firstAct = menuBar->player->aRatio->choice->actions().at(0);
    if (menuBar->player->aRatio->choice->checkedAction() != firstAct)
        firstAct->trigger();
}
void MainWidget::resetFlip()
{
    if (menuBar->playback->videoFilters->hFlip->isChecked())
        menuBar->playback->videoFilters->hFlip->trigger();
    if (menuBar->playback->videoFilters->vFlip->isChecked())
        menuBar->playback->videoFilters->vFlip->trigger();
}
void MainWidget::resetRotate90()
{
    if (menuBar->playback->videoFilters->rotate90->isChecked())
        menuBar->playback->videoFilters->rotate90->trigger();
}
void MainWidget::resetSpherical()
{
    if (menuBar->playback->videoFilters->spherical->isChecked())
        menuBar->playback->videoFilters->spherical->trigger();
}

void MainWidget::visualizationFullScreen()
{
    QWidget *senderW = (QWidget *)sender();
    const auto maybeGoFullScreen = [this, senderW] {
        if (!fullScreen)
        {
            videoDock->setWidget(senderW);
            toggleFullScreen();
        }
    };
#ifdef Q_OS_MACOS
    // On macOS if full screen is toggled to fast after double click, mouse remains in clicked state...
    QTimer::singleShot(200, maybeGoFullScreen);
#else
    maybeGoFullScreen();
#endif
}
void MainWidget::hideAllExtensions()
{
    for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
        if (DockWidget *dw = QMPlay2Ext->getDockWidget())
            dw->hide();
}
void MainWidget::toggleVisibility()
{
#ifndef Q_OS_ANDROID
    const bool isTray = isTrayVisible();
    static bool maximized;
    if (isVisible())
    {
        if (fullScreen)
            toggleFullScreen();
        if (!isTray)
        {
#ifndef Q_OS_MACOS
            showMinimized();
#else
            QMPlay2MacExtensions::setApplicationVisible(false);
#endif
        }
        else
        {
            menuBar->options->trayVisible->setEnabled(false);
            if (isMaximized())
            {
                if (!isCompactView)
                    dockWidgetState = saveState();
                maximized = true;
                showNormal();
            }
            hide();
        }
    }
    else
    {
        if (isTray)
            menuBar->options->trayVisible->setEnabled(true);
        if (!maximized)
            showNormal();
        else
        {
            showMaximized();
            if (!isCompactView && !dockWidgetState.isEmpty())
            {
                doRestoreState(dockWidgetState);
                dockWidgetState.clear();
            }
            maximized = false;
        }
        activateWindow();
        raise();
    }
#endif
}
void MainWidget::createMenuBar()
{
    const Settings &QMPSettings = QMPlay2Core.getSettings();

    menuBar = QMPlay2GUI.menuBar;

    for (Module *module : QMPlay2Core.getPluginsInstance())
        for (QAction *act : module->getAddActions())
        {
            act->setParent(menuBar->playlist->add);
            menuBar->playlist->add->addAction(act);
        }
    menuBar->playlist->add->setEnabled(menuBar->playlist->add->actions().count());

    connect(menuBar->window->toggleVisibility, SIGNAL(triggered()), this, SLOT(toggleVisibility()));
    connect(menuBar->window->toggleFullScreen, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
    connect(menuBar->window->toggleCompactView, SIGNAL(triggered()), this, SLOT(toggleCompactView()));
#ifndef Q_OS_ANDROID
    if (menuBar->window->alwaysOnTop)
        connect(menuBar->window->alwaysOnTop, &QAction::triggered, this, &MainWidget::toggleAlwaysOnTop);
    menuBar->window->hideOnClose->setChecked(QMPSettings.getBool("MainWidget/HideOnClose"));
    connect(menuBar->window->hideOnClose, &QAction::triggered,
            this, [](bool checked) {
        QMPlay2Core.getSettings().set("MainWidget/HideOnClose", checked);
    });
#endif
    connect(menuBar->window->close, SIGNAL(triggered()), this, SLOT(close()));

    connect(menuBar->playlist->add->address, SIGNAL(triggered()), this, SLOT(openUrl()));
    connect(menuBar->playlist->add->file, SIGNAL(triggered()), this, SLOT(openFiles()));
    connect(menuBar->playlist->add->dir, SIGNAL(triggered()), this, SLOT(openDir()));
    connect(menuBar->playlist->stopLoading, SIGNAL(triggered()), playlistDock, SLOT(stopLoading()));
    connect(menuBar->playlist->sync, SIGNAL(triggered()), playlistDock, SLOT(syncCurrentFolder()));
    connect(menuBar->playlist->quickSync, SIGNAL(triggered()), playlistDock, SLOT(quickSyncCurrentFolder()));
    connect(menuBar->playlist->loadPlist, SIGNAL(triggered()), this, SLOT(loadPlist()));
    connect(menuBar->playlist->savePlist, SIGNAL(triggered()), this, SLOT(savePlist()));
    connect(menuBar->playlist->saveGroup, SIGNAL(triggered()), this, SLOT(saveGroup()));
    connect(menuBar->playlist->newGroup, SIGNAL(triggered()), playlistDock, SLOT(newGroup()));
    connect(menuBar->playlist->renameGroup, SIGNAL(triggered()), playlistDock, SLOT(renameGroup()));
    connect(menuBar->playlist->lock, SIGNAL(triggered()), playlistDock, SLOT(toggleLock()));
    connect(menuBar->playlist->alwaysSync, &QAction::triggered, playlistDock, &PlaylistDock::alwaysSyncTriggered);
    connect(menuBar->playlist->delEntries, &QAction::triggered, playlistDock, [this] {
        playlistDock->delEntries(false);
    });
    connect(menuBar->playlist->delEntriesFromDisk, &QAction::triggered, playlistDock, [this] {
        playlistDock->delEntries(true);
    });
    connect(menuBar->playlist->delNonGroupEntries, SIGNAL(triggered()), playlistDock, SLOT(delNonGroupEntries()));
    connect(menuBar->playlist->clear, SIGNAL(triggered()), playlistDock, SLOT(clear()));
    connect(menuBar->playlist->copy, SIGNAL(triggered()), playlistDock, SLOT(copy()));
    connect(menuBar->playlist->paste, &QAction::triggered, playlistDock, [this] {
        playlistDock->paste(false);
    });
    connect(menuBar->playlist->pasteAndPlay, &QAction::triggered, playlistDock, [this] {
        playlistDock->paste(true);
    });
    connect(menuBar->playlist->find, SIGNAL(triggered()), playlistDock->findEdit(), SLOT(setFocus()));
    connect(menuBar->playlist->sort->timeSort1, SIGNAL(triggered()), playlistDock, SLOT(timeSort1()));
    connect(menuBar->playlist->sort->timeSort2, SIGNAL(triggered()), playlistDock, SLOT(timeSort2()));
    connect(menuBar->playlist->sort->titleSort1, SIGNAL(triggered()), playlistDock, SLOT(titleSort1()));
    connect(menuBar->playlist->sort->titleSort2, SIGNAL(triggered()), playlistDock, SLOT(titleSort2()));
    connect(menuBar->playlist->collapseAll, SIGNAL(triggered()), playlistDock, SLOT(collapseAll()));
    connect(menuBar->playlist->expandAll, SIGNAL(triggered()), playlistDock, SLOT(expandAll()));
    connect(menuBar->playlist->goToPlayback, SIGNAL(triggered()), playlistDock, SLOT(goToPlayback()));
    connect(menuBar->playlist->queue, SIGNAL(triggered()), playlistDock, SLOT(queue()));
    connect(menuBar->playlist->skip, SIGNAL(triggered()), playlistDock, SLOT(skip()));
    connect(menuBar->playlist->stopAfter, SIGNAL(triggered()), playlistDock, SLOT(stopAfter()));
    connect(menuBar->playlist->entryProperties, SIGNAL(triggered()), playlistDock, SLOT(entryProperties()));

    connect(menuBar->player->togglePlay, SIGNAL(triggered()), this, SLOT(togglePlay()));
    connect(menuBar->player->stop, SIGNAL(triggered()), &playC, SLOT(stop()));
    connect(menuBar->player->next, SIGNAL(triggered()), playlistDock, SLOT(next()));
    connect(menuBar->player->prev, SIGNAL(triggered()), playlistDock, SLOT(prev()));
    connect(menuBar->player->rec, &QAction::triggered, &playC, &PlayClass::setRecording);
    connect(menuBar->player->prevFrame, SIGNAL(triggered()), &playC, SLOT(prevFrame()));
    connect(menuBar->player->nextFrame, SIGNAL(triggered()), &playC, SLOT(nextFrame()));
    for (QAction *act : menuBar->player->repeat->actions())
        connect(act, SIGNAL(triggered()), playlistDock, SLOT(repeat()));
    connect(menuBar->player->abRepeat, SIGNAL(triggered()), &playC, SLOT(setAB()));
    connect(menuBar->player->seekF, SIGNAL(triggered()), this, SLOT(actionSeek()));
    connect(menuBar->player->seekB, SIGNAL(triggered()), this, SLOT(actionSeek()));
    connect(menuBar->player->lSeekF, SIGNAL(triggered()), this, SLOT(actionSeek()));
    connect(menuBar->player->lSeekB, SIGNAL(triggered()), this, SLOT(actionSeek()));
    connect(menuBar->player->speedUp, SIGNAL(triggered()), &playC, SLOT(speedUp()));
    connect(menuBar->player->slowDown, SIGNAL(triggered()), &playC, SLOT(slowDown()));
    connect(menuBar->player->setSpeed, SIGNAL(triggered()), &playC, SLOT(setSpeed()));
    connect(menuBar->player->zoom->integerScaling, &QAction::toggled, menuBar->player->zoom->setZoom, &QAction::setDisabled);
    connect(menuBar->player->zoom->integerScaling, &QAction::toggled, menuBar->player->zoom->preciseZoom, &QAction::setDisabled);
    menuBar->player->zoom->integerScaling->setChecked(QMPSettings.getBool("IntegerScaling"));
    menuBar->player->zoom->preciseZoom->setChecked(QMPSettings.getBool("PreciseZoom"));
    connect(menuBar->player->zoom->integerScaling, &QAction::triggered, &playC, &PlayClass::setIntegerScaling);
    connect(menuBar->player->zoom->preciseZoom, &QAction::triggered, &playC, &PlayClass::setPreciseZoom);
    connect(menuBar->player->zoom->zoomIn, SIGNAL(triggered()), &playC, SLOT(zoomIn()));
    connect(menuBar->player->zoom->zoomOut, SIGNAL(triggered()), &playC, SLOT(zoomOut()));
    connect(menuBar->player->zoom->setZoom, &QAction::triggered, &playC, &PlayClass::setZoom);
    connect(menuBar->player->switchARatio, SIGNAL(triggered()), this, SLOT(switchARatio()));
    {
        const QString initialAspectRatio = QMPSettings.getBool("StoreARatioAndZoom") ? QMPSettings.getString("AspectRatio") : QString();
        QAction *autoARatioAct = nullptr;
        bool aRatioTriggered = false;
        for (QAction *act : menuBar->player->aRatio->actions())
        {
            connect(act, &QAction::triggered, &playC, &PlayClass::aRatio);
            if (aRatioTriggered)
                continue;
            const auto objectName = act->objectName();
            if (objectName == "auto")
                autoARatioAct = act;
            if (!initialAspectRatio.isEmpty() && (objectName.startsWith("custom:") ? objectName.right(objectName.size() - 7) == initialAspectRatio : objectName == initialAspectRatio))
            {
                act->setProperty("SkipDialog", true);
                act->trigger();
                aRatioTriggered = true;
            }
        }
        if (!aRatioTriggered && autoARatioAct)
            autoARatioAct->trigger();
    }
    connect(menuBar->player->reset, SIGNAL(triggered()), this, SLOT(resetFlip()));
    connect(menuBar->player->reset, SIGNAL(triggered()), this, SLOT(resetRotate90()));
    connect(menuBar->player->reset, SIGNAL(triggered()), this, SLOT(resetSpherical()));
    connect(menuBar->player->reset, SIGNAL(triggered()), this, SLOT(resetARatio()));
    connect(menuBar->player->reset, SIGNAL(triggered()), &playC, SLOT(zoomReset()));
    connect(menuBar->player->reset, SIGNAL(triggered()), &playC, SLOT(otherReset()));
    connect(menuBar->player->continuePlayback, &QAction::triggered, this, [this] {
        const double pos = menuBar->player->continuePlayback->property("Pos").toDouble();
        if (pos > 0.0)
            playC.seek(pos);
        else
            emit playC.continuePos(0.0, true);
    });
    connect(menuBar->player->volUp, SIGNAL(triggered()), this, SLOT(volUpDown()));
    connect(menuBar->player->volDown, SIGNAL(triggered()), this, SLOT(volUpDown()));
    connect(menuBar->player->toggleMute, SIGNAL(triggered()), &playC, SLOT(toggleMute()));
    connect(menuBar->player->toggleMute, SIGNAL(triggered()), this, SLOT(onMuteIconToggled()));
    if (menuBar->player->detach)
        connect(menuBar->player->detach, SIGNAL(triggered()), this, SLOT(detachFromPipe()));
    if (menuBar->player->suspend)
        connect(menuBar->player->suspend, SIGNAL(triggered(bool)), &playC, SLOT(suspendWhenFinished(bool)));

    connect(menuBar->playback->toggleAudio, SIGNAL(triggered(bool)), &playC, SLOT(toggleAVS(bool)));
    if (menuBar->playback->keepAudioPitch)
    {
        menuBar->playback->keepAudioPitch->setChecked(QMPSettings.getBool("KeepAudioPitch"));
        connect(menuBar->playback->keepAudioPitch, &QAction::triggered, &playC, &PlayClass::setKeepAudioPitch);
    }
    connect(menuBar->playback->toggleVideo, SIGNAL(triggered(bool)), &playC, SLOT(toggleAVS(bool)));
    connect(menuBar->playback->toggleSubtitles, SIGNAL(triggered(bool)), &playC, SLOT(toggleAVS(bool)));
    connect(menuBar->playback->videoSync, SIGNAL(triggered()), &playC, SLOT(setVideoSync()));
    connect(menuBar->playback->slowDownVideo, SIGNAL(triggered()), &playC, SLOT(slowDownVideo()));
    connect(menuBar->playback->speedUpVideo, SIGNAL(triggered()), &playC, SLOT(speedUpVideo()));
    connect(menuBar->playback->slowDownSubtitles, SIGNAL(triggered()), &playC, SLOT(slowDownSubs()));
    connect(menuBar->playback->speedUpSubtitles, SIGNAL(triggered()), &playC, SLOT(speedUpSubs()));
    connect(menuBar->playback->biggerSubtitles, SIGNAL(triggered()), &playC, SLOT(biggerSubs()));
    connect(menuBar->playback->smallerSubtitles, SIGNAL(triggered()), &playC, SLOT(smallerSubs()));
    connect(menuBar->playback->screenShot, SIGNAL(triggered()), &playC, SLOT(screenShot()));
    connect(menuBar->playback->subsFromFile, SIGNAL(triggered()), this, SLOT(browseSubsFile()));
    connect(menuBar->playback->subtitlesSync, SIGNAL(triggered()), &playC, SLOT(setSubtitlesSync()));
    connect(menuBar->playback->videoFilters->spherical, SIGNAL(triggered(bool)), &playC, SLOT(setSpherical(bool)));
    connect(menuBar->playback->videoFilters->hFlip, SIGNAL(triggered(bool)), &playC, SLOT(setHFlip(bool)));
    connect(menuBar->playback->videoFilters->vFlip, SIGNAL(triggered(bool)), &playC, SLOT(setVFlip(bool)));
    connect(menuBar->playback->videoFilters->rotate90, SIGNAL(triggered(bool)), &playC, SLOT(setRotate90(bool)));
    connect(menuBar->playback->videoFilters->more, SIGNAL(triggered(bool)), this, SLOT(showSettings()));
    for (QAction *act : menuBar->playback->audioChannels->actions())
        connect(act, SIGNAL(triggered()), this, SLOT(audioChannelsChanged()));
    SettingsWidget::SetAudioChannelsMenu();

    connect(menuBar->options->settings, SIGNAL(triggered()), this, SLOT(showSettings()));
    connect(menuBar->options->playbackSettings, SIGNAL(triggered()), this, SLOT(showSettings()));
    connect(menuBar->options->rendererSettings, SIGNAL(triggered()), this, SLOT(showSettings()));
    connect(menuBar->options->modulesSettings, SIGNAL(triggered()), this, SLOT(showSettings()));
    if (tray)
        connect(menuBar->options->trayVisible, SIGNAL(triggered(bool)), tray, SLOT(setVisible(bool)));

    connect(menuBar->help->about, SIGNAL(triggered()), this, SLOT(about()));
#ifdef UPDATER
    connect(menuBar->help->updates, SIGNAL(triggered()), &updater, SLOT(exec()));
#endif
    connect(menuBar->help->aboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    menuBar->window->toggleCompactView->setChecked(isCompactView);
    if (tray)
        menuBar->options->trayVisible->setChecked(tray->isVisible());
    else
        menuBar->options->trayVisible->setVisible(false);

    setContinuePlaybackVisibility();

    setMenuBar(menuBar);

#ifndef Q_OS_MACOS
    if (tray)
    {
        auto secondMenu = new QMenu(this);
        copyMenu(secondMenu, menuBar->window);
        secondMenu->addMenu(menuBar->widgets);
        copyMenu(secondMenu, menuBar->playlist, {menuBar->playlist->extensions});
        copyMenu(secondMenu, menuBar->player);
        copyMenu(secondMenu, menuBar->playback,
             {
                 menuBar->playback->audioStreams,
                 menuBar->playback->videoStreams,
                 menuBar->playback->subtitlesStreams,
                 menuBar->playback->chapters,
                 menuBar->playback->programs,
             },
             {
#ifdef Q_OS_WIN
                 menuBar->playback->videoFilters->videoAdjustmentMenu,
#endif
             }
        );
        copyMenu(secondMenu, menuBar->options);
        for (auto ext : QMPlay2Extensions::QMPlay2ExtensionsList())
        {
            if (auto menu = ext->getTrayMenu())
                secondMenu->addMenu(menu);
        }
        tray->setContextMenu(secondMenu);

        connect(secondMenu, &QMenu::aboutToShow,
                this, [=] {
            for (auto ext : QMPlay2Extensions::QMPlay2ExtensionsList())
            {
                if (auto menu = ext->getTrayMenu())
                {
                    ext->ensureTrayMenu();
                    if (!secondMenu->actions().contains(menu->menuAction()))
                        secondMenu->addMenu(menu);
                }
            }
        });
    }
#else //On OS X add only the most important menu actions to dock menu
    auto secondMenu = new QMenu(this);
    secondMenu->addAction(menuBar->player->togglePlay);
    secondMenu->addAction(menuBar->player->stop);
    secondMenu->addAction(menuBar->player->next);
    secondMenu->addAction(menuBar->player->prev);
    secondMenu->addSeparator();
    secondMenu->addAction(menuBar->player->toggleMute);
    secondMenu->addSeparator();
    // Copy action, because PreferencesRole doesn't show in dock menu.
    QAction *settings = new QAction(menuBar->options->settings->icon(), menuBar->options->settings->text(), menuBar->options->settings->parent());
    connect(settings, &QAction::triggered, menuBar->options->settings, &QAction::trigger);
    secondMenu->addAction(settings);

    QAction *newInstanceAct = new QAction(tr("New window"), secondMenu);
    connect(newInstanceAct, &QAction::triggered, [] {
        QProcess::startDetached(QCoreApplication::applicationFilePath(), {"-noplay"}, QCoreApplication::applicationDirPath());
    });
    secondMenu->addSeparator();
    secondMenu->addAction(newInstanceAct);

    secondMenu->setAsDockMenu();
#endif
}
void MainWidget::trayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
#if !defined Q_OS_MACOS && !defined Q_OS_ANDROID
    switch (reason)
    {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            toggleVisibility();
            break;
        case QSystemTrayIcon::MiddleClick:
            if (isVisible())
                menuBar->window->toggleCompactView->trigger();
            else
                togglePlay();
            break;
        default:
            break;
    }
#else
    Q_UNUSED(reason)
#endif
}
void MainWidget::toggleCompactView()
{
    if (!isCompactView)
    {
        dockWidgetState = saveState();

        hideDockWidgetsAndDisableFeatures();

#if !defined Q_OS_MACOS && !defined Q_OS_ANDROID
        menuBar->hide();
#endif
        mainTB->hide();
        statusBar->hide();
        videoDock->show();

        videoDock->fullScreen(true);

        isCompactView = true;
    }
    else
    {
        videoDock->setLoseHeight(0);
        isCompactView = false;

        videoDock->fullScreen(false);

        restoreState(dockWidgetState);
        dockWidgetState.clear();

        restoreDockWidgetFeatures();

#if !defined Q_OS_MACOS && !defined Q_OS_ANDROID
        menuBar->setVisible(!hideMenuAct->isChecked());
#endif

        statusBar->show();
    }
}
void MainWidget::toggleAlwaysOnTop(bool checked)
{
    auto flags = windowFlags();
    if (checked)
        flags |= Qt::WindowStaysOnTopHint;
    else
        flags &= ~Qt::WindowStaysOnTopHint;
    const auto visible = isVisible();
    destroy(false, false);
    setWindowFlags(flags);
    if (visible)
        show();
}
void MainWidget::toggleFullScreen()
{
    static bool visible, tb_movable;
#ifdef Q_OS_MACOS
    if (isFullScreen())
    {
        showNormal();
        return;
    }
#endif
    if (!fullScreen)
    {
        visible = isVisible();

        if ((m_compactViewBeforeFullScreen = isCompactView))
            toggleCompactView();

#ifndef Q_OS_ANDROID
        m_maximizedBeforeFullScreen = isMaximized();

#ifndef Q_OS_MACOS
#ifndef Q_OS_WIN
        if (isFullScreen())
#endif
            showNormal();
#endif

        dockWidgetState = saveState();
        if (!m_maximizedBeforeFullScreen)
            savedGeo = winGeo();
#else
        dockWidgetState = saveState();
#endif // Q_OS_ANDROID

#if !defined Q_OS_MACOS
        menuBar->hide();
#endif
        statusBar->hide();

        mainTB->hide();
        addToolBar(Qt::BottomToolBarArea, mainTB);
        tb_movable = mainTB->isMovable();
        mainTB->setMovable(false);

        hideDockWidgetsAndDisableFeatures();

        videoDock->fullScreen(true);
        videoDock->show();

#ifdef Q_OS_MACOS
        menuBar->window->toggleVisibility->setEnabled(false);
#endif
        menuBar->window->toggleCompactView->setEnabled(false);
        menuBar->window->toggleFullScreen->setShortcuts(QList<QKeySequence>() << menuBar->window->toggleFullScreen->shortcut() << QKeySequence("ESC"));
        fullScreen = true;

#ifndef Q_OS_MACOS
        showFullScreen();
#else
        const auto geo = window()->windowHandle()->screen()->geometry();
        setMinimumSize(geo.size());
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        setGeometry(geo);
        QMPlay2MacExtensions::showSystemUi(windowHandle(), false);
        show();
#endif

        if (playC.isPlaying())
            QMPlay2GUI.screenSaver->inhibit(1);
    }
    else
    {
#ifdef Q_OS_MACOS
        menuBar->window->toggleVisibility->setEnabled(true);
#endif
        menuBar->window->toggleCompactView->setEnabled(true);
        menuBar->window->toggleFullScreen->setShortcuts(QList<QKeySequence>() << menuBar->window->toggleFullScreen->shortcut());

        videoDock->setLoseHeight(0);
        fullScreen = false;

#ifndef Q_OS_ANDROID
#ifdef Q_OS_MACOS
        setMinimumSize(0, 0);
        QMPlay2MacExtensions::showSystemUi(windowHandle(), true);
        setWindowFlags(Qt::Window);
#else
        showNormal();
#endif // Q_OS_MACOS
        if (m_maximizedBeforeFullScreen)
        {
            showMaximized();
        }
        else
        {
#ifdef Q_OS_MACOS
            showNormal();
#endif
            setGeometry(savedGeo);
        }
#else // Q_OS_ANDROID
        showMaximized();
#endif
#ifdef Q_OS_WIN
        QCoreApplication::processEvents();
#endif

        videoDock->fullScreen(false);

        restoreState(dockWidgetState);
        dockWidgetState.clear();

        if (!visible) //jeżeli okno było wcześniej ukryte, to ma je znowu ukryć
            toggleVisibility();

        restoreDockWidgetFeatures();

#if defined(Q_OS_ANDROID)
        menuBar->setVisible(true);
#elif !defined Q_OS_MACOS
        menuBar->setVisible(!hideMenuAct->isChecked());
#endif
        statusBar->show();

        mainTB->setMovable(tb_movable);

        if (m_compactViewBeforeFullScreen)
            toggleCompactView();

        playlistDock->scrollToCurrectItem();

        QMPlay2GUI.screenSaver->unInhibit(1);

        setFocus();
    }
    if (menuBar->window->alwaysOnTop)
        menuBar->window->alwaysOnTop->setEnabled(!fullScreen);
    emit QMPlay2Core.fullScreenChanged(fullScreen);
}
void MainWidget::showMessage(const QString &msg, const QString &title, int messageIcon, int ms)
{
    if (ms < 1 || !Notifies::notify(title, msg, ms, messageIcon))
    {
        QMessageBox *messageBox = new QMessageBox(this);
        messageBox->setIcon((QMessageBox::Icon)messageIcon);
        messageBox->setStandardButtons(QMessageBox::Ok);
        messageBox->setAttribute(Qt::WA_DeleteOnClose);
        messageBox->setInformativeText(msg);
        messageBox->setText(title);
        messageBox->setModal(true);
        messageBox->show();
    }
}
void MainWidget::statusBarMessage(const QString &msg, int ms)
{
    statusBar->showMessage(msg, ms);
}

void MainWidget::openUrl()
{
    AddressDialog ad(this);
    if (ad.exec())
    {
        if (ad.addAndPlay())
            playlistDock->addAndPlay(ad.url());
        else
            playlistDock->add(ad.url());

    }
}
void MainWidget::openFiles()
{
    playlistDock->add(QFileDialog::getOpenFileNames(this, tr("Choose files"), QMPlay2GUI.getCurrentPth(playlistDock->getUrl(), true)));
}
void MainWidget::openDir()
{
    playlistDock->add(QFileDialog::getExistingDirectory(this, tr("Choose directory"), QMPlay2GUI.getCurrentPth(playlistDock->getUrl())));
}
void MainWidget::loadPlist()
{
    QString filter = tr("Playlists") + " (";
    for (const QString &e : Playlist::extensions())
        filter += "*." + e + " ";
    filter.chop(1);
    filter += ")";
    if (filter.isEmpty())
        return;
    playlistDock->load(QFileDialog::getOpenFileName(this, tr("Choose playlist"), QMPlay2GUI.getCurrentPth(playlistDock->getUrl()), filter));
}
void MainWidget::savePlist()
{
    savePlistHelper(tr("Save playlist"), QMPlay2GUI.getCurrentPth(playlistDock->getUrl()), false);
}
void MainWidget::saveGroup()
{
    savePlistHelper(tr("Save group"), QMPlay2GUI.getCurrentPth(playlistDock->getUrl()) + playlistDock->getCurrentItemName(), true);
}
void MainWidget::showSettings(const QString &moduleName)
{
    if (!settingsW)
    {
        int page;
        if (sender() == menuBar->options->rendererSettings)
            page = 1;
        else if (sender() == menuBar->options->playbackSettings)
            page = 2;
        else if (sender() == menuBar->options->modulesSettings || !moduleName.isEmpty())
            page = 3;
        else if (sender() == menuBar->playback->videoFilters->more)
            page = 6;
        else
            page = 0;

        settingsW = new SettingsWidget(page, moduleName, QMPlay2GUI.videoAdjustment);
        if (windowFlags() & Qt::WindowStaysOnTopHint)
            settingsW->setWindowFlag(Qt::WindowStaysOnTopHint);
        settingsW->show();
        connect(settingsW, &SettingsWidget::settingsChanged, this, [this](int page, bool forceRestart, bool initFilters) {
            Q_UNUSED(forceRestart);
            Q_UNUSED(initFilters);
            if (page == 2)
                setContinuePlaybackVisibility();
        });
        connect(settingsW, SIGNAL(settingsChanged(int, bool, bool)), &playC, SLOT(settingsChanged(int, bool, bool)));
        connect(settingsW, SIGNAL(setWheelStep(int)), seekS, SLOT(setWheelStep(int)));
        connect(settingsW, SIGNAL(setVolMax(int)), volW, SLOT(setMaximumVolume(int)));
        connect(settingsW, &SettingsWidget::keepDocksSizeChanged, this, [this](bool keepDocksSize) {
            m_keepDocksSize = keepDocksSize;
            m_storeDockSizesTimer.start();
        });
        connect(settingsW, SIGNAL(destroyed()), this, SLOT(showSettings()));
    }
    else
    {
        bool showAgain = sender() != settingsW;
        if (showAgain)
            disconnect(settingsW, SIGNAL(destroyed()), this, SLOT(showSettings()));
        settingsW->close();
        settingsW = nullptr;
        if (showAgain)
            showSettings(moduleName);
    }
}
void MainWidget::showSettings()
{
    showSettings(QString());
}

void MainWidget::itemDropped(const QString &pth, bool subs)
{
    if (subs)
        playC.loadSubsFile(pth);
    else
        playlistDock->addAndPlay(pth);
}
void MainWidget::browseSubsFile()
{
    QString dir = Functions::filePath(playC.getUrl());
    if (dir.isEmpty())
        dir = QMPlay2GUI.getCurrentPth(playlistDock->getUrl());
    if (dir.startsWith("file://"))
        dir.remove(0, 7);

    QString filter = tr("Subtitles") + " ASS/SSA (*.ass *.ssa)";
    for (const QString &ext : SubsDec::extensions())
        filter += ";;" + tr("Subtitles") + " " + ext.toUpper() + " (*." + ext + ")";

    QString f = QFileDialog::getOpenFileName(this, tr("Choose subtitles"), dir, filter);
    if (!f.isEmpty())
        playC.loadSubsFile(Functions::Url(f));
}

void MainWidget::setSeekSMaximum(int max)
{
    seekS->setMaximum(qMax(0, max * 10));
#if defined(Q_OS_WIN)
    if (m_taskBarProgress)
    {
        m_taskBarProgress->setMaximum(seekS->maximum() / 10);
        m_taskBarProgress->setVisible(m_taskBarProgress->maximum() > 0);
    }
#endif
    if (max >= 0)
    {
        seekS->setEnabled(true);
        if (seekSFocus)
            seekS->setFocus();
    }
    else
    {
        const bool focus = seekS->hasFocus();
        if (focus)
            setFocus(); //This changes the "seekSFocus" in "MainWidget::focusChanged()"
        seekSFocus = focus;
        seekS->setEnabled(false);
    }
}
void MainWidget::updatePos(double pos)
{
    static int currPos;
    if (!playC.isPlaying() || playC.canUpdatePosition())
    {
        const int intPos = pos;
        if (pos <= 0.0 || currPos != intPos)
        {
            const double max = seekS->maximum() / 10.0;
            const QString remainingTime = (max - pos > 0.0) ? timeToStr(max - pos) : timeToStr(0.0);
            timeL->setText(timeToStr(pos) + " ; -" + remainingTime + " / " + timeToStr(max));
            emit QMPlay2Core.posChanged(intPos);
            currPos = intPos;
        }
        seekS->setValue(pos * 10.0);
#if defined(Q_OS_WIN)
        if (m_taskBarProgress)
            m_taskBarProgress->setValue(pos);
#endif
    }
}
void MainWidget::mousePositionOnSlider(int pos)
{
    statusBar->showMessage(tr("Pointed position") + ": " + timeToStr(pos / 10.0, true), 750);
}

void MainWidget::newConnection(IPCSocket *socket)
{
    connect(socket, SIGNAL(readyRead()), this, SLOT(readSocket()));
    connect(socket, SIGNAL(aboutToClose()), socket, SLOT(deleteLater()));
}
void MainWidget::readSocket()
{
    IPCSocket *socket = (IPCSocket *)sender();
    disconnect(socket, SIGNAL(aboutToClose()), socket, SLOT(deleteLater()));
    for (const QByteArray &arr : socket->readAll().split('\0'))
    {
        int idx = arr.indexOf('\t');
        if (idx > -1)
            processParam(arr.mid(0, idx), arr.mid(idx + 1)); //tu może wywołać się precessEvents()!
    }
    if (socket->isConnected())
        connect(socket, SIGNAL(aboutToClose()), socket, SLOT(deleteLater()));
    else
        socket->deleteLater();
}

void MainWidget::about()
{
    if (!aboutW)
    {
        aboutW = new AboutWidget;
        if (windowFlags() & Qt::WindowStaysOnTopHint)
            aboutW->setWindowFlag(Qt::WindowStaysOnTopHint);
        aboutW->show();
        connect(aboutW, SIGNAL(destroyed()), this, SLOT(about()));
    }
    else
    {
        aboutW->close();
        aboutW = nullptr;
    }
}

#if !defined Q_OS_MACOS && !defined Q_OS_ANDROID
void MainWidget::hideMenu(bool h)
{
    if (fullScreen || isCompactView)
        qobject_cast<QAction *>(sender())->setChecked(!h);
    else
    {
        menuBar->setVisible(!h);
        QMPlay2Core.getSettings().set("MainWidget/MenuHidden", h);
    }
}
#endif
void MainWidget::lockWidgets(bool l)
{
    if (fullScreen || isCompactView)
        qobject_cast<QAction *>(sender())->setChecked(!l);
    else
    {
        for (QObject *o : children())
        {
            DockWidget *dW = qobject_cast<DockWidget *>(o);
            if (dW)
            {
                if (dW->isFloating())
                    dW->setFloating(false);
                dW->setGlobalTitleBarVisible(!l);
            }
        }
        mainTB->setMovable(!l);
        QMPlay2Core.getSettings().set("MainWidget/WidgetsLocked", l);
    }
}

void MainWidget::doRestoreState(const QByteArray &data, bool doToggleCompactView)
{
    if (isMaximized())
    {
        setUpdatesEnabled(false);
        QTimer::singleShot(0, this, [=] {
            QTimer::singleShot(0, this, [=] {
                QTimer::singleShot(0, this, [=] {
                    restoreState(data);
                    if (doToggleCompactView)
                        menuBar->window->toggleCompactView->trigger();
                    setUpdatesEnabled(true);
                    repaint();
                });
            });
        });
    }
    else
    {
        restoreState(data);
        if (doToggleCompactView)
            menuBar->window->toggleCompactView->trigger();
    }
}

void MainWidget::uncheckSuspend()
{
    if (menuBar->player->suspend)
        menuBar->player->suspend->setChecked(false);
}

void MainWidget::setStreamsMenu(const QStringList &videoStreams, const QStringList &audioStreams, const QStringList &subsStreams, const QStringList &chapters, const QStringList &programs)
{
    auto setActions = [this](const QStringList &streams, MenuBar::Playback::Streams *menu) {
        menu->clear();
        for (auto &&videoStream : streams)
        {
            auto lines = videoStream.split(QLatin1Char('\n'));
            Q_ASSERT(lines.size() >= 2);

            auto action = menu->addAction(lines.constFirst());
            const bool hasData = !lines.at(1).isEmpty();

            if (menu->group)
            {
                menu->group->addAction(action);
                action->setCheckable(true);
                if (lines.size() == 3)
                    action->setChecked(true);
            }

            if (hasData)
            {
                connect(action, &QAction::triggered,
                        this, [this, data = std::move(lines[1])] {
                    if (data.startsWith("seek"))
                        seek(QStringView(data).mid(4).toDouble());
                    else
                        playC.chStream(data);
                });
            }
            else
            {
                action->setEnabled(false);
            }
        }
        menu->menuAction()->setVisible(!menu->isEmpty());
    };

    setActions(videoStreams, menuBar->playback->videoStreams);
    setActions(audioStreams, menuBar->playback->audioStreams);
    setActions(subsStreams, menuBar->playback->subtitlesStreams);
    setActions(chapters, menuBar->playback->chapters);
    setActions(programs, menuBar->playback->programs);
}

void MainWidget::savePlistHelper(const QString &title, const QString &fPth, bool saveCurrentGroup)
{
    QString filter;
    for (const QString &e : Playlist::extensions())
        filter += e.toUpper() + " (*." + e + ");;";
    filter.chop(2);
    if (filter.isEmpty())
        return;
    QString selectedFilter;
    QString plistFile = QFileDialog::getSaveFileName(this, title, fPth, filter, &selectedFilter);
#ifdef Q_OS_ANDROID
    if (selectedFilter.isEmpty())
        selectedFilter = QFileInfo(plistFile).suffix();
#endif
    if (!selectedFilter.isEmpty())
    {
        selectedFilter = "." + selectedFilter.mid(0, selectedFilter.indexOf(' ')).toLower();
        if (plistFile.right(selectedFilter.length()).toLower() != selectedFilter)
            plistFile += selectedFilter;
        if (playlistDock->save(plistFile, saveCurrentGroup))
            QMPlay2GUI.setCurrentPth(Functions::filePath(plistFile));
        else
            QMessageBox::critical(this, title, tr("Error while saving playlist"));
    }
}

QMenu *MainWidget::createPopupMenu()
{
    QMenu *popupMenu = QMainWindow::createPopupMenu();
    if (!fullScreen && !isCompactView)
    {
#if !defined Q_OS_MACOS && !defined Q_OS_ANDROID
        popupMenu->insertAction(popupMenu->actions().value(0), hideMenuAct);
        popupMenu->insertSeparator(popupMenu->actions().value(1));
        popupMenu->addSeparator();
#endif
        popupMenu->addAction(lockWidgetsAct);
    }
    for (QAction *act : popupMenu->actions())
    {
        if (act->parentWidget() == mainTB)
        {
            act->setEnabled(isVisible() && !fullScreen && !isCompactView);
            break;
        }
    }
    return popupMenu;
}

void MainWidget::hideDockWidgetsAndDisableFeatures()
{
    infoDock->hide();
    infoDock->setFeatures(DockWidget::NoDockWidgetFeatures);

    playlistDock->hide();
    playlistDock->setFeatures(DockWidget::NoDockWidgetFeatures);

    addDockWidget(Qt::RightDockWidgetArea, videoDock);
    for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
    {
        if (DockWidget *dw = QMPlay2Ext->getDockWidget())
        {
            if (dw->isVisible())
                visibleQMPlay2Extensions += QMPlay2Ext;
            dw->hide();
            dw->setFeatures(DockWidget::NoDockWidgetFeatures);
        }
    }
}
void MainWidget::restoreDockWidgetFeatures()
{
    infoDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    playlistDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
        if (QDockWidget *dw = QMPlay2Ext->getDockWidget())
            dw->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    visibleQMPlay2Extensions.clear();
}

void MainWidget::showToolBar(bool showTB)
{
    if (showTB && (!mainTB->isVisible() || !statusBar->isVisible()))
    {
        videoDock->setLoseHeight(statusBar->height());
        statusBar->show();
        videoDock->setLoseHeight(mainTB->height() + statusBar->height());
        mainTB->show();
    }
    else if (!showTB && (mainTB->isVisible() || statusBar->isVisible()))
    {
        videoDock->setLoseHeight(statusBar->height());
        mainTB->hide();
        videoDock->setLoseHeight(0);
        statusBar->hide();
    }
}
void MainWidget::hideDocks()
{
    if (fullScreen)
        fullScreenDockWidgetState = saveState();
    if (isCompactView)
        m_compactViewDockWidgetState = saveState();
    showToolBar(false);
    playlistDock->hide();
    infoDock->hide();
    hideAllExtensions();
}

inline bool MainWidget::isTrayVisible() const
{
    return (tray && QSystemTrayIcon::isSystemTrayAvailable() && tray->isVisible());
}

bool MainWidget::getFullScreen() const
{
    return fullScreen;
}

void MainWidget::addChooseNextStreamKeyShortcuts()
{
    auto chooseNextStream = [](const auto &actions) {
        if (actions.empty())
            return;

        bool foundChecked = false;
        for (auto &&action : actions)
        {
            if (action->isChecked())
            {
                foundChecked = true;
            }
            else if (foundChecked)
            {
                action->trigger();
                return;
            }
        }
        actions[0]->trigger();
    };

    auto nextAudioStream = new QAction(tr("Next audio stream"), this);
    connect(nextAudioStream, &QAction::triggered,
            this, [=] {
        chooseNextStream(menuBar->playback->audioStreams->actions());
    });
    addAction(nextAudioStream);
    QMPlay2GUI.shortcutHandler->appendAction(nextAudioStream, "KeyBindings/NextAudioStream", "Ctrl+1");

    auto nextSubsStream = new QAction(tr("Next subtitles stream"), this);
    connect(nextSubsStream, &QAction::triggered,
            this, [=] {
        chooseNextStream(menuBar->playback->subtitlesStreams->actions());
    });
    addAction(nextSubsStream);
    QMPlay2GUI.shortcutHandler->appendAction(nextSubsStream, "KeyBindings/NextSubsStream", "Ctrl+2");
}

void MainWidget::setContinuePlaybackVisibility()
{
    menuBar->player->continuePlayback->setVisible(QMPlay2Core.getSettings().getBool("StoreUrlPos"));
}

inline const QList<DockWidget *> MainWidget::getDockWidgets() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    return findChildren<DockWidget *>(Qt::FindDirectChildrenOnly);
#else
    return findChildren<DockWidget *>(QString(), Qt::FindDirectChildrenOnly);
#endif
}

inline QRect MainWidget::winGeo() const
{
    if (const auto win = windowHandle())
        return win->geometry();
    return geometry();
}

void MainWidget::storeDockSizes()
{
    m_winSizeForDocks = size();
    m_dockSizes.clear();

    if (!m_keepDocksSize)
        return;

    if (!fullScreen && !isCompactView)
    {
        QList<QDockWidget *> tabified;
        for (auto &&dock : getDockWidgets())
        {
            if (!dock->isVisible())
                continue;

            if (dock->isFloating())
                continue;

            tabified += tabifiedDockWidgets(dock);

            if (tabified.contains(dock))
                continue;

            m_dockSizes.emplace_back(dock, dock->size());
        }
    }
}

#if defined(Q_OS_WIN)
void MainWidget::setWindowsTaskBarFeatures()
{
    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::Windows7)
        return;

    m_taskbarButton = new QWinTaskbarButton(this);
    m_taskbarButton->setWindow(windowHandle());

    m_taskBarProgress = m_taskbarButton->progress();


    m_thumbnailToolBar = new QWinThumbnailToolBar(this);
    m_thumbnailToolBar->setWindow(windowHandle());

    auto togglePlay = new QWinThumbnailToolButton(m_thumbnailToolBar);
    connect(togglePlay, &QWinThumbnailToolButton::clicked, menuBar->player->togglePlay, &QAction::trigger);
    connect(menuBar->player->togglePlay, &QAction::changed, togglePlay, [=] {
        togglePlay->setToolTip(menuBar->player->togglePlay->toolTip());
        togglePlay->setIcon(menuBar->player->togglePlay->icon());
    });

    auto stop = new QWinThumbnailToolButton(m_thumbnailToolBar);
    connect(stop, &QWinThumbnailToolButton::clicked, menuBar->player->stop, &QAction::trigger);
    stop->setToolTip(menuBar->player->stop->toolTip());
    stop->setIcon(menuBar->player->stop->icon());

    auto prev = new QWinThumbnailToolButton(m_thumbnailToolBar);
    connect(prev, &QWinThumbnailToolButton::clicked, menuBar->player->prev, &QAction::trigger);
    prev->setToolTip(menuBar->player->prev->toolTip());
    prev->setIcon(menuBar->player->prev->icon());

    auto next = new QWinThumbnailToolButton(m_thumbnailToolBar);
    connect(next, &QWinThumbnailToolButton::clicked, menuBar->player->next, &QAction::trigger);
    next->setToolTip(menuBar->player->next->toolTip());
    next->setIcon(menuBar->player->next->icon());

    auto toggleFullScreen = new QWinThumbnailToolButton(m_thumbnailToolBar);
    connect(toggleFullScreen, &QWinThumbnailToolButton::clicked, menuBar->window->toggleFullScreen, &QAction::trigger);
    toggleFullScreen->setToolTip(menuBar->window->toggleFullScreen->toolTip());
    toggleFullScreen->setIcon(menuBar->window->toggleFullScreen->icon());
    toggleFullScreen->setDismissOnClick(true);

    auto toggleMute = new QWinThumbnailToolButton(m_thumbnailToolBar);
    connect(toggleMute, &QWinThumbnailToolButton::clicked, menuBar->player->toggleMute, &QAction::trigger);
    connect(menuBar->player->toggleMute, &QAction::changed, toggleMute, [=] {
        toggleMute->setIcon(menuBar->player->toggleMute->icon());
    });
    toggleMute->setToolTip(menuBar->player->toggleMute->toolTip());
    toggleMute->setIcon(menuBar->player->toggleMute->icon());

    auto settings = new QWinThumbnailToolButton(m_thumbnailToolBar);
    connect(settings, &QWinThumbnailToolButton::clicked, menuBar->options->settings, &QAction::trigger);
    settings->setToolTip(menuBar->options->settings->toolTip());
    settings->setIcon(menuBar->options->settings->icon());
    settings->setDismissOnClick(true);

    m_thumbnailToolBar->addButton(togglePlay);
    m_thumbnailToolBar->addButton(stop);
    m_thumbnailToolBar->addButton(prev);
    m_thumbnailToolBar->addButton(next);
    m_thumbnailToolBar->addButton(toggleFullScreen);
    m_thumbnailToolBar->addButton(toggleMute);
    m_thumbnailToolBar->addButton(settings);
}
#endif
#if defined(Q_OS_WIN) && QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
void MainWidget::setTitleBarStyle()
{
    if (auto winId = reinterpret_cast<HWND>(window()->internalWinId()))
    {
        constexpr WORD DwmwaUseImmersiveDarkMode = 20;
        constexpr WORD DwmwaUseImmersiveDarkModeBefore20h1 = 19;
        const BOOL darkBorder = (palette().window().color().lightnessF() < 0.5) ? TRUE : FALSE;
        if (DwmSetWindowAttribute(winId, DwmwaUseImmersiveDarkMode, &darkBorder, sizeof(darkBorder)) != S_OK)
            DwmSetWindowAttribute(winId, DwmwaUseImmersiveDarkModeBefore20h1, &darkBorder, sizeof(darkBorder));
    }
}
#endif

void MainWidget::keyPressEvent(QKeyEvent *e)
{
#ifdef Q_OS_ANDROID
    if (e->key() == Qt::Key_Back && fullScreen)
    {
        toggleFullScreen();
        e->accept();
        return;
    }
#endif
    // Trigger group sync on quick group sync key shortcut if quick group sync is unavailable.
    // This works only in quick group sync has only one key shourtcut.
    QAction *sync = menuBar->playlist->sync;
    QAction *quickSync = menuBar->playlist->quickSync;
    if (!quickSync->isVisible() && sync->isVisible() && (!e->isAutoRepeat() || sync->autoRepeat()))
    {
        const QKeySequence keySeq = quickSync->shortcut();
        if (keySeq.count() == 1 && keySeq.matches(e->key()))
            sync->trigger();
    }
    QMainWindow::keyPressEvent(e);
}
void MainWidget::mouseMoveEvent(QMouseEvent *e)
{
    static bool inRestoreState = false;
    if (!inRestoreState && (fullScreen || isCompactView) && (e->buttons() == Qt::NoButton || videoDock->isTouch))
    {
        const bool isToolbarVisible = mainTB->isVisible();

        bool canDisplayLeftPanel = fullScreen || isCompactView;
        if (canDisplayLeftPanel && !isToolbarVisible)
        {
            const auto winScreen = windowHandle()->screen();
            const auto winScreenGeo = winScreen->geometry();
            if (winScreenGeo.x() != 0)
            {
                const auto screens = QGuiApplication::screens();
                for (auto &&screen : screens)
                {
                    if (screen == winScreen)
                        continue;

                    auto geo = screen->geometry();
                    if (geo.x() >= winScreenGeo.x())
                        continue;

                    geo.moveLeft(winScreenGeo.x());
                    if (winScreenGeo.intersects(geo))
                    {
                        canDisplayLeftPanel = false;
                        break;
                    }
                }
            }
        }

        const int trigger1 = canDisplayLeftPanel
            ? qMax<int>(5, ceil(0.003 * (videoDock->isTouch ? 8 : 1) * width()))
            : 0
        ;
        const int trigger2 = qMax<int>(15, ceil(0.025 * (videoDock->isTouch ? 4 : 1) * width()));

        if (videoDock->touchEnded)
            videoDock->isTouch = videoDock->touchEnded = false;

        int mPosX = 0;
        if (videoDock->x() >= 0)
            mPosX = videoDock->mapFromGlobal(e->globalPos()).x();

        /* ToolBar */
        if (!playlistDock->isVisible() && mPosX >= trigger1)
            showToolBar(e->pos().y() >= height() - mainTB->height() - statusBar->height() + 10);

        /* DockWidgets */
        if (canDisplayLeftPanel && !playlistDock->isVisible() && mPosX <= trigger1)
        {
            showToolBar(true); //Before restoring dock widgets - show toolbar and status bar

            inRestoreState = true;
            if (fullScreen)
                restoreState(fullScreenDockWidgetState);
            if (isCompactView)
                restoreState(m_compactViewDockWidgetState);
            inRestoreState = false;

            const QList<QDockWidget *> tDW = tabifiedDockWidgets(infoDock);
            bool reloadQMPlay2Extensions = false;
            for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
                if (DockWidget *dw = QMPlay2Ext->getDockWidget())
                {
                    if (!visibleQMPlay2Extensions.contains(QMPlay2Ext) || QMPlay2Ext->isVisualization())
                    {
                        if (dw->isVisible())
                            dw->hide();
                        continue;
                    }
                    if (dw->isFloating())
                        dw->setFloating(false);
                    if (!tDW.contains(dw))
                        reloadQMPlay2Extensions = true;
                    else if (!dw->isVisible())
                        dw->show();
                }
            if (reloadQMPlay2Extensions || !playlistDock->isVisible() || !infoDock->isVisible())
            {
                playlistDock->hide();
                infoDock->hide();
                hideAllExtensions();

                addDockWidget(Qt::LeftDockWidgetArea, playlistDock);
                addDockWidget(Qt::LeftDockWidgetArea, infoDock);

                playlistDock->show();
                infoDock->show();

                for (QMPlay2Extensions *QMPlay2Ext : std::as_const(visibleQMPlay2Extensions))
                    if (!QMPlay2Ext->isVisualization())
                        if (DockWidget *dw = QMPlay2Ext->getDockWidget())
                        {
                            tabifyDockWidget(infoDock, dw);
                            dw->show();
                        }
            }
            showToolBar(true); //Ensures that status bar is visible
        }
        else if ((fullScreen || isCompactView) && playlistDock->isVisible() && mPosX > trigger2)
        {
            hideDocks();
        }
    }
    QMainWindow::mouseMoveEvent(e);
}

void MainWidget::enterEvent(Q_ENTER_EVENT *e)
{
    m_hideDocksTimer.stop();
    QMainWindow::enterEvent(e);
}
void MainWidget::leaveEvent(QEvent *e)
{
    if (fullScreen || isCompactView)
        m_hideDocksTimer.start(isCompactView ? 750 : 0);
    QMainWindow::leaveEvent(e);
}
void MainWidget::closeEvent(QCloseEvent *e)
{
    Settings &settings = QMPlay2Core.getSettings();

#ifndef Q_OS_ANDROID
    if (!sender() && settings.getBool("MainWidget/HideOnClose"))
    {
        e->ignore();
        toggleVisibility();
        return;
    }
#endif

    const QString quitMsg = tr("Are you sure you want to quit?");
    if
    (
        (QMPlay2Core.isWorking() && QMessageBox::question(this, QString(), tr("QMPlay2 is doing something in the background.") + " " + quitMsg, QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
#ifdef UPDATER
        || (updater.downloading() && QMessageBox::question(this, QString(), tr("The update is being downloaded now.") + " " + quitMsg, QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
#endif
    )
    {
        QMPlay2GUI.restartApp = QMPlay2GUI.removeSettings = false;
        e->ignore();
        return;
    }

    emit QMPlay2Core.waitCursor();

    if (QMPlay2GUI.pipe)
        disconnect(QMPlay2GUI.pipe.get(), SIGNAL(newConnection(IPCSocket *)), this, SLOT(newConnection(IPCSocket *)));

    if (wasShow)
    {
        if (!fullScreen && !isCompactView)
            settings.set("MainWidget/DockWidgetState", saveState());
        else
            settings.set("MainWidget/DockWidgetState", dockWidgetState);
    }
    settings.set("MainWidget/FullScreenDockWidgetState", fullScreenDockWidgetState);
    settings.set("MainWidget/CompactViewDockWidgetState", m_compactViewDockWidgetState);
    settings.set("MainWidget/AlwaysOnTop", !!(windowFlags() & Qt::WindowStaysOnTopHint));
#ifndef Q_OS_MACOS
    settings.set("MainWidget/isVisible", isVisible());
#endif
    settings.set("MainWidget/IsCompactView", fullScreen ? m_compactViewBeforeFullScreen : isCompactView);
    if (tray)
        settings.set("TrayVisible", tray->isVisible());
    settings.set("VolumeL", volW->volumeL());
    settings.set("VolumeR", volW->volumeR());
    settings.set("Mute", menuBar->player->toggleMute->isChecked());
    for (int i = 0; i < RepeatModeCount; ++i)
        if (menuBar->player->repeat->repeatActions[i]->isChecked())
        {
            settings.set("RepeatMode", i);
            break;
        }
    QMPlay2GUI.videoAdjustment->saveValues();

    hide();

    if (playC.isPlaying() && settings.getBool("SavePos"))
    {
        settings.set("Pos", playC.getPos());
        settings.set("Url", playC.getUrl());
    }
    else
    {
        settings.remove("Pos");
        settings.remove("Url");
    }

    playlistDock->stopThreads();

    if (m_loaded)
    {
        if (settings.getBool("AutoDelNonGroupEntries"))
            playlistDock->delNonGroupEntries(true);
        playlistDock->save(QMPlay2Core.getSettingsDir() + "Playlist.pls");
    }

    playC.stop(true);

#if defined(Q_OS_WIN)
    m_taskBarProgress = nullptr;
    delete m_taskbarButton;
    delete m_thumbnailToolBar;
#endif
}
void MainWidget::moveEvent(QMoveEvent *e)
{
    if (videoDock->isVisible() && !videoDock->isFloating())
        emit QMPlay2Core.videoDockMoved();
    QMainWindow::moveEvent(e);
}
void MainWidget::showEvent(QShowEvent *)
{
    if (!wasShow)
    {
#ifndef Q_OS_ANDROID
        QMPlay2GUI.restoreGeometry("MainWidget/Geometry", this, 80);
        savedGeo = winGeo();
        if (QMPlay2Core.getSettings().getBool("MainWidget/isMaximized"))
#endif
            showMaximized();
        doRestoreState(QMPlay2Core.getSettings().getByteArray("MainWidget/DockWidgetState"), QMPlay2Core.getSettings().getBool("MainWidget/IsCompactView"));
#if defined(Q_OS_WIN)
        setWindowsTaskBarFeatures();
#endif
#if defined(Q_OS_WIN) && QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
        setTitleBarStyle();
#endif
        wasShow = true;
    }
    menuBar->window->toggleVisibility->setText(tr("&Hide"));
}
void MainWidget::hideEvent(QHideEvent *)
{
#ifndef Q_OS_ANDROID
    if (wasShow)
    {
        const auto geo = winGeo();
        QRect geoToStore;
        if (!fullScreen && !isMaximized())
        {
            geoToStore = geo;
        }
        else if (!fullScreen) // isMaximized()
        {
            geoToStore.setTopLeft(geo.topLeft());
            geoToStore.setSize(savedGeo.size());
        }
        else // fullScreen
        {
            geoToStore = savedGeo;
        }
        QMPlay2Core.getSettings().set("MainWidget/Geometry", geoToStore);
        QMPlay2Core.getSettings().set("MainWidget/isMaximized", fullScreen ? m_maximizedBeforeFullScreen : isMaximized());
    }
#endif
    menuBar->window->toggleVisibility->setText(tr("&Show"));
}
void MainWidget::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::WindowStateChange)
    {
        const bool fs = isFullScreen();
        if (fs || isMaximized())
            videoDock->scheduleEnterEventWorkaround();
        else if (!fs)
            videoDock->maybeTriggerWidgetVisibility();
    }
#if defined(Q_OS_WIN) && QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
    else if (e->type() == QEvent::PaletteChange)
    {
        setTitleBarStyle();
    }
#endif
    QMainWindow::changeEvent(e);
}

void MainWidget::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);

    if (m_keepDocksSize && !m_dockSizes.empty() && !m_winSizeForDocks.isEmpty() && e->spontaneous())
    {
        const double wRatio = static_cast<double>(width()) / static_cast<double>(m_winSizeForDocks.width());
        const double hRatio = static_cast<double>(height()) / static_cast<double>(m_winSizeForDocks.height());

        QList<QDockWidget *> docks;
        QList<int> sizesW, sizesH;

        for (auto &&dock : getDockWidgets())
        {
            dock->setResizingByMainWindow(true);
        }

        docks.reserve(m_dockSizes.size());
        sizesW.reserve(m_dockSizes.size());
        sizesH.reserve(m_dockSizes.size());

        for (auto &&[dock, size] : m_dockSizes)
        {
            docks.push_back(dock);
            sizesW.push_back(qRound(size.width() * wRatio));
            sizesH.push_back(qRound(size.height() * hRatio));
        }

        resizeDocks(docks, sizesH, Qt::Vertical);
        resizeDocks(docks, sizesW, Qt::Horizontal);
    }
}

bool MainWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (tray && obj == tray && event->type() == QEvent::Wheel)
    {
        QWheelEvent *we = static_cast<QWheelEvent *>(event);
        volW->changeVolume(we->angleDelta().y() / 30);
    }
#ifdef Q_OS_MACOS
    else if (event->type() == QEvent::FileOpen)
    {
        filesToAdd.append(((QFileOpenEvent *)event)->file());
        fileOpenTimer.start(10);
    }
#endif
    else if (event->type() == QEvent::Resize && m_keepDocksSize && obj == windowHandle())
    {
        for (auto &&dock : getDockWidgets())
        {
            dock->setResizingByMainWindow(true);
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

#ifdef Q_OS_MACOS
void MainWidget::fileOpenTimerTimeout()
{
    if (filesToAdd.count() == 1)
        playlistDock->addAndPlay(filesToAdd.at(0));
    else
        playlistDock->addAndPlay(filesToAdd);
    filesToAdd.clear();
}
#endif
