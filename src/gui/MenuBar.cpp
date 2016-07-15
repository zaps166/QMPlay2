/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

#include <VideoAdjustment.hpp>
#include <DockWidget.hpp>
#include <Settings.hpp>
#include <Main.hpp>

#include <QWidgetAction>
#include <QMainWindow>

static QAction *newAction(const QString &txt, QMenu *parent, QAction *&act, bool autoRepeat, const QIcon &icon, bool checkable)
{
	act = new QAction(txt, parent);
	act->setAutoRepeat(autoRepeat);
	act->setCheckable(checkable);
	parent->addAction(act);
	act->setIcon(icon);
	return act;
}

/**/

void MenuBar::init()
{
	Settings &settings = QMPlay2Core.getSettings();

	settings.init("KeyBindings/Window-toggleVisibility", "`");
	settings.init("KeyBindings/Window-toggleFullScreen", "F");
	settings.init("KeyBindings/Window-toggleCompactView", "Alt+V");
	settings.init("KeyBindings/Window-close", "Alt+F4");


	settings.init("KeyBindings/Widgets-hideMenu", "Alt+Ctrl+M");
	settings.init("KeyBindings/Widgets-lockWidgets", "Shift+L");


	settings.init("KeyBindings/Playlist-stopLoading", "F4");
	settings.init("KeyBindings/Playlist-sync", "F5");
	settings.init("KeyBindings/Playlist-loadPlist", "Ctrl+L");
	settings.init("KeyBindings/Playlist-savePlist", "Ctrl+S");
	settings.init("KeyBindings/Playlist-saveGroup", "Ctrl+Shift+S");
	settings.init("KeyBindings/Playlist-delEntries", "Del");
	settings.init("KeyBindings/Playlist-delNonGroupEntries", "Ctrl+Del");
	settings.init("KeyBindings/Playlist-clear", "Shift+Del");
	settings.init("KeyBindings/Playlist-copy", "Ctrl+C");
	settings.init("KeyBindings/Playlist-paste", "Ctrl+V");
	settings.init("KeyBindings/Playlist-newGroup", "F7");
	settings.init("KeyBindings/Playlist-renameGroup", "F2");
	settings.init("KeyBindings/Playlist-find", "Ctrl+F");
	settings.init("KeyBindings/Playlist-collapseAll", "");
	settings.init("KeyBindings/Playlist-expandAll", "");
	settings.init("KeyBindings/Playlist-goToPlayback", "Ctrl+P");
	settings.init("KeyBindings/Playlist-queue", "Q");
	settings.init("KeyBindings/Playlist-entryProperties", "Alt+Return");

	settings.init("KeyBindings/Playlist-Add-file", "Ctrl+I");
	settings.init("KeyBindings/Playlist-Add-dir", "Ctrl+D");
	settings.init("KeyBindings/Playlist-Add-address", "Ctrl+U");

	settings.init("KeyBindings/Playlist-Sort-timeSort1", "");
	settings.init("KeyBindings/Playlist-Sort-timeSort2", "");
	settings.init("KeyBindings/Playlist-Sort-titleSort1", "");
	settings.init("KeyBindings/Playlist-Sort-titleSort2", "");


	settings.init("KeyBindings/Player-togglePlay", "Space");
	settings.init("KeyBindings/Player-stop", "V");
	settings.init("KeyBindings/Player-next", "B");
	settings.init("KeyBindings/Player-prev", "Z");
	settings.init("KeyBindings/Player-nextFrame", ".");
	settings.init("KeyBindings/Player-abRepeat", "Ctrl+-");
	settings.init("KeyBindings/Player-seekF", "Right");
	settings.init("KeyBindings/Player-seekB", "Left");
	settings.init("KeyBindings/Player-lSeekF", "Up");
	settings.init("KeyBindings/Player-lSeekB", "Down");
	settings.init("KeyBindings/Player-speedUp", "]");
	settings.init("KeyBindings/Player-slowDown", "[");
	settings.init("KeyBindings/Player-setSpeed", "Shift+S");
	settings.init("KeyBindings/Player-zoomIn", "E");
	settings.init("KeyBindings/Player-zoomOut", "W");
	settings.init("KeyBindings/Player-switchARatio", "A");
	settings.init("KeyBindings/Player-reset", "R");
	settings.init("KeyBindings/Player-volUp", "*");
	settings.init("KeyBindings/Player-volDown", "/");
	settings.init("KeyBindings/Player-toggleMute", "M");
	settings.init("KeyBindings/Player-detach", "");
	settings.init("KeyBindings/Player-suspend", "");

	settings.init("KeyBindings/Player-Repeat-RepeatNormal", "Alt+0");
	settings.init("KeyBindings/Player-Repeat-RepeatEntry", "Alt+1");
	settings.init("KeyBindings/Player-Repeat-RepeatGroup", "Alt+2");
	settings.init("KeyBindings/Player-Repeat-RepeatList", "Alt+3");
	settings.init("KeyBindings/Player-Repeat-RandomMode", "Alt+4");
	settings.init("KeyBindings/Player-Repeat-RandomGroupMode", "Alt+5");
	settings.init("KeyBindings/Player-Repeat-RepeatRandom", "Alt+6");
	settings.init("KeyBindings/Player-Repeat-RepeatRandomGroup", "Alt+7");


	settings.init("KeyBindings/Playback-toggleAudio", "D");
	settings.init("KeyBindings/Playback-toggleVideo", "O");
	settings.init("KeyBindings/Playback-videoSync", "Shift+O");
	settings.init("KeyBindings/Playback-slowDownVideo", "-");
	settings.init("KeyBindings/Playback-speedUpVideo", "+");
	settings.init("KeyBindings/Playback-toggleSubtitles", "N");
	settings.init("KeyBindings/Playback-subsFromFile", "Alt+I");
	settings.init("KeyBindings/Playback-subtitlesSync", "Shift+N");
	settings.init("KeyBindings/Playback-slowDownSubtitles", "Shift+Z");
	settings.init("KeyBindings/Playback-speedUpSubtitles", "Shift+X");
	settings.init("KeyBindings/Playback-biggerSubtitles", "Shift+R");
	settings.init("KeyBindings/Playback-smallerSubtitles", "Shift+T");
	settings.init("KeyBindings/Playback-playbackSettings", "Ctrl+Shift+P");
	settings.init("KeyBindings/Playback-screenShot", "Alt+S");

	settings.init("KeyBindings/Playback-VideoFilters-spherical", "Ctrl+3");
	settings.init("KeyBindings/Playback-VideoFilters-hFlip", "Ctrl+M");
	settings.init("KeyBindings/Playback-VideoFilters-vFlip", "Ctrl+R");
	settings.init("KeyBindings/Playback-VideoFilters-rotate90", "Ctrl+9");
	settings.init("KeyBindings/Playback-VideoFilters-more", "Alt+F");


	settings.init("KeyBindings/Options-settings", "Ctrl+O");
	settings.init("KeyBindings/Options-modulesSettings", "Ctrl+Shift+O");
	settings.init("KeyBindings/Options-trayVisible", "Ctrl+T");


	settings.init("KeyBindings/Help-about", "F1");
#ifdef UPDATER
	settings.init("KeyBindings/Help-updates", "F12");
#endif
}

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
	setKeyShourtcuts();
}

