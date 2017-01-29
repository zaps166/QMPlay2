/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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
#include <qevent.h>

/* QMPlay2 gui */
#include <Main.hpp>
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
#include <VideoAdjustment.hpp>
#include <ShortcutHandler.hpp>
#include <VolWidget.hpp>
#include <ScreenSaver.hpp>

using Functions::timeToStr;

/* QMPlay2 lib */
#include <QMPlay2Extensions.hpp>
#include <Settings.hpp>
#include <MenuBar.hpp>
#include <SubsDec.hpp>
#include <IPC.hpp>

#include <math.h>

#if QT_VERSION >= 0x050000 && defined Q_OS_WIN
	#define QT5_WINDOWS
#endif

/* Qt5 or (Qt4 in Windows) */
#if (QT_VERSION >= 0x050000 || defined Q_OS_WIN)
	#define UseMainWidgetTmpStyle
#endif

#ifdef UseMainWidgetTmpStyle
/* MainWidgetTmpStyle -  dock widget separator extent must be larger for touch screens */
class MainWidgetTmpStyle : public QCommonStyle
{
public:
	int pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
	{
		const int pM = QCommonStyle::pixelMetric(metric, option, widget);
		if (metric == QStyle::PM_DockWidgetSeparatorExtent)
			return pM * 5 / 2;
		return pM;
	}
};
#endif

static void copyMenu(QMenu *dest, QMenu *src, QMenu *dontCopy = NULL)
{
	QMenu *newMenu = new QMenu(src->title(), dest);
	foreach (QAction *act, src->actions())
	{
		QMenu *menu = act->menu();
		if (!menu)
			newMenu->addAction(act);
		else if (menu != dontCopy)
			copyMenu(newMenu, menu, dontCopy);
		else if (dontCopy)
			newMenu->addMenu(dontCopy);
	}
	dest->addMenu(newMenu);
}

/* MainWidget */
MainWidget::MainWidget(QPair<QStringList, QStringList> &QMPArguments)
#ifdef UPDATER
	: updater(this)
#endif
{
	QMPlay2GUI.videoAdjustment = new VideoAdjustment;
	QMPlay2GUI.shortcutHandler = new ShortcutHandler(this);
	QMPlay2GUI.mainW = this;

#ifdef UseMainWidgetTmpStyle
	#if QT_VERSION >= 0x050000
		bool createTmpStyle = false;
		/* Looking for touch screen */
		foreach (const QTouchDevice *touchDev, QTouchDevice::devices())
		{
			/* Touchscreen found */
			if (touchDev->type() == QTouchDevice::TouchScreen)
			{
				createTmpStyle = true;
				break;
			}
		}
		if (createTmpStyle)
	#elif defined Q_OS_WIN
		if (QSysInfo::windowsVersion() > QSysInfo::WV_6_1) //Qt4 can't detect touchscreen, so MainWidgetTmpStyle will be used only with Windows >= 8.0
	#endif
			setStyle(QScopedPointer<MainWidgetTmpStyle>(new MainWidgetTmpStyle).data()); //Is it always OK?
#endif

	setObjectName("MainWidget");

	settingsW = NULL;
	aboutW = NULL;
	lastFocusWidget = NULL;

	isCompactView = wasShow = fullScreen = seekSFocus = false;

	if (QMPlay2GUI.pipe)
		connect(QMPlay2GUI.pipe, SIGNAL(newConnection(IPCSocket *)), this, SLOT(newConnection(IPCSocket *)));

	Settings &settings = QMPlay2Core.getSettings();

	SettingsWidget::InitSettings();
#ifndef Q_OS_ANDROID
	settings.init("MainWidget/WidgetsLocked", false);
#else
	settings.init("MainWidget/WidgetsLocked", true);
#endif

	QMPlay2GUI.menuBar = new MenuBar;

	tray = new QSystemTrayIcon(this);
	tray->setIcon(QMPlay2Core.getIconFromTheme("QMPlay2-panel", QMPlay2Core.getQMPlay2Pixmap()));
	tray->setVisible(settings.getBool("TrayVisible", true));
	QMPlay2Core.systemTray = tray;

	setDockOptions(AllowNestedDocks | AnimatedDocks | AllowTabbedDocks);
	setMouseTracking(true);
	updateWindowTitle();

	statusBar = new QStatusBar;
	statusBar->setSizeGripEnabled(false);
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
	DockWidget *firstVisualization = NULL;
	foreach (QMPlay2Extensions *QMPlay2Ext, QMPlay2Extensions::QMPlay2ExtensionsList()) //GUI Extensions
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
			else if (QMPlay2Ext->addressPrefixList(false).isEmpty())
				tabifyDockWidget(videoDock, dw);
			else
				tabifyDockWidget(playlistDock, dw);
			dw->setVisible(true);
		}
