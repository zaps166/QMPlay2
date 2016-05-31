#include <MenuBar.hpp>

#include <VideoEqualizer.hpp>
#include <DockWidget.hpp>
#include <Main.hpp>

#include <QWidgetAction>
#include <QMainWindow>

static QAction *newAction(const QString &txt, QMenu *parent, const QKeySequence &keySequence, QAction *&act, bool autoRepeat, const QIcon &icon, bool checkable)
{
	act = new QAction(txt, parent);
	act->setAutoRepeat(autoRepeat);
	act->setShortcut(keySequence);
	act->setCheckable(checkable);
	parent->addAction(act);
	act->setIcon(icon);
	return act;
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
#ifdef Q_OS_MAC
	widgets->addAction(QString()); //Mac must have got at least one item inside menu, otherwise the menu is not shown (QTBUG?)
#endif
}

MenuBar::Window::Window(MenuBar *parent) :
	QMenu(Window::tr("&Window"), parent)
{
	newAction(QString(), this, QKeySequence("`"), toggleVisibility, false, QIcon(), false);
	newAction(Window::tr("&Full screen"), this, QKeySequence("F"), toggleFullScreen, false, QMPlay2Core.getIconFromTheme("view-fullscreen"), false);
	newAction(Window::tr("&Compact view"), this, QKeySequence("Alt+V"), toggleCompactView, false, QIcon(), true );
	addSeparator();
	newAction(Window::tr("&Close"), this, QKeySequence("Alt+F4"), close, false, QMPlay2Core.getIconFromTheme("application-exit"), false);
}

MenuBar::Widgets::Widgets(MenuBar *parent) :
	QMenu(Widgets::tr("&Widgets"), parent)
{}
void MenuBar::Widgets::menuShow()
{
	clear();
	QMenu *menu = qobject_cast<QMainWindow *>(QMPlay2GUI.mainW)->createPopupMenu();
	if (menu)
	{
		foreach (QAction *act, menu->actions())
		{
			if (act->parent() == menu)
				act->setParent(this);
			addAction(act);
		}
		menu->deleteLater();
	}
}

