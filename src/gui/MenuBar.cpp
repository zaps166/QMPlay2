/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <MenuBar.hpp>

#include <VideoAdjustmentW.hpp>
#include <ShortcutHandler.hpp>
#include <DockWidget.hpp>
#include <Functions.hpp>
#include <Settings.hpp>
#include <Main.hpp>

#include <QWidgetAction>
#include <QInputDialog>
#include <QMainWindow>
#include <qevent.h>
#include <QDir>
#ifdef Q_OS_MACOS
    #include <QTimer>
#endif

static QAction *newAction(const QString &txt, QMenu *parent, QAction *&act, bool autoRepeat, const QIcon &icon, bool checkable, QAction::MenuRole role = QAction::NoRole)
{
    act = new QAction(txt, parent);
    act->setAutoRepeat(autoRepeat);
    act->setCheckable(checkable);
    parent->addAction(act);
    act->setMenuRole(role);
    act->setIcon(icon);
    return act;
}

static void restartApp()
{
    QMPlay2GUI.restartApp = true;
    QMPlay2GUI.cmdLineProfile.clear();
    QMPlay2GUI.mainW->close();
}

static inline QString getCurrentProfile(const QSettings &profileSettings)
{
    return QMPlay2GUI.cmdLineProfile.isEmpty() ? profileSettings.value("Profile", "/").toString() : QMPlay2GUI.cmdLineProfile;
}

static QString createAndSetProfile(MenuBar *menuBar)
{
    const QString selectedProfile = Functions::cleanFileName(QInputDialog::getText(menuBar, MenuBar::Options::tr("Create new profile"), MenuBar::Options::tr("Enter new profile name:")));
    if (!selectedProfile.isEmpty())
    {
        QSettings profileSettings(QMPlay2Core.getSettingsDir() + "Profile.ini", QSettings::IniFormat);
        if (selectedProfile.compare(getCurrentProfile(profileSettings), Qt::CaseInsensitive) != 0)
        {
            QDir(QMPlay2Core.getSettingsDir()).mkpath("Profiles/" + selectedProfile);

            profileSettings.setValue("Profile", selectedProfile);
            restartApp();

            return selectedProfile;
        }
    }
    return QString();
}

/**/

MenuBar::MenuBar()
{
    addMenu(window = new Window(this));
    addMenu(widgets = new Widgets(this));
    addMenu(playlist = new Playlist(this));
    addMenu(player = new Player(this));
    addMenu(playback = new Playback(this));
    addMenu(options = new Options(this));
    addMenu(help = new Help(this));
    connect(widgets, SIGNAL(aboutToShow()), this, SLOT(widgetsMenuShow()));
#ifdef Q_OS_MACOS
    widgets->addAction(QString()); //Mac must have got at least one item inside menu, otherwise the menu is not shown
#endif

    QMPlay2Core.registerProcessWheelEventFn([this](QWheelEvent *e) {
        auto &settings = QMPlay2Core.getSettings();
        if (e->orientation() == Qt::Vertical && e->buttons() == Qt::NoButton && settings.getBool("WheelAction"))
        {
            if (settings.getBool("WheelSeek"))
                e->delta() > 0 ? player->seekF->trigger() : player->seekB->trigger();
            else if (settings.getBool("WheelVolume"))
                e->delta() > 0 ? player->volUp->trigger() : player->volDown->trigger();
        }
    });
}
MenuBar::~MenuBar()
{
    QMPlay2Core.registerProcessWheelEventFn(nullptr);
}

MenuBar::Window::Window(MenuBar *parent) :
    QMenu(Window::tr("Window(&W)"), parent)
{
    newAction(QString(), this, toggleVisibility, false, QIcon(), false);
    newAction(Window::tr("Full screen(&F)"), this, toggleFullScreen, false, QMPlay2Core.getIconFromTheme("view-fullscreen"), false);
    newAction(Window::tr("Compact view(&C)"), this, toggleCompactView, false, QIcon(), true);
    addSeparator();
    newAction(Window::tr("Always on top(&A)"), this, alwaysOnTop, false, QIcon(), true);
    addSeparator();
    newAction(Window::tr("Close(&C)"), this, close, false, QMPlay2Core.getIconFromTheme("application-exit"), false, QAction::QuitRole);
}

MenuBar::Widgets::Widgets(MenuBar *parent) :
    QMenu(Widgets::tr("Widgets(&W)"), parent)
{}
void MenuBar::Widgets::menuShow()
{
    clear();
    QMenu *menu = qobject_cast<QMainWindow *>(QMPlay2GUI.mainW)->createPopupMenu();
    if (menu)
    {
        for (QAction *act : menu->actions())
        {
            if (act->parent() == menu)
                act->setParent(this);
            addAction(act);
        }
        menu->deleteLater();
    }
}