MenuBar::Window::Window(MenuBar *parent) :
	QMenu(Window::tr("&Window"), parent)
{
	newAction(QString(), this, toggleVisibility, false, QIcon(), false);
	newAction(Window::tr("&Full screen"), this, toggleFullScreen, false, QMPlay2Core.getIconFromTheme("view-fullscreen"), false);
	newAction(Window::tr("&Compact view"), this, toggleCompactView, false, QIcon(), true );
	addSeparator();
	newAction(Window::tr("&Close"), this, close, false, QMPlay2Core.getIconFromTheme("application-exit"), false);
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
	newAction(Playlist::tr("&Stop loading"), this, stopLoading, false, QMPlay2Core.getIconFromTheme("process-stop"), false);
	addSeparator();
	newAction(Playlist::tr("&Synchronize group"), this, sync, false, QMPlay2Core.getIconFromTheme("view-refresh"), false);
	addSeparator();
	newAction(Playlist::tr("Load &list"), this, loadPlist, false, QIcon(), false);
	newAction(Playlist::tr("Save &list"), this, savePlist, false, QIcon(), false);
	newAction(Playlist::tr("Save &group"), this, saveGroup, false, QIcon(), false);
	addSeparator();
	newAction(Playlist::tr("&Remove selected entries"), this, delEntries, true, QMPlay2Core.getIconFromTheme("list-remove"), false);
	newAction(Playlist::tr("Remove entries &without groups"), this, delNonGroupEntries, false, QMPlay2Core.getIconFromTheme("list-remove"), false);
	newAction(Playlist::tr("&Clear list"), this, clear, false, QMPlay2Core.getIconFromTheme("archive-remove"), false);
	addSeparator();
	newAction(Playlist::tr("&Copy"), this, copy, false, QMPlay2Core.getIconFromTheme("edit-copy"), false);
	newAction(Playlist::tr("&Paste"), this, paste, false, QMPlay2Core.getIconFromTheme("edit-paste"), false);
	addSeparator();

	extensions = new QMenu(Playlist::tr("&Extensions"), this);
	extensions->setEnabled(false);
	addMenu(extensions);

	addSeparator();
	newAction(Playlist::tr("&Create &group"), this, newGroup, false, QMPlay2Core.getIconFromTheme("folder-new"), false);
	newAction(Playlist::tr("&Rename"), this, renameGroup, false, QIcon(), false);
	addSeparator();
	newAction(Playlist::tr("&Find entries"), this, find, false, QMPlay2Core.getIconFromTheme("edit-find"), false);
	addSeparator();

	sort = new Sort(this);
	addMenu(sort);

	addSeparator();
	newAction(Playlist::tr("&Collapse all"), this, collapseAll, false, QIcon(), false);
	newAction(Playlist::tr("&Expand all"), this, expandAll, false, QIcon(), false);
	addSeparator();
	newAction(Playlist::tr("&Go to the playback"), this, goToPlayback, false, QIcon(), false);
	addSeparator();
	newAction(Playlist::tr("&Enqueue"), this, queue, false, QIcon(), false);
	addSeparator();
	newAction(Playlist::tr("&Properties"), this, entryProperties, false, QMPlay2Core.getIconFromTheme("document-properties"), false);
}
MenuBar::Playlist::Add::Add(QMenu *parent) :
	QMenu(Add::tr("&Add"), parent)
{
	setIcon(QMPlay2Core.getIconFromTheme("list-add"));
	newAction(Add::tr("&Files"), this, file, false, QMPlay2Core.getIconFromTheme("applications-multimedia"), false);
	newAction(Add::tr("&Directory"), this, dir, false, QMPlay2Core.getIconFromTheme("folder"), false);
	newAction(Add::tr("&Address"), this, address, false, QMPlay2Core.getIconFromTheme("application-x-mswinurl"), false);
	addSeparator();
}
MenuBar::Playlist::Sort::Sort(QMenu *parent) :
	QMenu(Sort::tr("&Sort"), parent)
{
	newAction(Sort::tr("&From the shortest to the longest"), this, timeSort1, false, QIcon(), false);
	newAction(Sort::tr("&From the longest to the shortest"), this, timeSort2, false, QIcon(), false);
	addSeparator();
	newAction(Sort::tr("&A-Z"), this, titleSort1, false, QIcon(), false);
	newAction(Sort::tr("&Z-A"), this, titleSort2, false, QIcon(), false);
}