MenuBar::Playlist::Playlist(MenuBar *parent) :
	QMenu(Playlist::tr("&Playlist"), parent)
{
	add = new Add(this);
	addMenu(add);

	addSeparator();
	newAction(Playlist::tr("&Stop loading"), this, QKeySequence("F4"), stopLoading, false, QMPlay2Core.getIconFromTheme("process-stop"), false);
	addSeparator();
	newAction(Playlist::tr("&Sync directory"), this, QKeySequence("F5"), sync, false, QMPlay2Core.getIconFromTheme("view-refresh"), false);
	addSeparator();
	newAction(Playlist::tr("Load &list"), this, QKeySequence("Ctrl+L"), loadPlist, false, QIcon(), false);
	newAction(Playlist::tr("Save &list"), this, QKeySequence("Ctrl+S"), savePlist, false, QIcon(), false);
	newAction(Playlist::tr("Save &group"), this, QKeySequence("Ctrl+Shift+S"), saveGroup, false, QIcon(), false);
	addSeparator();
	newAction(Playlist::tr("&Remove selected entries"), this, QKeySequence("Del"), delEntries, true, QMPlay2Core.getIconFromTheme("list-remove"), false);
	newAction(Playlist::tr("Remove entries &without groups"), this, QKeySequence("Ctrl+Del"), delNonGroupEntries, false, QMPlay2Core.getIconFromTheme("list-remove"), false);
	newAction(Playlist::tr("&Clear list"), this, QKeySequence("Shift+Del"), clear, false, QMPlay2Core.getIconFromTheme("archive-remove"), false);
	addSeparator();
	newAction(Playlist::tr("&Copy"), this, QKeySequence("Ctrl+C"), copy, false, QMPlay2Core.getIconFromTheme("edit-copy"), false);
	newAction(Playlist::tr("&Paste"), this, QKeySequence("Ctrl+V"), paste, false, QMPlay2Core.getIconFromTheme("edit-paste"), false);
	addSeparator();

	extensions = new QMenu(Playlist::tr("&Extensions"), this);
	extensions->setEnabled(false);
	addMenu(extensions);

	addSeparator();
	newAction(Playlist::tr("&Create &group"), this, QKeySequence("F7"), newGroup, false, QMPlay2Core.getIconFromTheme("folder-new"), false);
	newAction(Playlist::tr("&Rename"), this, QKeySequence("F2"), renameGroup, false, QIcon(), false);
	addSeparator();
	newAction(Playlist::tr("&Find entries"), this, QKeySequence("Ctrl+F"), find, false, QMPlay2Core.getIconFromTheme("edit-find"), false);
	addSeparator();

	sort = new Sort(this);
	addMenu(sort);

	addSeparator();
	newAction(Playlist::tr("&Collapse all"), this, QKeySequence(), collapseAll, false, QIcon(), false);
	newAction(Playlist::tr("&Expand all"), this, QKeySequence(), expandAll, false, QIcon(), false);
	addSeparator();
	newAction(Playlist::tr("&Go to the playback"), this, QKeySequence("Ctrl+P"), goToPlayback, false, QIcon(), false);
	addSeparator();
	newAction(Playlist::tr("&Enqueue"), this, QKeySequence("Q"), queue, false, QIcon(), false);
	addSeparator();
	newAction(Playlist::tr("&Properties"), this, QKeySequence("Alt+Return"), entryProperties, false, QMPlay2Core.getIconFromTheme("document-properties"), false);
}
MenuBar::Playlist::Add::Add(QMenu *parent) :
	QMenu(Add::tr("&Add"), parent)
{
	setIcon(QMPlay2Core.getIconFromTheme("list-add"));
	newAction(Add::tr("&Files"), this, QKeySequence("Ctrl+I"), file, false, QMPlay2Core.getIconFromTheme("applications-multimedia"), false);
	newAction(Add::tr("&Directory"), this, QKeySequence("Ctrl+D"), dir, false, QMPlay2Core.getIconFromTheme("folder"), false);
	newAction(Add::tr("&Address"), this, QKeySequence("Ctrl+U"), address, false, QMPlay2Core.getIconFromTheme("application-x-mswinurl"), false);
	addSeparator();
}
MenuBar::Playlist::Sort::Sort(QMenu *parent) :
	QMenu(Sort::tr("&Sort"), parent)
{
	newAction(Sort::tr("&From the shortest to the longest"), this, QKeySequence(), timeSort1, false, QIcon(), false);
	newAction(Sort::tr("&From the longest to the shortest"), this, QKeySequence(), timeSort2, false, QIcon(), false);
	addSeparator();
	newAction(Sort::tr("&A-Z"), this, QKeySequence(), titleSort1, false, QIcon(), false);
	newAction(Sort::tr("&Z-A"), this, QKeySequence(), titleSort2, false, QIcon(), false);
}