MenuBar::Playlist::Playlist(MenuBar *parent) :
    QMenu(Playlist::tr("Playlist(&P)"), parent)
{
    add = new Add(this);
    addMenu(add);

    addSeparator();
    newAction(Playlist::tr("Stop loading(&S)"), this, stopLoading, false, QMPlay2Core.getIconFromTheme("process-stop"), false);
    addSeparator();
    newAction(Playlist::tr("Synchronize groups(&S)"), this, sync, false, QMPlay2Core.getIconFromTheme("view-refresh"), false);
    newAction(Playlist::tr("Quick group synchronization(&Q)"), this, quickSync, false, QMPlay2Core.getIconFromTheme("view-refresh"), false);
    addSeparator();
    newAction(Playlist::tr("Load list(&L)"), this, loadPlist, false, QIcon(), false);
    newAction(Playlist::tr("Save list(&L)"), this, savePlist, false, QIcon(), false);
    newAction(Playlist::tr("Save group(&G)"), this, saveGroup, false, QIcon(), false);
    addSeparator();
    newAction(QString(), this, lock, false, QIcon(), false);
    newAction(Playlist::tr("Always sync(&A)"), this, alwaysSync, false, QIcon(), true);
    addSeparator();
    newAction(Playlist::tr("Remove selected entries(&R)"), this, delEntries, true, QMPlay2Core.getIconFromTheme("list-remove"), false);
    newAction(Playlist::tr("Remove entries without groups(&W)"), this, delNonGroupEntries, false, QMPlay2Core.getIconFromTheme("list-remove"), false);
    newAction(Playlist::tr("Clear list(&C)"), this, clear, false, QMPlay2Core.getIconFromTheme("archive-remove"), false);
    addSeparator();
    newAction(Playlist::tr("Copy(&C)"), this, copy, false, QMPlay2Core.getIconFromTheme("edit-copy"), false);
    newAction(Playlist::tr("Paste(&P)"), this, paste, false, QMPlay2Core.getIconFromTheme("edit-paste"), false);
    addSeparator();

    extensions = new QMenu(Playlist::tr("Extensions(&E)"), this);
    extensions->setEnabled(false);
    addMenu(extensions);

    addSeparator();
    newAction(Playlist::tr("Create group(&C)"), this, newGroup, false, QMPlay2Core.getIconFromTheme("folder-new"), false);
    newAction(Playlist::tr("Rename(&R)"), this, renameGroup, false, QIcon(), false);
    addSeparator();
    newAction(Playlist::tr("Find entries(&F)"), this, find, false, QMPlay2Core.getIconFromTheme("edit-find"), false);
    addSeparator();

    sort = new Sort(this);
    addMenu(sort);

    addSeparator();
    newAction(Playlist::tr("Collapse all(&C)"), this, collapseAll, false, QIcon(), false);
    newAction(Playlist::tr("Expand all(&E)"), this, expandAll, false, QIcon(), false);
    addSeparator();
    newAction(Playlist::tr("Go to the playback(&G)"), this, goToPlayback, false, QIcon(), false);
    addSeparator();
    newAction(Playlist::tr("Enqueue(&E)"), this, queue, false, QIcon(), false);
    newAction(Player::tr("Skip(&S)"), this, skip, false, QIcon(), false);
    newAction(Player::tr("Stop after playing(&S)"), this, stopAfter, false, QIcon(), false);
    addSeparator();
    newAction(Playlist::tr("Properties(&P)"), this, entryProperties, false, QMPlay2Core.getIconFromTheme("document-properties"), false);
}
MenuBar::Playlist::Add::Add(QMenu *parent) :
    QMenu(Add::tr("Add(&A)"), parent)
{
    setIcon(QMPlay2Core.getIconFromTheme("list-add"));
    newAction(Add::tr("Files(&F)"), this, file, false, *QMPlay2GUI.mediaIcon, false);
    newAction(Add::tr("Directory(&D)"), this, dir, false, QMPlay2Core.getIconFromTheme("folder"), false);
    newAction(Add::tr("Address(&A)"), this, address, false, QMPlay2Core.getIconFromTheme("application-x-mswinurl"), false);
    addSeparator();
}
MenuBar::Playlist::Sort::Sort(QMenu *parent) :
    QMenu(Sort::tr("Sort(&S)"), parent)
{
    newAction(Sort::tr("From the shortest to the longest(&F)"), this, timeSort1, false, QIcon(), false);
    newAction(Sort::tr("From the longest to the shortest(&F)"), this, timeSort2, false, QIcon(), false);
    addSeparator();
    newAction(Sort::tr("A-Z(&A)"), this, titleSort1, false, QIcon(), false);
    newAction(Sort::tr("Z-A(&Z)"), this, titleSort2, false, QIcon(), false);
}