MenuBar::Player::Player(MenuBar *parent) :
	QMenu(Player::tr("&Player"), parent)
{
	newAction(QString(), this,  togglePlay, false, QMPlay2Core.getIconFromTheme("media-playback-start"), false);
	newAction(Player::tr("&Stop"), this, stop, false, QMPlay2Core.getIconFromTheme("media-playback-stop"), false);
	newAction(Player::tr("&Next"), this, next, true, QMPlay2Core.getIconFromTheme("media-skip-forward"), false);
	newAction(Player::tr("&Previous"), this, prev, true, QMPlay2Core.getIconFromTheme("media-skip-backward"), false);
	newAction(Player::tr("Next &frame"), this, nextFrame, true, QIcon(), false);
	addSeparator();

	repeat = new Repeat(this);
	addMenu(repeat);

	newAction(Player::tr("A&-B Repeat"), this, abRepeat, true, QIcon(), false);

	addSeparator();
	newAction(Player::tr("Seek &forward"), this, seekF, true, QIcon(), false);
	newAction(Player::tr("seek &backward"), this, seekB, true, QIcon(), false);
	newAction(Player::tr("Long &seek &forward"), this, lSeekF, true, QIcon(), false);
	newAction(Player::tr("Long s&eek backward"), this, lSeekB, true, QIcon(), false);
	addSeparator();
	newAction(Player::tr("Fa&ster"), this, speedUp, true, QIcon(), false);
	newAction(Player::tr("Slowe&r"), this, slowDown, true, QIcon(), false);
	newAction(Player::tr("&Set speed"), this, setSpeed, false, QIcon(), false);
	addSeparator();
	newAction(Player::tr("Zoom i&n"), this, zoomIn, true, QIcon(), false);
	newAction(Player::tr("Zoom ou&t"), this, zoomOut, true, QIcon(), false);
	newAction(Player::tr("Toggle &aspect ratio"), this, switchARatio, true, QIcon(), false);

	aRatio = new AspectRatio(this);
	addMenu(aRatio);

	newAction(Player::tr("Reset image &settings"), this, reset, false, QIcon(), false);

	addSeparator();
	newAction(Player::tr("Volume &up"), this, volUp, true, QIcon(), false);
	newAction(Player::tr("Volume &down"), this, volDown, true, QIcon(), false);
	newAction(Player::tr("&Mute"), this, toggleMute, false, QMPlay2Core.getIconFromTheme("audio-volume-high"), true);

	if (!QMPlay2GUI.pipe)
		detach = NULL;
	else
	{
		addSeparator();
		newAction(Player::tr("Detach from receiving &commands"), this, detach, false, QIcon(), false);
	}

	if (!QMPlay2CoreClass::canSuspend())
		suspend = NULL;
	else
	{
		addSeparator();
		newAction(Player::tr("Suspend after playbac&k is finished"), this, suspend, false, QIcon(), true);
	}
}
MenuBar::Player::Repeat::Repeat(QMenu *parent) :
	QMenu(Repeat::tr("&Repeat"), parent)
{
	choice = new QActionGroup(this);
	choice->addAction(newAction(Repeat::tr("&No repeating"), this, repeatActions[RepeatNormal], false, QIcon(), true));
	addSeparator();
	choice->addAction(newAction(Repeat::tr("&Entry repeating"), this, repeatActions[RepeatEntry], false, QIcon(), true));
	choice->addAction(newAction(Repeat::tr("&Group repeating"), this, repeatActions[RepeatGroup], false, QIcon(), true));
	choice->addAction(newAction(Repeat::tr("&Playlist repeating"), this, repeatActions[RepeatList], false, QIcon(), true));
	addSeparator();
	choice->addAction(newAction(Repeat::tr("R&andom"), this, repeatActions[RandomMode], false, QIcon(), true));
	choice->addAction(newAction(Repeat::tr("Random in &group"), this, repeatActions[RandomGroupMode], false, QIcon(), true));
	addSeparator();
	choice->addAction(newAction(Repeat::tr("Random and &repeat"), this, repeatActions[RepeatRandom], false, QIcon(), true));
	choice->addAction(newAction(Repeat::tr("Random in group and repea&t"), this, repeatActions[RepeatRandomGroup], false, QIcon(), true));

	for (int i = 0; i < RepeatModeCount; ++i)
		repeatActions[i]->setProperty("enumValue", i);
}
MenuBar::Player::AspectRatio::AspectRatio(QMenu *parent) :
	QMenu(AspectRatio::tr("&Aspect ratio"), parent)
{
	choice = new QActionGroup(this);
	choice->addAction(newAction(AspectRatio::tr("&Auto"), this, _auto, false, QIcon(), true));
	addSeparator();
	choice->addAction(newAction("&1:1", this, _1x1, false, QIcon(), true));
	choice->addAction(newAction("&4:3", this, _4x3, false, QIcon(), true));
	choice->addAction(newAction("&5:4", this, _5x4, false, QIcon(), true));
	choice->addAction(newAction("&16:9", this, _16x9, false, QIcon(), true));
	choice->addAction(newAction("&3:2", this, _3x2, false, QIcon(), true));
	choice->addAction(newAction("&21:9", this, _21x9, false, QIcon(), true));
	addSeparator();
	choice->addAction(newAction(AspectRatio::tr("D&epends on size"), this, sizeDep, false, QIcon(), true));
	choice->addAction(newAction(AspectRatio::tr("&Disabled"), this, off, false, QIcon(), true));

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
	newAction(Playback::tr("&Audio enabled"), this, toggleAudio, false, QIcon(), true)->setObjectName("toggleAudio");
	toggleAudio->setChecked(true);

	audioChannels = new AudioChannels(this);
	addMenu(audioChannels);

	addSeparator();

	newAction(Playback::tr("&Video enabled"), this, toggleVideo, false, QIcon(), true)->setObjectName("toggleVideo");
	toggleVideo->setChecked(true);

	videoFilters = new VideoFilters(this);
	addMenu(videoFilters);

	newAction(Playback::tr("Set &video delay"), this, videoSync, false, QIcon(), false);
	newAction(Playback::tr("&Delay video") + " (100ms)", this, slowDownVideo, true, QIcon(), false);
	newAction(Playback::tr("&Speed up video") + " (100ms)", this, speedUpVideo, true, QIcon(), false);
	addSeparator();
	newAction(Playback::tr("&Subtitles enabled"), this, toggleSubtitles, false, QIcon(), true)->setObjectName("toggleSubtitles");
	toggleSubtitles->setChecked(true);
	newAction(Playback::tr("Add &subtities from file"), this, subsFromFile, false, QIcon(), false);
	newAction(Playback::tr("Set &subtitles delay"), this, subtitlesSync, true, QIcon(), false);
	newAction(Playback::tr("&Delay subtitiles") + " (100ms)", this, slowDownSubtitles, true, QIcon(), false);
	newAction(Playback::tr("&Speed up subtitles") + " (100ms)", this, speedUpSubtitles, true, QIcon(), false);
	newAction(Playback::tr("Scale up subt&itles"), this, biggerSubtitles, true, QIcon(), false);
	newAction(Playback::tr("Scale down sub&titles"), this, smallerSubtitles, true, QIcon(), false);
	addSeparator();
	newAction(Playback::tr("&Playback settings"), this, playbackSettings, true, QMPlay2Core.getIconFromTheme("configure"), false);
	addSeparator();
	newAction(Playback::tr("&Screen shot"), this, screenShot, true, QIcon(), false);
}
MenuBar::Playback::VideoFilters::VideoFilters(QMenu *parent) :
	QMenu(VideoFilters::tr("Video &filters"), parent)
{
	/** Korektor wideo */
	videoAdjustmentMenu = new QMenu(VideoFilters::tr("Video &adjustment"), this);
	addMenu(videoAdjustmentMenu);
	QWidgetAction *widgetAction = new QWidgetAction(this);
	widgetAction->setDefaultWidget(QMPlay2GUI.videoAdjustment);
	QMPlay2GUI.videoAdjustment->setObjectName(videoAdjustmentMenu->title().remove('&'));
	videoAdjustmentMenu->addAction(widgetAction);
	/**/
	addSeparator();
	newAction(VideoFilters::tr("&Spherical view"), this, spherical, true, QIcon(), true);
	addSeparator();
	newAction(VideoFilters::tr("&Horizontal flip"), this, hFlip, true, QIcon(), true);
	newAction(VideoFilters::tr("&Vertical flip"), this, vFlip, true, QIcon(), true);
	newAction(VideoFilters::tr("&Rotate 90°"), this, rotate90, true, QIcon(), true);
	addSeparator();
	newAction(VideoFilters::tr("&More filters"), this, more, false, QIcon(), false);
}
MenuBar::Playback::AudioChannels::AudioChannels(QMenu *parent) :
	QMenu(AudioChannels::tr("&Channels") , parent)
{
	choice = new QActionGroup(this);
	choice->addAction(newAction(AudioChannels::tr("&Autodetect"), this, _auto, false, QIcon(), true));
	addSeparator();
	choice->addAction(newAction(AudioChannels::tr("&Mono"), this, _1, false, QIcon(), true));
	choice->addAction(newAction(AudioChannels::tr("&Stereo"), this, _2, false, QIcon(), true));
	choice->addAction(newAction("&4.0", this, _4, false, QIcon(), true));
	choice->addAction(newAction("&5.1", this, _6, false, QIcon(), true));
	choice->addAction(newAction("&7.1", this, _8, false, QIcon(), true));
	addSeparator();
	choice->addAction(newAction(AudioChannels::tr("&Other"), this, other, false, QIcon(), true));

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
	newAction(Options::tr("&Settings"), this, settings, false, QMPlay2Core.getIconFromTheme("configure"), false);
	newAction(Options::tr("&Modules settings"), this, modulesSettings, false, QMPlay2Core.getIconFromTheme("configure"), false);
	addSeparator();
	newAction(Options::tr("&Show tray icon"), this, trayVisible, false, QIcon(), true);
}