MenuBar::Player::Player(MenuBar *parent) :
	QMenu(Player::tr("&Player"), parent)
{
	newAction(QString(), this, QKeySequence("Space"), togglePlay, false, QMPlay2Core.getIconFromTheme("media-playback-start"), false);
	newAction(Player::tr("&Stop"), this, QKeySequence("V"), stop, false, QMPlay2Core.getIconFromTheme("media-playback-stop"), false);
	newAction(Player::tr("&Next"), this, QKeySequence("B"), next, true, QMPlay2Core.getIconFromTheme("media-skip-forward"), false);
	newAction(Player::tr("&Previous"), this, QKeySequence("Z"), prev, true, QMPlay2Core.getIconFromTheme("media-skip-backward"), false);
	newAction(Player::tr("Next &frame"), this, QKeySequence("."), nextFrame, true, QIcon(), false);
	addSeparator();

	repeat = new Repeat(this);
	addMenu(repeat);

	newAction(Player::tr("A&-B Repeat"), this, QKeySequence("Ctrl+-"), abRepeat, true, QIcon(), false);

	addSeparator();
	newAction(Player::tr("Seek &forward"), this, QKeySequence("Right"), seekF, true, QIcon(), false);
	newAction(Player::tr("seek &backward"), this, QKeySequence("Left"), seekB, true, QIcon(), false);
	newAction(Player::tr("Long &seek &forward"), this, QKeySequence("Up"), lSeekF, true, QIcon(), false);
	newAction(Player::tr("Long s&eek backward"), this, QKeySequence("Down"), lSeekB, true, QIcon(), false);
	addSeparator();
	newAction(Player::tr("Fa&ster"), this, QKeySequence("]"), speedUp, true, QIcon(), false);
	newAction(Player::tr("Slowe&r"), this, QKeySequence("["), slowDown, true, QIcon(), false);
	newAction(Player::tr("&Set speed"), this, QKeySequence("Shift+S"), setSpeed, false, QIcon(), false);
	addSeparator();
	newAction(Player::tr("Zoom i&n"), this, QKeySequence("E"), zoomIn, true, QIcon(), false);
	newAction(Player::tr("Zoom ou&t"), this, QKeySequence("W"), zoomOut, true, QIcon(), false);
	newAction(Player::tr("Toggle &aspect ratio"), this, QKeySequence("A"), switchARatio, true, QIcon(), false);

	aRatio = new AspectRatio(this);
	addMenu(aRatio);

	newAction(Player::tr("Reset image &settings"), this, QKeySequence("R"), reset, false, QIcon(), false);

	addSeparator();
	newAction(Player::tr("Volume &up"), this, QKeySequence("*"), volUp, true, QIcon(), false);
	newAction(Player::tr("Volume &down"), this, QKeySequence("/"), volDown, true, QIcon(), false);
	newAction(Player::tr("&Mute"), this, QKeySequence("M"), toggleMute, false, QMPlay2Core.getIconFromTheme("audio-volume-high"), true);

	if (!QMPlay2GUI.pipe)
		detach = NULL;
	else
	{
		addSeparator();
		newAction(Player::tr("Detach from receiving &commands"), this, QKeySequence(), detach, false, QIcon(), false);
	}

	if (!QMPlay2CoreClass::canSuspend())
		suspend = NULL;
	else
	{
		addSeparator();
		newAction(Player::tr("Suspend after playbac&k is finished"), this, QKeySequence(), suspend, false, QIcon(), true);
	}
}
MenuBar::Player::Repeat::Repeat(QMenu *parent) :
	QMenu(Repeat::tr("&Repeat"), parent)
{
	choice = new QActionGroup(this);
	choice->addAction(newAction(Repeat::tr("&No repeating"), this, QKeySequence("Alt+0"), repeatActions[RepeatNormal], false, QIcon(), true));
	addSeparator();
	choice->addAction(newAction(Repeat::tr("&Entry repeating"), this, QKeySequence("Alt+1"), repeatActions[RepeatEntry], false, QIcon(), true));
	choice->addAction(newAction(Repeat::tr("&Group repeating"), this, QKeySequence("Alt+2"), repeatActions[RepeatGroup], false, QIcon(), true));
	choice->addAction(newAction(Repeat::tr("&Playlist repeating"), this, QKeySequence("Alt+3"), repeatActions[RepeatList], false, QIcon(), true));
	addSeparator();
	choice->addAction(newAction(Repeat::tr("R&andom"), this, QKeySequence("Alt+4"), repeatActions[RandomMode], false, QIcon(), true));
	choice->addAction(newAction(Repeat::tr("Random in &group"), this, QKeySequence("Alt+5"), repeatActions[RandomGroupMode], false, QIcon(), true));
	addSeparator();
	choice->addAction(newAction(Repeat::tr("Random and &repeat"), this, QKeySequence("Alt+6"), repeatActions[RepeatRandom], false, QIcon(), true));
	choice->addAction(newAction(Repeat::tr("Random in group and repea&t"), this, QKeySequence("Alt+7"), repeatActions[RepeatRandomGroup], false, QIcon(), true));

	for (int i = 0; i < RepeatModeCount; ++i)
		repeatActions[i]->setProperty("enumValue", i);
}
MenuBar::Player::AspectRatio::AspectRatio(QMenu *parent) :
	QMenu(AspectRatio::tr("&Aspect ratio"), parent)
{
	choice = new QActionGroup(this);
	choice->addAction(newAction(AspectRatio::tr("&Auto"), this, QKeySequence(), _auto, false, QIcon(), true));
	addSeparator();
	choice->addAction(newAction("&1:1", this, QKeySequence(), _1x1, false, QIcon(), true));
	choice->addAction(newAction("&4:3", this, QKeySequence(), _4x3, false, QIcon(), true));
	choice->addAction(newAction("&5:4", this, QKeySequence(), _5x4, false, QIcon(), true));
	choice->addAction(newAction("&16:9", this, QKeySequence(), _16x9, false, QIcon(), true));
	choice->addAction(newAction("&3:2", this, QKeySequence(), _3x2, false, QIcon(), true));
	choice->addAction(newAction("&21:9", this, QKeySequence(), _21x9, false, QIcon(), true));
	addSeparator();
	choice->addAction(newAction(AspectRatio::tr("D&epends on size"), this, QKeySequence(), sizeDep, false, QIcon(), true));
	choice->addAction(newAction(AspectRatio::tr("&Disabled"), this, QKeySequence(), off, false, QIcon(), true));

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
	Qt::ShortcutContext ctx = e ? Qt::WindowShortcut : Qt::WidgetShortcut;
	seekF->setShortcutContext(ctx);
	seekB->setShortcutContext(ctx);
	lSeekF->setShortcutContext(ctx);
	lSeekB->setShortcutContext(ctx);
}