MenuBar::Player::Player(MenuBar *parent) :
    QMenu(Player::tr("Player(&P)"), parent)
{
    newAction(QString(), this,  togglePlay, false, QMPlay2Core.getIconFromTheme("media-playback-start"), false);
    newAction(Player::tr("Stop(&S)"), this, stop, false, QMPlay2Core.getIconFromTheme("media-playback-stop"), false);
    newAction(Player::tr("Next(&N)"), this, next, true, QMPlay2Core.getIconFromTheme("media-skip-forward"), false);
    newAction(Player::tr("Previous(&P)"), this, prev, true, QMPlay2Core.getIconFromTheme("media-skip-backward"), false);
    newAction(Player::tr("Previous frame(&F)"), this, prevFrame, true, QIcon(), false);
    newAction(Player::tr("Next frame(&F)"), this, nextFrame, true, QIcon(), false);
    addSeparator();

    repeat = new Repeat(this);
    addMenu(repeat);

    newAction(Player::tr("A-B Repeat(&-)"), this, abRepeat, true, QIcon(), false);

    addSeparator();
    newAction(Player::tr("Seek forward(&F)"), this, seekF, true, QIcon(), false);
    newAction(Player::tr("Seek backward(&B)"), this, seekB, true, QIcon(), false);
    newAction(Player::tr("Long seek &forward(&S)"), this, lSeekF, true, QIcon(), false);
    newAction(Player::tr("Long seek backward(&E)"), this, lSeekB, true, QIcon(), false);
    addSeparator();
    newAction(Player::tr("Faster(&S)"), this, speedUp, true, QIcon(), false);
    newAction(Player::tr("Slower(&R)"), this, slowDown, true, QIcon(), false);
    newAction(Player::tr("Set speed(&S)"), this, setSpeed, false, QIcon(), false);
    addSeparator();
    newAction(Player::tr("Zoom in(&N)"), this, zoomIn, true, QIcon(), false);
    newAction(Player::tr("Zoom out(&T)"), this, zoomOut, true, QIcon(), false);
    newAction(Player::tr("Toggle aspect ratio(&A)"), this, switchARatio, true, QIcon(), false);

    aRatio = new AspectRatio(this);
    addMenu(aRatio);

    newAction(Player::tr("Reset image settings(&S)"), this, reset, false, QIcon(), false);

    addSeparator();
    newAction(Player::tr("Volume up(&U)"), this, volUp, true, QIcon(), false);
    newAction(Player::tr("Volume down(&D)"), this, volDown, true, QIcon(), false);
    newAction(Player::tr("Mute(&M)"), this, toggleMute, false, QMPlay2Core.getIconFromTheme("audio-volume-high"), true);

    if (!QMPlay2GUI.pipe)
        detach = nullptr;
    else
    {
        addSeparator();
        newAction(Player::tr("Detach from receiving commands(&C)"), this, detach, false, QIcon(), false);
    }

    if (QMPlay2Core.canSuspend())
    {
        addSeparator();
        newAction(Player::tr("Suspend after playback is finished(&K)"), this, suspend, false, QIcon(), true);
    }
}
MenuBar::Player::Repeat::Repeat(QMenu *parent) :
    QMenu(Repeat::tr("Repeat(&R)"), parent)
{
    choice = new QActionGroup(this);
    choice->addAction(newAction(Repeat::tr("No repeating(&N)"), this, repeatActions[RepeatNormal], false, QIcon(), true));
    addSeparator();
    choice->addAction(newAction(Repeat::tr("Entry repeating(&E)"), this, repeatActions[RepeatEntry], false, QIcon(), true));
    choice->addAction(newAction(Repeat::tr("Group repeating(&G)"), this, repeatActions[RepeatGroup], false, QIcon(), true));
    choice->addAction(newAction(Repeat::tr("Playlist repeating(&P)"), this, repeatActions[RepeatList], false, QIcon(), true));
    addSeparator();
    choice->addAction(newAction(Repeat::tr("Random(&A)"), this, repeatActions[RandomMode], false, QIcon(), true));
    choice->addAction(newAction(Repeat::tr("Random in group(&G)"), this, repeatActions[RandomGroupMode], false, QIcon(), true));
    addSeparator();
    choice->addAction(newAction(Repeat::tr("Random and repeat(&R)"), this, repeatActions[RepeatRandom], false, QIcon(), true));
    choice->addAction(newAction(Repeat::tr("Random in group and repeat(&T)"), this, repeatActions[RepeatRandomGroup], false, QIcon(), true));
    addSeparator();
    choice->addAction(newAction(Repeat::tr("Stop playback after every file(&S)"), this, repeatActions[RepeatStopAfter], false, QIcon(), true));

    for (int i = 0; i < RepeatModeCount; ++i)
        repeatActions[i]->setProperty("enumValue", i);
}
MenuBar::Player::AspectRatio::AspectRatio(QMenu *parent) :
    QMenu(AspectRatio::tr("Aspect ratio(&A)"), parent)
{
    choice = new QActionGroup(this);
    choice->addAction(newAction(AspectRatio::tr("Auto(&A)"), this, _auto, false, QIcon(), true));
    addSeparator();
    choice->addAction(newAction("1:1(&1)", this, _1x1, false, QIcon(), true));
    choice->addAction(newAction("4:3(&4)", this, _4x3, false, QIcon(), true));
    choice->addAction(newAction("5:4(&5)", this, _5x4, false, QIcon(), true));
    choice->addAction(newAction("16:9(&1)", this, _16x9, false, QIcon(), true));
    choice->addAction(newAction("3:2(&3)", this, _3x2, false, QIcon(), true));
    choice->addAction(newAction("21:9(&2)", this, _21x9, false, QIcon(), true));
    addSeparator();
    choice->addAction(newAction(AspectRatio::tr("Depends on size(&E)"), this, sizeDep, false, QIcon(), true));
    choice->addAction(newAction(AspectRatio::tr("Disabled(&D)"), this, off, false, QIcon(), true));

    _auto->setObjectName("auto");
    _1x1->setObjectName("1");
    _4x3->setObjectName(QString::number(4.0 / 3.0));
    _5x4->setObjectName(QString::number(5.0 / 4.0));
    _16x9->setObjectName(QString::number(16.0 / 9.0));
    _3x2->setObjectName(QString::number(3.0 / 2.0));
    _21x9->setObjectName(QString::number(21.0 / 9.0));
    sizeDep->setObjectName("sizeDep");
    off->setObjectName("off");
}