MenuBar::Help::Help(MenuBar *parent) :
	QMenu(Help::tr("&Help"), parent)
{
	newAction(Help::tr("&About QMPlay2"), this, about, false, QIcon(), false);
#ifdef UPDATER
	newAction(Help::tr("&Updates"), this, updates, false, QIcon(), false);
#endif
	addSeparator();
	newAction(Help::tr("About &Qt"), this, aboutQt, false, QIcon(), false);
}

void MenuBar::setKeyShourtcuts()
{
	Settings &settings = QMPlay2Core.getSettings();

	window->toggleVisibility->setShortcut(settings.getString("KeyBindings/Window-toggleVisibility"));
	window->toggleFullScreen->setShortcut(settings.getString("KeyBindings/Window-toggleFullScreen"));
	window->toggleCompactView->setShortcut(settings.getString("KeyBindings/Window-toggleCompactView"));
	window->close->setShortcut(settings.getString("KeyBindings/Window-close"));


	playlist->stopLoading->setShortcut(settings.getString("KeyBindings/Playlist-stopLoading"));
	playlist->sync->setShortcut(settings.getString("KeyBindings/Playlist-sync"));
	playlist->loadPlist->setShortcut(settings.getString("KeyBindings/Playlist-loadPlist"));
	playlist->savePlist->setShortcut(settings.getString("KeyBindings/Playlist-savePlist"));
	playlist->saveGroup->setShortcut(settings.getString("KeyBindings/Playlist-saveGroup"));
	playlist->delEntries->setShortcut(settings.getString("KeyBindings/Playlist-delEntries"));
	playlist->delNonGroupEntries->setShortcut(settings.getString("KeyBindings/Playlist-delNonGroupEntries"));
	playlist->clear->setShortcut(settings.getString("KeyBindings/Playlist-clear"));
	playlist->copy->setShortcut(settings.getString("KeyBindings/Playlist-copy"));
	playlist->paste->setShortcut(settings.getString("KeyBindings/Playlist-paste"));
	playlist->newGroup->setShortcut(settings.getString("KeyBindings/Playlist-newGroup"));
	playlist->renameGroup->setShortcut(settings.getString("KeyBindings/Playlist-renameGroup"));
	playlist->find->setShortcut(settings.getString("KeyBindings/Playlist-find"));
	playlist->collapseAll->setShortcut(settings.getString("KeyBindings/Playlist-collapseAll"));
	playlist->expandAll->setShortcut(settings.getString("KeyBindings/Playlist-expandAll"));
	playlist->goToPlayback->setShortcut(settings.getString("KeyBindings/Playlist-goToPlayback"));
	playlist->queue->setShortcut(settings.getString("KeyBindings/Playlist-queue"));
	playlist->entryProperties->setShortcut(settings.getString("KeyBindings/Playlist-entryProperties"));

	playlist->add->file->setShortcut(settings.getString("KeyBindings/Playlist-Add-file"));
	playlist->add->dir->setShortcut(settings.getString("KeyBindings/Playlist-Add-dir"));
	playlist->add->address->setShortcut(settings.getString("KeyBindings/Playlist-Add-address"));

	playlist->sort->timeSort1->setShortcut(settings.getString("KeyBindings/Playlist-Sort-timeSort1"));
	playlist->sort->timeSort2->setShortcut(settings.getString("KeyBindings/Playlist-Sort-timeSort2"));
	playlist->sort->titleSort1->setShortcut(settings.getString("KeyBindings/Playlist-Sort-titleSort1"));
	playlist->sort->titleSort2->setShortcut(settings.getString("KeyBindings/Playlist-Sort-titleSort2"));


	player->togglePlay->setShortcut(settings.getString("KeyBindings/Player-togglePlay"));
	player->stop->setShortcut(settings.getString("KeyBindings/Player-stop"));
	player->next->setShortcut(settings.getString("KeyBindings/Player-next"));
	player->prev->setShortcut(settings.getString("KeyBindings/Player-prev"));
	player->nextFrame->setShortcut(settings.getString("KeyBindings/Player-nextFrame"));
	player->abRepeat->setShortcut(settings.getString("KeyBindings/Player-abRepeat"));
	player->seekF->setShortcut(settings.getString("KeyBindings/Player-seekF"));
	player->seekB->setShortcut(settings.getString("KeyBindings/Player-seekB"));
	player->lSeekF->setShortcut(settings.getString("KeyBindings/Player-lSeekF"));
	player->lSeekB->setShortcut(settings.getString("KeyBindings/Player-lSeekB"));
	player->speedUp->setShortcut(settings.getString("KeyBindings/Player-speedUp"));
	player->slowDown->setShortcut(settings.getString("KeyBindings/Player-slowDown"));
	player->setSpeed->setShortcut(settings.getString("KeyBindings/Player-setSpeed"));
	player->zoomIn->setShortcut(settings.getString("KeyBindings/Player-zoomIn"));
	player->zoomOut->setShortcut(settings.getString("KeyBindings/Player-zoomOut"));
	player->switchARatio->setShortcut(settings.getString("KeyBindings/Player-switchARatio"));
	player->reset->setShortcut(settings.getString("KeyBindings/Player-reset"));
	player->volUp->setShortcut(settings.getString("KeyBindings/Player-volUp"));
	player->volDown->setShortcut(settings.getString("KeyBindings/Player-volDown"));
	player->toggleMute->setShortcut(settings.getString("KeyBindings/Player-toggleMute"));
	if (player->detach)
		player->detach->setShortcut(settings.getString("KeyBindings/Player-detach"));
	if (player->suspend)
		player->suspend->setShortcut(settings.getString("KeyBindings/Player-suspend"));

	player->repeat->repeatActions[RepeatNormal]->setShortcut(settings.getString("KeyBindings/Player-Repeat-RepeatNormal"));
	player->repeat->repeatActions[RepeatEntry]->setShortcut(settings.getString("KeyBindings/Player-Repeat-RepeatEntry"));
	player->repeat->repeatActions[RepeatGroup]->setShortcut(settings.getString("KeyBindings/Player-Repeat-RepeatGroup"));
	player->repeat->repeatActions[RepeatList]->setShortcut(settings.getString("KeyBindings/Player-Repeat-RepeatList"));
	player->repeat->repeatActions[RandomMode]->setShortcut(settings.getString("KeyBindings/Player-Repeat-RandomMode"));
	player->repeat->repeatActions[RandomGroupMode]->setShortcut(settings.getString("KeyBindings/Player-Repeat-RandomGroupMode"));
	player->repeat->repeatActions[RepeatRandom]->setShortcut(settings.getString("KeyBindings/Player-Repeat-RepeatRandom"));
	player->repeat->repeatActions[RepeatRandomGroup]->setShortcut(settings.getString("KeyBindings/Player-Repeat-RepeatRandomGroup"));


	playback->toggleAudio->setShortcut(settings.getString("KeyBindings/Playback-toggleAudio"));
	playback->toggleVideo->setShortcut(settings.getString("KeyBindings/Playback-toggleVideo"));
	playback->videoSync->setShortcut(settings.getString("KeyBindings/Playback-videoSync"));
	playback->slowDownVideo->setShortcut(settings.getString("KeyBindings/Playback-slowDownVideo"));
	playback->speedUpVideo->setShortcut(settings.getString("KeyBindings/Playback-speedUpVideo"));
	playback->toggleSubtitles->setShortcut(settings.getString("KeyBindings/Playback-toggleSubtitles"));
	playback->subsFromFile->setShortcut(settings.getString("KeyBindings/Playback-subsFromFile"));
	playback->subtitlesSync->setShortcut(settings.getString("KeyBindings/Playback-subtitlesSync"));
	playback->slowDownSubtitles->setShortcut(settings.getString("KeyBindings/Playback-slowDownSubtitles"));
	playback->speedUpSubtitles->setShortcut(settings.getString("KeyBindings/Playback-speedUpSubtitles"));
	playback->biggerSubtitles->setShortcut(settings.getString("KeyBindings/Playback-biggerSubtitles"));
	playback->smallerSubtitles->setShortcut(settings.getString("KeyBindings/Playback-smallerSubtitles"));
	playback->playbackSettings->setShortcut(settings.getString("KeyBindings/Playback-playbackSettings"));
	playback->screenShot->setShortcut(settings.getString("KeyBindings/Playback-screenShot"));

	playback->videoFilters->spherical->setShortcut(settings.getString("KeyBindings/Playback-VideoFilters-spherical"));
	playback->videoFilters->hFlip->setShortcut(settings.getString("KeyBindings/Playback-VideoFilters-hFlip"));
	playback->videoFilters->vFlip->setShortcut(settings.getString("KeyBindings/Playback-VideoFilters-vFlip"));
	playback->videoFilters->rotate90->setShortcut(settings.getString("KeyBindings/Playback-VideoFilters-rotate90"));
	playback->videoFilters->more->setShortcut(settings.getString("KeyBindings/Playback-VideoFilters-more"));


	options->settings->setShortcut(settings.getString("KeyBindings/Options-settings"));
	options->modulesSettings->setShortcut(settings.getString("KeyBindings/Options-modulesSettings"));
	options->trayVisible->setShortcut(settings.getString("KeyBindings/Options-trayVisible"));


	help->about->setShortcut(settings.getString("KeyBindings/Help-about"));
#ifdef UPDATER
	help->updates->setShortcut(settings.getString("KeyBindings/Help-updates"));
#endif
}

void MenuBar::widgetsMenuShow()
{
	widgets->menuShow();
}