#else //On Android tabify docks (usually screen is too small)
	addDockWidget(Qt::TopDockWidgetArea, videoDock);
	addDockWidget(Qt::BottomDockWidgetArea, playlistDock);
	tabifyDockWidget(playlistDock, infoDock);
	foreach (QMPlay2Extensions *QMPlay2Ext, QMPlay2Extensions::QMPlay2ExtensionsList()) //GUI Extensions
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

	seekS = new Slider;
	seekS->setMaximum(0);
	seekS->setWheelStep(settings.getInt("ShortSeek"));
	mainTB->addWidget(seekS);
	updatePos(0);

	vLine = new QFrame;
	vLine->setFrameShape(QFrame::VLine);
	vLine->setFrameShadow(QFrame::Sunken);
	mainTB->addWidget(vLine);

	mainTB->addAction(menuBar->player->toggleMute);

	volW = new VolWidget(settings.getInt("MaxVol"));
	mainTB->addWidget(volW);

	/**/

	Appearance::init();

	/* Connects */
	connect(qApp, SIGNAL(focusChanged(QWidget *, QWidget *)), this, SLOT(focusChanged(QWidget *, QWidget *)));

	connect(infoDock, SIGNAL(seek(int)), this, SLOT(seek(int)));
	connect(infoDock, SIGNAL(chStream(const QString &)), &playC, SLOT(chStream(const QString &)));
	connect(infoDock, SIGNAL(saveCover()), &playC, SLOT(saveCover()));

	connect(videoDock, SIGNAL(resized(int, int)), &playC, SLOT(videoResized(int, int)));
	connect(videoDock, SIGNAL(itemDropped(const QString &, bool)), this, SLOT(itemDropped(const QString &, bool)));

	connect(playlistDock, SIGNAL(play(const QString &)), &playC, SLOT(play(const QString &)));
	connect(playlistDock, SIGNAL(stop()), &playC, SLOT(stop()));

	connect(seekS, SIGNAL(valueChanged(int)), this, SLOT(seek(int)));
	connect(seekS, SIGNAL(mousePosition(int)), this, SLOT(mousePositionOnSlider(int)));

	connect(volW, SIGNAL(volumeChanged(int, int)), &playC, SLOT(volume(int, int)));

	connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconClicked(QSystemTrayIcon::ActivationReason)));

	connect(&QMPlay2Core, SIGNAL(sendMessage(const QString &, const QString &, int, int)), this, SLOT(showMessage(const QString &, const QString &, int, int)));
	connect(&QMPlay2Core, SIGNAL(processParam(const QString &, const QString &)), this, SLOT(processParam(const QString &, const QString &)));
	connect(&QMPlay2Core, SIGNAL(statusBarMessage(const QString &, int)), this, SLOT(statusBarMessage(const QString &, int)));
	connect(&QMPlay2Core, SIGNAL(showSettings(const QString &)), this, SLOT(showSettings(const QString &)));

	connect(QMPlay2GUI.videoAdjustment, SIGNAL(videoAdjustmentChanged()), &playC, SLOT(videoAdjustmentChanged()));

	connect(&playC, SIGNAL(chText(const QString &)), stateL, SLOT(setText(const QString &)));
	connect(&playC, SIGNAL(updateLength(int)), this, SLOT(setSeekSMaximum(int)));
	connect(&playC, SIGNAL(updatePos(int)), this, SLOT(updatePos(int)));
	connect(&playC, SIGNAL(playStateChanged(bool)), this, SLOT(playStateChanged(bool)));
	connect(&playC, SIGNAL(setCurrentPlaying()), playlistDock, SLOT(setCurrentPlaying()));
	connect(&playC, SIGNAL(setInfo(const QString &, bool, bool)), infoDock, SLOT(setInfo(const QString &, bool, bool)));
	connect(&playC, SIGNAL(updateCurrentEntry(const QString &, double)), playlistDock, SLOT(updateCurrentEntry(const QString &, double)));
	connect(&playC, SIGNAL(playNext(bool)), playlistDock, SLOT(next(bool)));
	connect(&playC, SIGNAL(clearCurrentPlaying()), playlistDock, SLOT(clearCurrentPlaying()));
	connect(&playC, SIGNAL(clearInfo()), infoDock, SLOT(clear()));
	connect(&playC, SIGNAL(quit()), this, SLOT(deleteLater()));
	connect(&playC, SIGNAL(resetARatio()), this, SLOT(resetARatio()));
	connect(&playC, SIGNAL(updateBitrateAndFPS(int, int, double, double, bool)), infoDock, SLOT(updateBitrateAndFPS(int, int, double, double, bool)));
	connect(&playC, SIGNAL(updateBuffered(qint64, qint64, double, double)), infoDock, SLOT(updateBuffered(qint64, qint64, double, double)));
	connect(&playC, SIGNAL(updateBufferedRange(int, int)), seekS, SLOT(drawRange(int, int)));
	connect(&playC, SIGNAL(updateWindowTitle(const QString &)), this, SLOT(updateWindowTitle(const QString &)));
	connect(&playC, SIGNAL(updateImage(const QImage &)), videoDock, SLOT(updateImage(const QImage &)));
	connect(&playC, SIGNAL(videoStarted()), this, SLOT(videoStarted()));
	connect(&playC, SIGNAL(uncheckSuspend()), this, SLOT(uncheckSuspend()));
	/**/

	if (settings.getBool("MainWidget/TabPositionNorth"))
		setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

#if !defined Q_OS_MAC && !defined Q_OS_ANDROID
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
	QMPlay2GUI.menuBar->widgets->hideMenuAct = NULL;
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

	volW->setVolume(settings.getInt("VolumeL"), settings.getInt("VolumeR"), true);
	if (settings.getBool("Mute"))
		menuBar->player->toggleMute->trigger();

	RepeatMode repeatMode = RepeatNormal;
	if (settings.getBool("RestoreRepeatMode"))
		repeatMode = settings.getWithBounds("RepeatMode", RepeatNormal, RepeatRandomGroup);
	menuBar->player->repeat->repeatActions[repeatMode]->trigger();

	if (settings.getBool("RestoreVideoEqualizer"))
		QMPlay2GUI.videoAdjustment->restoreValues();

	fullScreenDockWidgetState = settings.getByteArray("MainWidget/FullScreenDockWidgetState");