void MenuBar::Player::seekActionsEnable(bool e)
{
#ifndef Q_OS_MACOS
    Qt::ShortcutContext ctx = e ? Qt::WindowShortcut : Qt::WidgetShortcut;
    seekF->setShortcutContext(ctx);
    seekB->setShortcutContext(ctx);
    lSeekF->setShortcutContext(ctx);
    lSeekB->setShortcutContext(ctx);
#else
    seekF->setEnabled(e);
    seekB->setEnabled(e);
    lSeekF->setEnabled(e);
    lSeekB->setEnabled(e);
#endif
}
void MenuBar::Player::playActionEnable(bool e)
{
#ifndef Q_OS_MACOS
    togglePlay->setShortcutContext(e ? Qt::WindowShortcut : Qt::WidgetShortcut);
#else
    togglePlay->setEnabled(e);
#endif
}

MenuBar::Playback::Playback(MenuBar *parent) :
    QMenu(Playback::tr("Playback(&P)"), parent)
{
    newAction(Playback::tr("Enable audio(&E)"), this, toggleAudio, false, QIcon(), true)->setObjectName("toggleAudio");
    toggleAudio->setChecked(true);

    audioChannels = new AudioChannels(this);
    addMenu(audioChannels);

    audioStreams = new Streams(Streams::tr("Audio streams(&A)"), this);
    audioStreams->menuAction()->setVisible(false);
    addMenu(audioStreams);

    addSeparator();

    newAction(Playback::tr("Enable video(&E)"), this, toggleVideo, false, QIcon(), true)->setObjectName("toggleVideo");
    toggleVideo->setChecked(true);

    videoFilters = new VideoFilters(this);
    addMenu(videoFilters);

    newAction(Playback::tr("Set video delay(&V)"), this, videoSync, false, QIcon(), false);
    newAction(Playback::tr("Delay video(&D)") + " (100ms)", this, slowDownVideo, true, QIcon(), false);
    newAction(Playback::tr("Speed up video(&S)") + " (100ms)", this, speedUpVideo, true, QIcon(), false);

    videoStreams = new Streams(Streams::tr("Video streams(&V)"), this);
    videoStreams->menuAction()->setVisible(false);
    addMenu(videoStreams);

    addSeparator();

    newAction(Playback::tr("Enable subtitles(&E)"), this, toggleSubtitles, false, QIcon(), true)->setObjectName("toggleSubtitles");
    toggleSubtitles->setChecked(true);
    newAction(Playback::tr("Add subtities from file(&S)"), this, subsFromFile, false, QIcon(), false);
    newAction(Playback::tr("Set subtitles delay(&S)"), this, subtitlesSync, true, QIcon(), false);
    newAction(Playback::tr("Delay subtitiles(&D)") + " (100ms)", this, slowDownSubtitles, true, QIcon(), false);
    newAction(Playback::tr("Speed up subtitles(&S)") + " (100ms)", this, speedUpSubtitles, true, QIcon(), false);
    newAction(Playback::tr("Scale up subtitles(&I)"), this, biggerSubtitles, true, QIcon(), false);
    newAction(Playback::tr("Scale down subtitles(&T)"), this, smallerSubtitles, true, QIcon(), false);

    subtitlesStreams = new Streams(Streams::tr("Subtitles streams(&S)"), this);
    subtitlesStreams->menuAction()->setVisible(false);
    addMenu(subtitlesStreams);

    addSeparator();

    chapters = new Streams(Streams::tr("Chapters(&C)"), this, false);
    chapters->menuAction()->setVisible(false);
    addMenu(chapters);

    programs = new Streams(Streams::tr("Programs(&P)"), this);
    programs->menuAction()->setVisible(false);
    addMenu(programs);

    addSeparator();

    newAction(Playback::tr("Screen shot(&S)"), this, screenShot, true, QIcon(), false);
}
MenuBar::Playback::~Playback()
{
    Settings &QMPSettings = QMPlay2Core.getSettings();
    if (QMPSettings.getBool("RestoreAVSState"))
    {
        QMPSettings.set("AudioEnabled", toggleAudio->isChecked());
        QMPSettings.set("VideoEnabled", toggleVideo->isChecked());
        QMPSettings.set("SubtitlesEnabled", toggleSubtitles->isChecked());
    }
    else
    {
        QMPSettings.remove("AudioEnabled");
        QMPSettings.remove("VideoEnabled");
        QMPSettings.remove("SubtitlesEnabled");
    }
}