MenuBar::Playback::Playback(MenuBar *parent) :
	QMenu(Playback::tr("&Playback"), parent)
{
	newAction(Playback::tr("&Audio enabled"), this, QKeySequence("D"), toggleAudio, false, QIcon(), true)->setObjectName("toggleAudio");
	toggleAudio->setChecked(true);

	audioChannels = new AudioChannels(this);
	addMenu(audioChannels);

	addSeparator();

	newAction(Playback::tr("&Video enabled"), this, QKeySequence("O"), toggleVideo, false, QIcon(), true)->setObjectName("toggleVideo");
	toggleVideo->setChecked(true);

	videoFilters = new VideoFilters(this);
	addMenu(videoFilters);

	newAction(Playback::tr("Set &video delay"), this, QKeySequence("Shift+O"), videoSync, false, QIcon(), false);
	newAction(Playback::tr("&Delay video") + " (100ms)", this, QKeySequence("-"), slowDownVideo, true, QIcon(), false);
	newAction(Playback::tr("&Speed up video") + " (100ms)", this, QKeySequence("+"), speedUpVideo, true, QIcon(), false);
	addSeparator();
	newAction(Playback::tr("&Subtitles enabled"), this, QKeySequence("N"), toggleSubtitles, false, QIcon(), true)->setObjectName("toggleSubtitles");
	toggleSubtitles->setChecked(true);
	newAction(Playback::tr("Add &subtities from file"), this, QKeySequence("Alt+I"), subsFromFile, false, QIcon(), false);
	newAction(Playback::tr("Set &subtitles delay"), this, QKeySequence("Shift+N"), subtitlesSync, true, QIcon(), false);
	newAction(Playback::tr("&Delay subtitiles") + " (100ms)", this, QKeySequence("Shift+Z"), slowDownSubtitles, true, QIcon(), false);
	newAction(Playback::tr("&Speed up subtitles") + " (100ms)", this, QKeySequence("Shift+X"), speedUpSubtitles, true, QIcon(), false);
	newAction(Playback::tr("Scale up subt&itles"), this, QKeySequence("Shift+R"), biggerSubtitles, true, QIcon(), false);
	newAction(Playback::tr("Scale down sub&titles"), this, QKeySequence("Shift+T"), smallerSubtitles, true, QIcon(), false);
	addSeparator();
	newAction(Playback::tr("&Playback settings"), this, QKeySequence("Ctrl+Shift+P"), playbackSettings, true, QMPlay2Core.getIconFromTheme("configure"), false);
	addSeparator();
	newAction(Playback::tr("&Screen shot"), this, QKeySequence("Alt+S"), screenShot, true, QIcon(), false);
}
MenuBar::Playback::VideoFilters::VideoFilters(QMenu *parent) :
	QMenu(VideoFilters::tr("Video &filters"), parent)
{
	/** Korektor wideo */
	QMenu *videoEqualizerMenu = new QMenu(VideoFilters::tr("Video &equalizer"), this);
	addMenu(videoEqualizerMenu);
	QWidgetAction *widgetAction = new QWidgetAction(this);
	widgetAction->setDefaultWidget(videoEqualizer = new VideoEqualizer);
	videoEqualizerMenu->addAction(widgetAction);
	/**/
	addSeparator();
	newAction(VideoFilters::tr("&Spherical view"), this, QKeySequence("Ctrl+3"), spherical, true, QIcon(), true);
	addSeparator();
	newAction(VideoFilters::tr("&Horizontal flip"), this, QKeySequence("Ctrl+M"), hFlip, true, QIcon(), true);
	newAction(VideoFilters::tr("&Vertical flip"), this, QKeySequence("Ctrl+R"), vFlip, true, QIcon(), true);
	newAction(VideoFilters::tr("&Rotate 90°"), this, QKeySequence("Ctrl+9"), rotate90, true, QIcon(), true);
	addSeparator();
	newAction(VideoFilters::tr("&More filters"), this, QKeySequence("Alt+F"), more, false, QIcon(), false);
}
MenuBar::Playback::AudioChannels::AudioChannels(QMenu *parent) :
	QMenu(AudioChannels::tr("&Channels") , parent)
{
	choice = new QActionGroup(this);
	choice->addAction(newAction(AudioChannels::tr("&Autodetect"), this, QKeySequence(), _auto, false, QIcon(), true));
	addSeparator();
	choice->addAction(newAction(AudioChannels::tr("&Mono"), this, QKeySequence(), _1, false, QIcon(), true));
	choice->addAction(newAction(AudioChannels::tr("&Stereo"), this, QKeySequence(), _2, false, QIcon(), true));
	choice->addAction(newAction("&4.0", this, QKeySequence(), _4, false, QIcon(), true));
	choice->addAction(newAction("&5.1", this, QKeySequence(), _6, false, QIcon(), true));
	choice->addAction(newAction("&7.1", this, QKeySequence(), _8, false, QIcon(), true));
	addSeparator();
	choice->addAction(newAction(AudioChannels::tr("&Other"), this, QKeySequence(), other, false, QIcon(), true));

	_auto->setObjectName("auto");
	_1->setObjectName("1");
	_2->setObjectName("2");
	_4->setObjectName("4");
	_6->setObjectName("6");
	_8->setObjectName("8");
	other->setEnabled(false);
}

MenuBar::Options::Options(MenuBar *parent) :
	QMenu(Options::tr("Op&tions"), parent)
{
	newAction(Options::tr("&Settings"), this, QKeySequence("Ctrl+O"), settings, false, QMPlay2Core.getIconFromTheme("configure"), false);
	newAction(Options::tr("&Modules settings"), this, QKeySequence("Ctrl+Shift+O"), modulesSettings, false, QMPlay2Core.getIconFromTheme("configure"), false);
	addSeparator();
	newAction(Options::tr("&Show tray icon"), this, QKeySequence("Ctrl+T"), trayVisible, false, QIcon(), true);
}

MenuBar::Help::Help(MenuBar *parent) :
	QMenu(Help::tr("&Help"), parent)
{
	newAction(Help::tr("&About QMPlay2"), this, QKeySequence("F1"), about, false, QIcon(), false);
#ifdef UPDATER
	newAction(Help::tr("&Updates"), this, QKeySequence("F12"), updates, false, QIcon(), false);
#endif
	addSeparator();
	newAction(Help::tr("About &Qt"), this, QKeySequence(), aboutQt, false, QIcon(), false);
}

void MenuBar::widgetsMenuShow()
{
	widgets->menuShow();
}