#if defined Q_OS_MAC || defined Q_OS_ANDROID
	show();
#else
	setVisible(settings.getBool("MainWidget/isVisible", true) ? true : !(QSystemTrayIcon::isSystemTrayAvailable() && tray->isVisible()));
#endif

#if QT_VERSION >= 0x050400
	foreach (QObject *obj, children())
	{
		if (QTabBar *tabBar = qobject_cast<QTabBar *>(obj))
		{
			tabBar->setAcceptDrops(true);
			tabBar->setChangeCurrentOnDrag(true);
		}
	}
#endif

	playlistDock->load(QMPlay2Core.getSettingsDir() + "Playlist.pls");

	bool noplay = false;
	while (!QMPArguments.first.isEmpty())
	{
		const QString param = QMPArguments.first.takeFirst();
		const QString data  = QMPArguments.second.takeFirst();
		noplay |= (param == "open" || param == "noplay");
		processParam(param, data);
	}

	if (!noplay)
	{
		const QString url = settings.getString("Url");
		if (!url.isEmpty() && url == playlistDock->getUrl())
		{
			const double pos = settings.getInt("Pos", 0);
			playC.setDoSilenceOnStart();
			playlistDock->start();
			if (pos > 0.0)
				seek(pos);
		}
		else
			playStateChanged(false);
	}

#ifdef UPDATER
	if (settings.getBool("AutoUpdates"))
		updater.downloadUpdate();
#endif
}
MainWidget::~MainWidget()
{
	QMPlay2Extensions::closeExtensions();
	emit QMPlay2Core.restoreCursor();
	QMPlay2GUI.mainW = NULL;
	QCoreApplication::quit();
}

void MainWidget::detachFromPipe()
{
	if (QMPlay2GUI.pipe)
	{
		QMPlay2GUI.pipe->deleteLater();
		QMPlay2GUI.pipe = NULL;
	}
	if (menuBar->player->detach)
	{
		menuBar->player->detach->deleteLater();
		menuBar->player->detach = NULL;
	}
}

void MainWidget::focusChanged(QWidget *old, QWidget *now)
{
	if ((qobject_cast<QTreeWidget *>(old) || qobject_cast<QSlider *>(old) || qobject_cast<QComboBox *>(old) || qobject_cast<QListWidget *>(old)) && old != seekS)
		menuBar->player->seekActionsEnable(true);
	if ((qobject_cast<QTreeWidget *>(now) || qobject_cast<QSlider *>(now) || qobject_cast<QComboBox *>(now) || qobject_cast<QListWidget *>(now)) && now != seekS)
		menuBar->player->seekActionsEnable(false);

	if (qobject_cast<QAbstractButton *>(old))
		menuBar->player->togglePlay->setShortcutContext(Qt::WindowShortcut);
	if (qobject_cast<QAbstractButton *>(now))
		menuBar->player->togglePlay->setShortcutContext(Qt::WidgetShortcut);

	seekSFocus = (now == seekS);
}