MenuBar::Playback::VideoFilters::VideoFilters(QMenu *parent) :
    QMenu(VideoFilters::tr("Video filters(&F)"), parent)
{
    /** Korektor wideo */
    videoAdjustmentMenu = new QMenu(VideoFilters::tr("Video adjustment(&A)"), this);
    addMenu(videoAdjustmentMenu);
    QWidgetAction *widgetAction = new QWidgetAction(this);
    widgetAction->setDefaultWidget(QMPlay2GUI.videoAdjustment);
    QMPlay2GUI.videoAdjustment->setObjectName(videoAdjustmentMenu->title().remove('&'));
    videoAdjustmentMenu->addAction(widgetAction);
#ifdef Q_OS_MACOS
    // Update visibility and update geometry of video adjustment widget
    connect(videoAdjustmentMenu, &VideoFilters::aboutToShow, [] {
        if (QWidget *parent = QMPlay2GUI.videoAdjustment->parentWidget())
        {
            if (qstrcmp(parent->metaObject()->className(), "QMenu") == 0)
            {
                QTimer::singleShot(0, [parent] {
                    QMPlay2GUI.videoAdjustment->setGeometry(QRect(QPoint(), parent->sizeHint()));
                });
            }
        }
    });
#endif
    /**/
    addSeparator();
    newAction(VideoFilters::tr("Spherical view(&S)"), this, spherical, true, QIcon(), true);
    addSeparator();
    newAction(VideoFilters::tr("Horizontal flip(&H)"), this, hFlip, true, QIcon(), true);
    newAction(VideoFilters::tr("Vertical flip(&AV)"), this, vFlip, true, QIcon(), true);
    newAction(VideoFilters::tr("Rotate 90°(&R)"), this, rotate90, true, QIcon(), true);
    addSeparator();
    newAction(VideoFilters::tr("More filters(&M)"), this, more, false, QIcon(), false);
}
MenuBar::Playback::AudioChannels::AudioChannels(QMenu *parent) :
    QMenu(AudioChannels::tr("Channels(&C)") , parent)
{
    choice = new QActionGroup(this);
    choice->addAction(newAction(AudioChannels::tr("Autodetect(&A)"), this, _auto, false, QIcon(), true));
    addSeparator();
    choice->addAction(newAction(AudioChannels::tr("Mono(&M)"), this, _1, false, QIcon(), true));
    choice->addAction(newAction(AudioChannels::tr("Stereo(&S)"), this, _2, false, QIcon(), true));
    choice->addAction(newAction("4.0(&4)", this, _4, false, QIcon(), true));
    choice->addAction(newAction("5.1(&5)", this, _6, false, QIcon(), true));
    choice->addAction(newAction("7.1(&7)", this, _8, false, QIcon(), true));
    addSeparator();
    choice->addAction(newAction(AudioChannels::tr("Other(&O)"), this, other, false, QIcon(), true));

    _auto->setObjectName("auto");
    _1->setObjectName("1");
    _2->setObjectName("2");
    _4->setObjectName("4");
    _6->setObjectName("6");
    _8->setObjectName("8");
    other->setEnabled(false);
}
MenuBar::Playback::Streams::Streams(const QString &title, QMenu *parent, bool createGroup)
    : QMenu(title, parent)
    , group(createGroup ? new QActionGroup(this) : nullptr)
{
}

MenuBar::Options::Options(MenuBar *parent) :
    QMenu(Options::tr("Options(&T)"), parent)
{
    const QIcon configureIcon = QMPlay2Core.getIconFromTheme("configure");
    newAction(Options::tr("Settings(&S)"), this, settings, false, configureIcon, false, QAction::PreferencesRole);
    newAction(Options::tr("Renderer settings(&R)"), this, rendererSettings, true, configureIcon, false);
    newAction(Options::tr("Playback settings(&P)"), this, playbackSettings, true, configureIcon, false);
    newAction(Options::tr("Modules settings(&M)"), this, modulesSettings, false, configureIcon, false);
    addSeparator();

    {
        QMenu *profiles = new QMenu(Options::tr("Profiles(&P)"), this);
        profiles->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(profiles, SIGNAL(customContextMenuRequested(QPoint)), parent, SLOT(removeProfileMenuRequest(QPoint)));
        profiles->setIcon(configureIcon);

        profiles->addAction(QMPlay2Core.getIconFromTheme("list-add"), Options::tr("New Profile(&N)"), parent, SLOT(addProfile()));
        profiles->addAction(QMPlay2Core.getIconFromTheme("edit-copy"), Options::tr("Copy Profile(&C)"), parent, SLOT(copyProfile()));
        profiles->addSeparator();

        profilesGroup = new QActionGroup(parent);

        QAction *act = profiles->addAction(Options::tr("Default(&D)"), parent, SLOT(changeProfile()));
        act->setProperty("path", "/");
        act->setCheckable(true);
        profilesGroup->addAction(act);

        for (const QString &profile : QDir(QMPlay2Core.getSettingsDir() + "Profiles/").entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        {
            QAction *act = profiles->addAction(profile, parent, SLOT(changeProfile()));
            act->setProperty("path", profile);
            act->setCheckable(true);
            profilesGroup->addAction(act);
        }

        const QString currentProfile = getCurrentProfile(QSettings(QMPlay2Core.getSettingsDir() + "Profile.ini", QSettings::IniFormat));
        for (QAction *act : profilesGroup->actions())
        {
            if (act->property("path").toString() == currentProfile)
            {
                act->setChecked(true);
                break;
            }
        }

        addMenu(profiles);
    }

    addSeparator();
    newAction(Options::tr("Show tray icon(&S)"), this, trayVisible, false, QIcon(), true);

    removeProfileMenu = new QMenu(this);
    removeProfileMenu->addAction(tr("Remove"), parent, SLOT(removeProfile()));
}

MenuBar::Help::Help(MenuBar *parent) :
    QMenu(Help::tr("Help(&H)"), parent)
{
    newAction(Help::tr("About QMPlay2(&A)"), this, about, false, QIcon(), false, QAction::AboutRole);
#ifdef UPDATER
    newAction(Help::tr("Updates(&U)"), this, updates, false, QIcon(), false);
#endif
    addSeparator();
    newAction(Help::tr("About Qt(&Q)"), this, aboutQt, false, QIcon(), false, QAction::AboutQtRole);
}

void MenuBar::setKeyShortcuts()
{
    ShortcutHandler *shortcuts = QMPlay2GUI.shortcutHandler;
    shortcuts->appendAction(window->toggleVisibility, "KeyBindings/Window-toggleVisibility", "`");
    shortcuts->appendAction(window->toggleFullScreen, "KeyBindings/Window-toggleFullScreen", "F");
    shortcuts->appendAction(window->toggleCompactView, "KeyBindings/Window-toggleCompactView", "Alt+V");
    shortcuts->appendAction(window->close, "KeyBindings/Window-close", "Alt+F4");
#ifdef Q_OS_MACOS
    window->toggleVisibility->setShortcuts({window->toggleVisibility->shortcut(), QKeySequence("Ctrl+W")});
#endif


    if (widgets->hideMenuAct)
        shortcuts->appendAction(widgets->hideMenuAct, "KeyBindings/Widgets-hideMenu", "Ctrl+Alt+M");
    shortcuts->appendAction(widgets->lockWidgetsAct, "KeyBindings/Widgets-lockWidgets", "Shift+L");


    shortcuts->appendAction(playlist->stopLoading, "KeyBindings/Playlist-stopLoading", "F4");
    shortcuts->appendAction(playlist->sync, "KeyBindings/Playlist-sync", "Shift+F5");
    shortcuts->appendAction(playlist->quickSync, "KeyBindings/Playlist-quickSync", "F5");
    shortcuts->appendAction(playlist->loadPlist, "KeyBindings/Playlist-loadPlist", "Ctrl+L");
    shortcuts->appendAction(playlist->savePlist, "KeyBindings/Playlist-savePlist", "Ctrl+S");
    shortcuts->appendAction(playlist->saveGroup, "KeyBindings/Playlist-saveGroup", "Ctrl+Shift+S");
    shortcuts->appendAction(playlist->delEntries, "KeyBindings/Playlist-delEntries", "Del");
    shortcuts->appendAction(playlist->delNonGroupEntries, "KeyBindings/Playlist-delNonGroupEntries", "Ctrl+Del");
    shortcuts->appendAction(playlist->clear, "KeyBindings/Playlist-clear", "Shift+Del");
    shortcuts->appendAction(playlist->copy, "KeyBindings/Playlist-copy", "Ctrl+C");
    shortcuts->appendAction(playlist->paste, "KeyBindings/Playlist-paste", "Ctrl+V");
    shortcuts->appendAction(playlist->newGroup, "KeyBindings/Playlist-newGroup", "F7");
    shortcuts->appendAction(playlist->renameGroup, "KeyBindings/Playlist-renameGroup", "F2");
    shortcuts->appendAction(playlist->find, "KeyBindings/Playlist-find", "Ctrl+F");
    shortcuts->appendAction(playlist->collapseAll, "KeyBindings/Playlist-collapseAll", "");
    shortcuts->appendAction(playlist->expandAll, "KeyBindings/Playlist-expandAll", "");
    shortcuts->appendAction(playlist->goToPlayback, "KeyBindings/Playlist-goToPlayback", "Ctrl+P");
    shortcuts->appendAction(playlist->queue, "KeyBindings/Playlist-queue", "Q");
    shortcuts->appendAction(playlist->skip, "KeyBindings/Playlist-skip", "S");
    shortcuts->appendAction(playlist->stopAfter, "KeyBindings/Playlist-stopAfter", "Alt+A");
    shortcuts->appendAction(playlist->lock, "KeyBindings/Playlist-lock", "Ctrl+Shift+L");
    shortcuts->appendAction(playlist->entryProperties, "KeyBindings/Playlist-entryProperties", "Alt+Return");

    shortcuts->appendAction(playlist->add->file, "KeyBindings/Playlist-Add-file", "Ctrl+I");
    shortcuts->appendAction(playlist->add->dir, "KeyBindings/Playlist-Add-dir", "Ctrl+D");
    shortcuts->appendAction(playlist->add->address, "KeyBindings/Playlist-Add-address", "Ctrl+U");

    shortcuts->appendAction(playlist->sort->timeSort1, "KeyBindings/Playlist-Sort-timeSort1", "");
    shortcuts->appendAction(playlist->sort->timeSort2, "KeyBindings/Playlist-Sort-timeSort2", "");
    shortcuts->appendAction(playlist->sort->titleSort1, "KeyBindings/Playlist-Sort-titleSort1", "");
    shortcuts->appendAction(playlist->sort->titleSort2, "KeyBindings/Playlist-Sort-titleSort2", "");


    shortcuts->appendAction(player->togglePlay, "KeyBindings/Player-togglePlay", "Space");
    shortcuts->appendAction(player->stop, "KeyBindings/Player-stop", "V");
    shortcuts->appendAction(player->next, "KeyBindings/Player-next", "B");
    shortcuts->appendAction(player->prev, "KeyBindings/Player-prev", "Z");
    shortcuts->appendAction(player->prevFrame, "KeyBindings/Player-prevFrame", ",");
    shortcuts->appendAction(player->nextFrame, "KeyBindings/Player-nextFrame", ".");
    shortcuts->appendAction(player->abRepeat, "KeyBindings/Player-abRepeat", "Ctrl+-");
    shortcuts->appendAction(player->seekF, "KeyBindings/Player-seekF", "Right");
    shortcuts->appendAction(player->seekB, "KeyBindings/Player-seekB", "Left");
    shortcuts->appendAction(player->lSeekF, "KeyBindings/Player-lSeekF", "Up");
    shortcuts->appendAction(player->lSeekB, "KeyBindings/Player-lSeekB", "Down");
    shortcuts->appendAction(player->speedUp, "KeyBindings/Player-speedUp", "]");
    shortcuts->appendAction(player->slowDown, "KeyBindings/Player-slowDown", "[");
    shortcuts->appendAction(player->setSpeed, "KeyBindings/Player-setSpeed", "Shift+S");
    shortcuts->appendAction(player->zoomIn, "KeyBindings/Player-zoomIn", "E");
    shortcuts->appendAction(player->zoomOut, "KeyBindings/Player-zoomOut", "W");
    shortcuts->appendAction(player->switchARatio, "KeyBindings/Player-switchARatio", "A");
    shortcuts->appendAction(player->reset, "KeyBindings/Player-reset", "R");
    shortcuts->appendAction(player->volUp, "KeyBindings/Player-volUp", "*");
    shortcuts->appendAction(player->volDown, "KeyBindings/Player-volDown", "/");
    shortcuts->appendAction(player->toggleMute, "KeyBindings/Player-toggleMute", "M");
    if (player->detach)
        shortcuts->appendAction(player->detach, "KeyBindings/Player-detach", "");
    if (player->suspend)
        shortcuts->appendAction(player->suspend, "KeyBindings/Player-suspend", "");

    shortcuts->appendAction(player->repeat->repeatActions[RepeatNormal], "KeyBindings/Player-Repeat-RepeatNormal", "Alt+0");
    shortcuts->appendAction(player->repeat->repeatActions[RepeatEntry], "KeyBindings/Player-Repeat-RepeatEntry", "Alt+1");
    shortcuts->appendAction(player->repeat->repeatActions[RepeatGroup], "KeyBindings/Player-Repeat-RepeatGroup", "Alt+2");
    shortcuts->appendAction(player->repeat->repeatActions[RepeatList], "KeyBindings/Player-Repeat-RepeatList", "Alt+3");
    shortcuts->appendAction(player->repeat->repeatActions[RandomMode], "KeyBindings/Player-Repeat-RandomMode", "Alt+4");
    shortcuts->appendAction(player->repeat->repeatActions[RandomGroupMode], "KeyBindings/Player-Repeat-RandomGroupMode", "Alt+5");
    shortcuts->appendAction(player->repeat->repeatActions[RepeatRandom], "KeyBindings/Player-Repeat-RepeatRandom", "Alt+6");
    shortcuts->appendAction(player->repeat->repeatActions[RepeatRandomGroup], "KeyBindings/Player-Repeat-RepeatRandomGroup", "Alt+7");
    shortcuts->appendAction(player->repeat->repeatActions[RepeatStopAfter], "KeyBindings/Player-Repeat-RepeatStopAfter", "Alt+8");


    shortcuts->appendAction(playback->toggleAudio, "KeyBindings/Playback-toggleAudio", "D");
    shortcuts->appendAction(playback->toggleVideo, "KeyBindings/Playback-toggleVideo", "O");
    shortcuts->appendAction(playback->videoSync, "KeyBindings/Playback-videoSync", "Shift+O");
    shortcuts->appendAction(playback->slowDownVideo, "KeyBindings/Playback-slowDownVideo", "-");
    shortcuts->appendAction(playback->speedUpVideo, "KeyBindings/Playback-speedUpVideo", "+");
    shortcuts->appendAction(playback->toggleSubtitles, "KeyBindings/Playback-toggleSubtitles", "N");
    shortcuts->appendAction(playback->subsFromFile, "KeyBindings/Playback-subsFromFile", "Alt+I");
    shortcuts->appendAction(playback->subtitlesSync, "KeyBindings/Playback-subtitlesSync", "Shift+N");
    shortcuts->appendAction(playback->slowDownSubtitles, "KeyBindings/Playback-slowDownSubtitles", "Shift+Z");
    shortcuts->appendAction(playback->speedUpSubtitles, "KeyBindings/Playback-speedUpSubtitles", "Shift+X");
    shortcuts->appendAction(playback->biggerSubtitles, "KeyBindings/Playback-biggerSubtitles", "Shift+R");
    shortcuts->appendAction(playback->smallerSubtitles, "KeyBindings/Playback-smallerSubtitles", "Shift+T");
    shortcuts->appendAction(playback->screenShot, "KeyBindings/Playback-screenShot", "Alt+S");

    shortcuts->appendAction(playback->videoFilters->spherical, "KeyBindings/Playback-VideoFilters-spherical", "Ctrl+3");
    shortcuts->appendAction(playback->videoFilters->hFlip, "KeyBindings/Playback-VideoFilters-hFlip", "Ctrl+M");
    shortcuts->appendAction(playback->videoFilters->vFlip, "KeyBindings/Playback-VideoFilters-vFlip", "Ctrl+R");
    shortcuts->appendAction(playback->videoFilters->rotate90, "KeyBindings/Playback-VideoFilters-rotate90", "Ctrl+9");
    shortcuts->appendAction(playback->videoFilters->more, "KeyBindings/Playback-VideoFilters-more", "Alt+F");


    shortcuts->appendAction(options->settings, "KeyBindings/Options-settings", "Ctrl+O");
    shortcuts->appendAction(options->rendererSettings, "KeyBindings/Options-rendererSettings", "Ctrl+Shift+R");
    shortcuts->appendAction(options->playbackSettings, "KeyBindings/Options-playbackSettings", "Ctrl+Shift+P");
    shortcuts->appendAction(options->modulesSettings, "KeyBindings/Options-modulesSettings", "Ctrl+Shift+O");
    shortcuts->appendAction(options->trayVisible, "KeyBindings/Options-trayVisible", "Ctrl+T");


    shortcuts->appendAction(help->about, "KeyBindings/Help-about", "F1");
#ifdef UPDATER
    shortcuts->appendAction(help->updates, "KeyBindings/Help-updates", "F12");
#endif
}

void MenuBar::changeProfile()
{
    QAction *act = (QAction *)sender();
    const QString selectedProfile = act->property("path").toString();
    QSettings profileSettings(QMPlay2Core.getSettingsDir() + "Profile.ini", QSettings::IniFormat);
    if (selectedProfile != getCurrentProfile(profileSettings))
    {
        profileSettings.setValue("Profile", selectedProfile);
        QMPlay2GUI.noAutoPlay = true;
        restartApp();
    }
}
void MenuBar::addProfile()
{
    createAndSetProfile(this);
}
void MenuBar::copyProfile()
{
    QMPlay2GUI.newProfileName = createAndSetProfile(this);
}
void MenuBar::removeProfileMenuRequest(const QPoint &p)
{
    QMenu *menu = qobject_cast<QMenu *>(sender());
    QAction *act = menu->actionAt(p);
    if (act && menu->actions().indexOf(act) >= 4)
    {
        options->removeProfileMenu->setProperty("profile", QVariant::fromValue((void *)act));
        options->removeProfileMenu->popup(menu->mapToGlobal(p));
    }
}
void MenuBar::removeProfile()
{
    if (QAction *act = (QAction *)options->removeProfileMenu->property("profile").value<void *>())
    {
        const QString &profile = "Profiles/" + act->property("path").toString() + "/";
        if (profile == QMPlay2Core.getSettingsProfile())
        {
            QSettings profileSettings(QMPlay2Core.getSettingsDir() + "Profile.ini", QSettings::IniFormat);
            profileSettings.setValue("Profile", "/");
            QMPlay2GUI.removeSettings = QMPlay2GUI.noAutoPlay = true;
            restartApp();
        }
        else
        {
            const QString settingsDir = QMPlay2Core.getSettingsDir() + profile;
            for (const QString &fName : QDir(settingsDir).entryList({"*.ini"}))
                QFile::remove(settingsDir + fName);
            QDir(QMPlay2Core.getSettingsDir()).rmdir(profile);
            delete act;
        }
    }
}

void MenuBar::widgetsMenuShow()
{
    widgets->menuShow();
}