void MainWidget::processParam(const QString &param, const QString &data)
{
	if (param == "open")
	{
		QMPlay2Core.getSettings().remove("Pos");
		QMPlay2Core.getSettings().remove("Url");
		if (data.contains('\n'))
			playlistDock->addAndPlay(data.split('\n', QString::SkipEmptyParts));
		else
			playlistDock->addAndPlay(data);
	}
	else if (param == "enqueue")
	{
		if (data.contains('\n'))
			playlistDock->add(data.split('\n', QString::SkipEmptyParts));
		else
			playlistDock->add(data);
	}
	else if (param == "toggle")
		togglePlay();
	else if (param == "show")
	{
		if (!isVisible())
			toggleVisibility();
		if (isMinimized())
		{
			showNormal();
			activateWindow();
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
		seek(data.toInt());
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
	tray->setToolTip(title);
	title.replace('\n', ' ');
	setWindowTitle(title);
}
void MainWidget::videoStarted()
{
	if (QMPlay2Core.getSettings().getBool("AutoOpenVideoWindow"))
	{
		if (!videoDock->isVisible())
			videoDock->show();
		videoDock->raise();
	}
}

void MainWidget::togglePlay()
{
	if (playC.isPlaying())
		playC.togglePause();
	else
		playlistDock->start();
}
void MainWidget::seek(int i)
{
	if (!seekS->ignoringValueChanged())
		playC.seek(i);
}
void MainWidget::playStateChanged(bool b)
{
	if (b)
	{
		menuBar->player->togglePlay->setIcon(QMPlay2Core.getIconFromTheme("media-playback-pause"));
		menuBar->player->togglePlay->setText(tr("&Paused"));
	}
	else
	{
		menuBar->player->togglePlay->setIcon(QMPlay2Core.getIconFromTheme("media-playback-start"));
		menuBar->player->togglePlay->setText(tr("&Play"));
	}
	emit QMPlay2Core.playStateChanged(playC.isPlaying() ? (b ? "Playing" : "Paused") : "Stopped");
}

void MainWidget::volUpDown()
{
	if (sender() == menuBar->player->volUp)
		volW->changeVolume(5);
	else if (sender() == menuBar->player->volDown)
		volW->changeVolume(-5);
}
void MainWidget::toggleMuteIcon()
{
	if (QAction *toggleMuteAct = qobject_cast<QAction *>(sender()))
		toggleMuteAct->setIcon(QMPlay2Core.getIconFromTheme(toggleMuteAct->isChecked() ? "audio-volume-muted" : "audio-volume-high"));
}
void MainWidget::actionSeek()
{
	qint64 seekTo = 0;
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
		const int max = seekS->maximum();
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
	const int count = menuBar->player->aRatio->choice->actions().count();
	const int idx = menuBar->player->aRatio->choice->actions().indexOf(checked);
	const int checkNewIdx = (idx == count - 1) ? 0 : (idx+1);
	menuBar->player->aRatio->choice->actions()[checkNewIdx]->trigger();
}
void MainWidget::resetARatio()
{
	if (menuBar->player->aRatio->choice->checkedAction() != menuBar->player->aRatio->choice->actions()[0])
		menuBar->player->aRatio->choice->actions()[0]->trigger();
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

void MainWidget::visualizationFullScreen()
{
	if (!fullScreen)
	{
		videoDock->setWidget((QWidget *)sender());
		toggleFullScreen();
	}
}
void MainWidget::hideAllExtensions()
{
	foreach (QMPlay2Extensions *QMPlay2Ext, QMPlay2Extensions::QMPlay2ExtensionsList())
		if (DockWidget *dw = QMPlay2Ext->getDockWidget())
			dw->hide();
}
void MainWidget::toggleVisibility()
{
#ifndef Q_OS_ANDROID
	const bool isTray = QSystemTrayIcon::isSystemTrayAvailable() && tray->isVisible();
	static bool maximized;
	if (isVisible())
	{
		if (fullScreen)
			toggleFullScreen();
		if (!isTray)
			showMinimized();
		else
		{
			menuBar->options->trayVisible->setEnabled(false);
			lastFocusWidget = focusWidget(); //Hold the current focus widget, because hiding from Mate systray applet and Xfwm4 can change focus on Qt5 (QTBUG-53993)
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
		bool doRestoreFocus = true;
		if (isTray)
			menuBar->options->trayVisible->setEnabled(true);
		if (!maximized)
			showNormal();
		else
		{
			showMaximized();
			if (!isCompactView && !dockWidgetState.isEmpty())
			{
				restoreState(dockWidgetState);
				QMetaObject::invokeMethod(this, "delayedRestore", Qt::QueuedConnection, Q_ARG(QByteArray, dockWidgetState));
				dockWidgetState.clear();
				doRestoreFocus = false;
			}
			maximized = false;
		}
		activateWindow();
		if (doRestoreFocus)
			restoreFocus();
	}
#endif
}
void MainWidget::createMenuBar()
{
	menuBar = QMPlay2GUI.menuBar;

	foreach (Module *module, QMPlay2Core.getPluginsInstance())
		foreach (QAction *act, module->getAddActions())
		{
			act->setParent(menuBar->playlist->add);
			menuBar->playlist->add->addAction(act);
		}
	menuBar->playlist->add->setEnabled(menuBar->playlist->add->actions().count());

	connect(menuBar->window->toggleVisibility, SIGNAL(triggered()), this, SLOT(toggleVisibility()));
	connect(menuBar->window->toggleFullScreen, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
	connect(menuBar->window->toggleCompactView, SIGNAL(triggered()), this, SLOT(toggleCompactView()));
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
	connect(menuBar->playlist->delEntries, SIGNAL(triggered()), playlistDock, SLOT(delEntries()));
	connect(menuBar->playlist->delNonGroupEntries, SIGNAL(triggered()), playlistDock, SLOT(delNonGroupEntries()));
	connect(menuBar->playlist->clear, SIGNAL(triggered()), playlistDock, SLOT(clear()));
	connect(menuBar->playlist->copy, SIGNAL(triggered()), playlistDock, SLOT(copy()));
	connect(menuBar->playlist->paste, SIGNAL(triggered()), playlistDock, SLOT(paste()));
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
	connect(menuBar->player->nextFrame, SIGNAL(triggered()), &playC, SLOT(nextFrame()));
	foreach (QAction *act, menuBar->player->repeat->actions())
		connect(act, SIGNAL(triggered()), playlistDock, SLOT(repeat()));
	connect(menuBar->player->abRepeat, SIGNAL(triggered()), &playC, SLOT(setAB()));
	connect(menuBar->player->seekF, SIGNAL(triggered()), this, SLOT(actionSeek()));
	connect(menuBar->player->seekB, SIGNAL(triggered()), this, SLOT(actionSeek()));
	connect(menuBar->player->lSeekF, SIGNAL(triggered()), this, SLOT(actionSeek()));
	connect(menuBar->player->lSeekB, SIGNAL(triggered()), this, SLOT(actionSeek()));
	connect(menuBar->player->speedUp, SIGNAL(triggered()), &playC, SLOT(speedUp()));
	connect(menuBar->player->slowDown, SIGNAL(triggered()), &playC, SLOT(slowDown()));
	connect(menuBar->player->setSpeed, SIGNAL(triggered()), &playC, SLOT(setSpeed()));
	connect(menuBar->player->zoomIn, SIGNAL(triggered()), &playC, SLOT(zoomIn()));
	connect(menuBar->player->zoomOut, SIGNAL(triggered()), &playC, SLOT(zoomOut()));
	connect(menuBar->player->switchARatio, SIGNAL(triggered()), this, SLOT(switchARatio()));
	foreach (QAction *act, menuBar->player->aRatio->actions())
	{
		connect(act, SIGNAL(triggered()), &playC, SLOT(aRatio()));
		if (act->objectName() == "auto")
			act->trigger();
	}
	connect(menuBar->player->reset, SIGNAL(triggered()), this, SLOT(resetFlip()));
	connect(menuBar->player->reset, SIGNAL(triggered()), this, SLOT(resetRotate90()));
	connect(menuBar->player->reset, SIGNAL(triggered()), this, SLOT(resetARatio()));
	connect(menuBar->player->reset, SIGNAL(triggered()), &playC, SLOT(zoomReset()));
	connect(menuBar->player->volUp, SIGNAL(triggered()), this, SLOT(volUpDown()));
	connect(menuBar->player->volDown, SIGNAL(triggered()), this, SLOT(volUpDown()));
	connect(menuBar->player->toggleMute, SIGNAL(triggered()), &playC, SLOT(toggleMute()));
	connect(menuBar->player->toggleMute, SIGNAL(triggered()), this, SLOT(toggleMuteIcon()));
	if (menuBar->player->detach)
		connect(menuBar->player->detach, SIGNAL(triggered()), this, SLOT(detachFromPipe()));
	if (menuBar->player->suspend)
		connect(menuBar->player->suspend, SIGNAL(triggered(bool)), &playC, SLOT(suspendWhenFinished(bool)));

	connect(menuBar->playback->toggleAudio, SIGNAL(triggered(bool)), &playC, SLOT(toggleAVS(bool)));
	connect(menuBar->playback->toggleVideo, SIGNAL(triggered(bool)), &playC, SLOT(toggleAVS(bool)));
	connect(menuBar->playback->toggleSubtitles, SIGNAL(triggered(bool)), &playC, SLOT(toggleAVS(bool)));
	connect(menuBar->playback->videoSync, SIGNAL(triggered()), &playC, SLOT(setVideoSync()));
	connect(menuBar->playback->slowDownVideo, SIGNAL(triggered()), &playC, SLOT(slowDownVideo()));
	connect(menuBar->playback->speedUpVideo, SIGNAL(triggered()), &playC, SLOT(speedUpVideo()));
	connect(menuBar->playback->slowDownSubtitles, SIGNAL(triggered()), &playC, SLOT(slowDownSubs()));
	connect(menuBar->playback->speedUpSubtitles, SIGNAL(triggered()), &playC, SLOT(speedUpSubs()));
	connect(menuBar->playback->biggerSubtitles, SIGNAL(triggered()), &playC, SLOT(biggerSubs()));
	connect(menuBar->playback->smallerSubtitles, SIGNAL(triggered()), &playC, SLOT(smallerSubs()));
	connect(menuBar->playback->playbackSettings, SIGNAL(triggered()), this, SLOT(showSettings()));
	connect(menuBar->playback->screenShot, SIGNAL(triggered()), &playC, SLOT(screenShot()));
	connect(menuBar->playback->subsFromFile, SIGNAL(triggered()), this, SLOT(browseSubsFile()));
	connect(menuBar->playback->subtitlesSync, SIGNAL(triggered()), &playC, SLOT(setSubtitlesSync()));
	connect(menuBar->playback->videoFilters->spherical, SIGNAL(triggered(bool)), &playC, SLOT(setSpherical(bool)));
	connect(menuBar->playback->videoFilters->hFlip, SIGNAL(triggered(bool)), &playC, SLOT(setHFlip(bool)));
	connect(menuBar->playback->videoFilters->vFlip, SIGNAL(triggered(bool)), &playC, SLOT(setVFlip(bool)));
	connect(menuBar->playback->videoFilters->rotate90, SIGNAL(triggered(bool)), &playC, SLOT(setRotate90(bool)));
	connect(menuBar->playback->videoFilters->more, SIGNAL(triggered(bool)), this, SLOT(showSettings()));
	foreach (QAction *act, menuBar->playback->audioChannels->actions())
		connect(act, SIGNAL(triggered()), this, SLOT(audioChannelsChanged()));
	SettingsWidget::SetAudioChannelsMenu();

	connect(menuBar->options->settings, SIGNAL(triggered()), this, SLOT(showSettings()));
	connect(menuBar->options->modulesSettings, SIGNAL(triggered()), this, SLOT(showSettings()));
	connect(menuBar->options->trayVisible, SIGNAL(triggered(bool)), tray, SLOT(setVisible(bool)));

	connect(menuBar->help->about, SIGNAL(triggered()), this, SLOT(about()));
#ifdef UPDATER
	connect(menuBar->help->updates, SIGNAL(triggered()), &updater, SLOT(exec()));
#endif
	connect(menuBar->help->aboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

	menuBar->window->toggleCompactView->setChecked(isCompactView);
	menuBar->options->trayVisible->setChecked(tray->isVisible());

	setMenuBar(menuBar);

	QMenu *secondMenu = new QMenu(this);
#ifndef Q_OS_MAC
	copyMenu(secondMenu, menuBar->window);
	secondMenu->addMenu(menuBar->widgets);
	copyMenu(secondMenu, menuBar->playlist, menuBar->playlist->extensions);
	copyMenu(secondMenu, menuBar->player);
	copyMenu(secondMenu, menuBar->playback, menuBar->playback->videoFilters->videoAdjustmentMenu);
	copyMenu(secondMenu, menuBar->options);
	copyMenu(secondMenu, menuBar->help);
	tray->setContextMenu(secondMenu);
#else //On OS X add only the most important menu actions to dock menu
	secondMenu->addAction(menuBar->player->togglePlay);
	secondMenu->addAction(menuBar->player->stop);
	secondMenu->addAction(menuBar->player->next);
	secondMenu->addAction(menuBar->player->prev);
	secondMenu->addSeparator();
	secondMenu->addAction(menuBar->player->toggleMute);
	secondMenu->addSeparator();
	secondMenu->addAction(menuBar->options->settings);
	qt_mac_set_dock_menu(secondMenu);
#endif
}
void MainWidget::trayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason)
	{
		case QSystemTrayIcon::Trigger:
#ifndef Q_OS_MAC
		case QSystemTrayIcon::DoubleClick:
#endif
			toggleVisibility();
			break;
		case QSystemTrayIcon::MiddleClick:
			menuBar->window->toggleCompactView->trigger();
			break;
		default:
			break;
	}
}
void MainWidget::toggleCompactView()
{
	if (!isCompactView)
	{
		dockWidgetState = saveState();

		hideAllExtensions();

#if !defined Q_OS_MAC && !defined Q_OS_ANDROID
		menuBar->hide();
#endif
		mainTB->hide();
		infoDock->hide();
		playlistDock->hide();
		statusBar->hide();
		videoDock->show();

		videoDock->fullScreen(true);

		isCompactView = true;
	}
	else
	{
		videoDock->setLoseHeight(0);
		isCompactView = false;

		restoreState(dockWidgetState);
		dockWidgetState.clear();

		videoDock->fullScreen(false);

#if !defined Q_OS_MAC && !defined Q_OS_ANDROID
		menuBar->setVisible(!hideMenuAct->isChecked());
#endif

		statusBar->show();
	}
}
void MainWidget::toggleFullScreen()
{
	static bool visible, compact_view, tb_movable;
#ifndef Q_OS_ANDROID
	static bool maximized;
#endif
	if (!fullScreen)
	{
		visible = isVisible();

		if ((compact_view = isCompactView))
			toggleCompactView();

#ifndef Q_OS_ANDROID
		maximized = isMaximized();

#ifndef QT5_WINDOWS
		if (isFullScreen())
#endif
			showNormal();

		dockWidgetState = saveState();
		if (!maximized)
			savedGeo = geometry();
#else
		dockWidgetState = saveState();
#endif // Q_OS_ANDROID

#if !defined Q_OS_MAC && !defined Q_OS_ANDROID
		menuBar->hide();
#endif
		statusBar->hide();

		mainTB->hide();
		addToolBar(Qt::BottomToolBarArea, mainTB);
		tb_movable = mainTB->isMovable();
		mainTB->setMovable(false);

		infoDock->hide();
		infoDock->setFeatures(DockWidget::NoDockWidgetFeatures);

		playlistDock->hide();
		playlistDock->setFeatures(DockWidget::NoDockWidgetFeatures);

		addDockWidget(Qt::RightDockWidgetArea, videoDock);
		foreach (QMPlay2Extensions *QMPlay2Ext, QMPlay2Extensions::QMPlay2ExtensionsList())
			if (DockWidget *dw = QMPlay2Ext->getDockWidget())
			{
				if (dw->isVisible())
					visibleQMPlay2Extensions += QMPlay2Ext;
				dw->hide();
				dw->setFeatures(DockWidget::NoDockWidgetFeatures);
			}

		videoDock->fullScreen(true);
		videoDock->show();

#ifdef Q_OS_MAC
		menuBar->window->toggleVisibility->setEnabled(false);
#endif
		menuBar->window->toggleCompactView->setEnabled(false);
		menuBar->window->toggleFullScreen->setShortcuts(QList<QKeySequence>() << menuBar->window->toggleFullScreen->shortcut() << QKeySequence("ESC"));
		fullScreen = true;
		showFullScreen();

		if (playC.isPlaying())
			QMPlay2GUI.screenSaver->inhibit(1);
	}
	else
	{
#ifdef Q_OS_MAC
		menuBar->window->toggleVisibility->setEnabled(true);
#endif
		menuBar->window->toggleCompactView->setEnabled(true);
		menuBar->window->toggleFullScreen->setShortcuts(QList<QKeySequence>() << menuBar->window->toggleFullScreen->shortcut());

		videoDock->setLoseHeight(0);
		fullScreen = false;

#ifndef Q_OS_ANDROID
		showNormal();
		if (maximized)
			showMaximized();
		else
			setGeometry(savedGeo);
#else
		showMaximized();
#endif
#ifdef QT5_WINDOWS
		QCoreApplication::processEvents();
#endif
		restoreState(dockWidgetState);
		dockWidgetState.clear();

		if (!visible) //jeżeli okno było wcześniej ukryte, to ma je znowu ukryć
			toggleVisibility();

		videoDock->fullScreen(false);

		infoDock->setFeatures(QDockWidget::AllDockWidgetFeatures);
		playlistDock->setFeatures(QDockWidget::AllDockWidgetFeatures);
		foreach (QMPlay2Extensions *QMPlay2Ext, QMPlay2Extensions::QMPlay2ExtensionsList())
			if (QDockWidget *dw = QMPlay2Ext->getDockWidget())
				dw->setFeatures(QDockWidget::AllDockWidgetFeatures);

#if !defined Q_OS_MAC && !defined Q_OS_ANDROID
		menuBar->setVisible(!hideMenuAct->isChecked());
#endif
		statusBar->show();

		mainTB->setMovable(tb_movable);

		if (compact_view)
			toggleCompactView();

		playlistDock->scrollToCurrectItem();

		visibleQMPlay2Extensions.clear();

		QMPlay2GUI.screenSaver->unInhibit(1);
	}
	QMPlay2Core.fullScreenChanged(fullScreen);
}
void MainWidget::showMessage(const QString &msg, const QString &title, int messageIcon, int ms)
{
	if (ms < 1)
		QMessageBox((QMessageBox::Icon)messageIcon, title, msg, QMessageBox::Ok, this).exec();
	else
		tray->showMessage(title, msg, (QSystemTrayIcon::MessageIcon)messageIcon, ms);
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
	foreach (const QString &e, Playlist::extensions())
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
		settingsW = new SettingsWidget(sender() == menuBar->playback->playbackSettings ? 1 : ((sender() == menuBar->options->modulesSettings || !moduleName.isEmpty()) ? 2 : (sender() == menuBar->playback->videoFilters->more ? 5 : 0)), moduleName, QMPlay2GUI.videoAdjustment);
		connect(settingsW, SIGNAL(settingsChanged(int, bool)), &playC, SLOT(settingsChanged(int, bool)));
		connect(settingsW, SIGNAL(setWheelStep(int)), seekS, SLOT(setWheelStep(int)));
		connect(settingsW, SIGNAL(setVolMax(int)), volW, SLOT(setMaximumVolume(int)));
		connect(settingsW, SIGNAL(destroyed()), this, SLOT(showSettings()));
	}
	else
	{
		bool showAgain = sender() != settingsW;
		if (showAgain)
			disconnect(settingsW, SIGNAL(destroyed()), this, SLOT(showSettings()));
		settingsW->close();
		settingsW = NULL;
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
	foreach (const QString &ext, SubsDec::extensions())
		filter += ";;" + tr("Subtitles") + " " + ext.toUpper() + " (*." + ext + ")";

	QString f = QFileDialog::getOpenFileName(this, tr("Choose subtitles"), dir, filter);
	if (!f.isEmpty())
		playC.loadSubsFile(Functions::Url(f));
}

void MainWidget::setSeekSMaximum(int max)
{
	seekS->setMaximum(qMax(0, max));
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
void MainWidget::updatePos(int pos)
{
	const QString remainingTime = (seekS->maximum() - pos > -1) ? timeToStr(seekS->maximum() - pos) : timeToStr(0);
	timeL->setText(timeToStr(pos) + " ; -" + remainingTime + " / " + timeToStr(seekS->maximum()));
	emit QMPlay2Core.posChanged(pos);
	seekS->setValue(pos);
}
void MainWidget::mousePositionOnSlider(int pos)
{
	statusBar->showMessage(tr("Pointed position") + ": " + timeToStr(pos), 750);
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
	foreach (const QByteArray &arr, socket->readAll().split('\0'))
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
		connect(aboutW, SIGNAL(destroyed()), this, SLOT(about()));
	}
	else
	{
		aboutW->close();
		aboutW = NULL;
	}
}

#if !defined Q_OS_MAC && !defined Q_OS_ANDROID
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
		foreach (QObject *o, children())
		{
			DockWidget *dW = dynamic_cast<DockWidget *>(o);
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

void MainWidget::hideDocksSlot()
{
	if ((fullScreen || isCompactView) && !geometry().contains(QCursor::pos()))
	{
		if (!playlistDock->isVisible())
			showToolBar(false);
		else
			hideDocks();
	}
}
void MainWidget::delayedRestore(QByteArray data)
{
	QCoreApplication::processEvents();
	restoreState(data);
	restoreFocus();
}

void MainWidget::uncheckSuspend()
{
	if (menuBar->player->suspend)
		menuBar->player->suspend->setChecked(false);
}

void MainWidget::savePlistHelper(const QString &title, const QString &fPth, bool saveCurrentGroup)
{
	QString filter;
	foreach (const QString &e, Playlist::extensions())
		filter += e.toUpper() + " (*." + e + ");;";
	filter.chop(2);
	if (filter.isEmpty())
		return;
	QString selectedFilter;
	QString plistFile = QFileDialog::getSaveFileName(this, title, fPth, filter, &selectedFilter);
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
#if !defined Q_OS_MAC && !defined Q_OS_ANDROID
		popupMenu->insertAction(popupMenu->actions().value(0), hideMenuAct);
		popupMenu->insertSeparator(popupMenu->actions().value(1));
		popupMenu->addSeparator();
#endif
		popupMenu->addAction(lockWidgetsAct);
	}
	foreach (QAction *act, popupMenu->actions())
		act->setEnabled(isVisible() && !fullScreen && !isCompactView);
	return popupMenu;
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
	fullScreenDockWidgetState = saveState();
	showToolBar(false);
	playlistDock->hide();
	infoDock->hide();
	hideAllExtensions();
}
void MainWidget::restoreFocus()
{
	if (lastFocusWidget)
	{
		if (lastFocusWidget != focusWidget())
			lastFocusWidget->setFocus();
		lastFocusWidget = NULL;
	}
}

bool MainWidget::getFullScreen() const
{
	return fullScreen;
}

void MainWidget::keyPressEvent(QKeyEvent *e)
{
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
	if ((fullScreen || isCompactView) && (e->buttons() == Qt::NoButton || videoDock->isTouch))
	{
		const int trigger1 = qMax<int>( 5, ceil(0.003 * (videoDock->isTouch ? 8 : 1) * width()));
		const int trigger2 = qMax<int>(15, ceil(0.025 * (videoDock->isTouch ? 4 : 1) * width()));
		if (videoDock->touchEnded)
			videoDock->isTouch = videoDock->touchEnded = false;

		int mPosX = 0;
		if (videoDock->x() >= 0)
			mPosX = videoDock->mapFromGlobal(e->globalPos()).x();

		/* ToolBar */
		if (!playlistDock->isVisible() && mPosX > trigger1)
			showToolBar(e->pos().y() >= height() - mainTB->height() - statusBar->height() + 10);

		/* DockWidgets */
		if (fullScreen && !playlistDock->isVisible() && mPosX <= trigger1)
		{
			showToolBar(true); //Before restoring dock widgets - show toolbar and status bar
			restoreState(fullScreenDockWidgetState);

			const QList<QDockWidget *> tDW = tabifiedDockWidgets(infoDock);
			bool reloadQMPlay2Extensions = false;
			foreach (QMPlay2Extensions *QMPlay2Ext, QMPlay2Extensions::QMPlay2ExtensionsList())
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

				foreach (QMPlay2Extensions *QMPlay2Ext, visibleQMPlay2Extensions)
					if (!QMPlay2Ext->isVisualization())
						if (DockWidget *dw = QMPlay2Ext->getDockWidget())
						{
							tabifyDockWidget(infoDock, dw);
							dw->show();
						}
			}
			showToolBar(true); //Ensures that status bar is visible
		}
		else if (fullScreen && playlistDock->isVisible() && mPosX > trigger2)
			hideDocks();
	}
	QMainWindow::mouseMoveEvent(e);
}
void MainWidget::leaveEvent(QEvent *e)
{
	if (fullScreen || isCompactView)
		QMetaObject::invokeMethod(this, "hideDocksSlot", Qt::QueuedConnection); //Qt5 can't hide docks properly here
	QMainWindow::leaveEvent(e);
}
void MainWidget::closeEvent(QCloseEvent *e)
{
	const QString quitMsg = tr("Are you sure you want to quit?");
	if
	(
		(QMPlay2Core.isWorking() && QMessageBox::question(this, QString(), tr("QMPlay2 is doing something in the background.") + " " + quitMsg, QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
#ifdef UPDATER
		|| (updater.downloading() && QMessageBox::question(this, QString(), tr("The update is downloading now.") + " " + quitMsg, QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
#endif
	)
	{
		QMPlay2GUI.restartApp = QMPlay2GUI.removeSettings = false;
		e->ignore();
		return;
	}

	emit QMPlay2Core.waitCursor();

	if (QMPlay2GUI.pipe)
		disconnect(QMPlay2GUI.pipe, SIGNAL(newConnection(IPCSocket *)), this, SLOT(newConnection(IPCSocket *)));

	Settings &settings = QMPlay2Core.getSettings();

	if (!fullScreen && !isCompactView)
		settings.set("MainWidget/DockWidgetState", saveState());
	else
		settings.set("MainWidget/DockWidgetState", dockWidgetState);
	settings.set("MainWidget/FullScreenDockWidgetState", fullScreenDockWidgetState);
#ifndef Q_OS_MAC
	settings.set("MainWidget/isVisible", isVisible());
#endif
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
		settings.set("Pos", seekS->value());
		settings.set("Url", playC.getUrl());
	}
	else
	{
		settings.remove("Pos");
		settings.remove("Url");
	}

	playlistDock->stopThreads();
	playlistDock->save(QMPlay2Core.getSettingsDir() + "Playlist.pls");

	playC.stop(true);
}
void MainWidget::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::WindowStateChange)
		emit QMPlay2Core.mainWidgetNotMinimized(!windowState().testFlag(Qt::WindowMinimized)); //Mainly for workaround the QTBUG-50589
	QMainWindow::changeEvent(e);
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
		QMPlay2GUI.restoreGeometry("MainWidget/Geometry", this, size());
		savedGeo = geometry();
		if (QMPlay2Core.getSettings().getBool("MainWidget/isMaximized"))
#endif
			showMaximized();
		const QByteArray restoreData = QMPlay2Core.getSettings().getByteArray("MainWidget/DockWidgetState");
		restoreState(restoreData);
		if (isMaximized())
			QMetaObject::invokeMethod(this, "delayedRestore", Qt::QueuedConnection, Q_ARG(QByteArray, restoreData));
		wasShow = true;
	}
	menuBar->window->toggleVisibility->setText(tr("&Hide"));
}
void MainWidget::hideEvent(QHideEvent *)
{
#ifndef Q_OS_ANDROID
	if (wasShow)
	{
		QMPlay2Core.getSettings().set("MainWidget/Geometry", (!fullScreen && !isMaximized()) ? geometry() : savedGeo);
		QMPlay2Core.getSettings().set("MainWidget/isMaximized", isMaximized());
	}
#endif
	menuBar->window->toggleVisibility->setText(tr("&Show"));
}
