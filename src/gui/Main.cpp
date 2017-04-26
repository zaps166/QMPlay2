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

#include <Main.hpp>

#include <ScreenSaver.hpp>
#include <VideoFrame.hpp>
#include <MainWidget.hpp>
#include <PlayClass.hpp>
#include <Functions.hpp>
#include <VideoDock.hpp>
#include <Settings.hpp>
#include <Version.hpp>
#include <Module.hpp>
#include <IPC.hpp>

#include <QDesktopWidget>
#include <QApplication>
#include <QImageReader>
#include <QTreeWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QPainter>
#include <QBuffer>
#include <QFile>
#include <QDir>
#if QT_VERSION < 0x050000
	#include <QTextCodec>
#endif
#ifdef Q_OS_MAC
	#include <QProcess>
#endif

#include <csignal>
#include <ctime>

static QPair<QStringList, QStringList> g_arguments;
static ScreenSaver *g_screenSaver = nullptr;
static bool g_useGui = true;
#ifdef Q_OS_MAC
	static QByteArray g_rcdPath("/System/Library/LaunchAgents/com.apple.rcd.plist");
	static bool g_rcdLoad;
#endif

/**/

QMPlay2GUIClass &QMPlay2GUIClass::instance()
{
	static QMPlay2GUIClass qmplay2Gui;
	return qmplay2Gui;
}

QString QMPlay2GUIClass::getPipe()
{
#if defined Q_OS_WIN
	return "\\\\.\\pipe\\QMPlay2";
#elif defined Q_OS_MAC
	return "/tmp/QMPlay2." + QString(getenv("USER"));
#else
	return QDir::tempPath() + "/QMPlay2." + QString(getenv("USER"));
#endif
}
void QMPlay2GUIClass::saveCover(QByteArray cover)
{
	QBuffer buffer(&cover);
	if (buffer.open(QBuffer::ReadOnly))
	{
		const QString fileExtension = QImageReader::imageFormat(&buffer);
		buffer.close();
		if (!fileExtension.isEmpty())
		{
			QString fileName = QFileDialog::getSaveFileName(QMPlay2GUI.mainW, tr("Saving cover picture"), QMPlay2GUI.getCurrentPth(), fileExtension.toUpper() + " (*." + fileExtension + ')');
			if (!fileName.isEmpty())
			{
				if (!fileName.endsWith('.' + fileExtension))
					fileName += '.' + fileExtension;
				QFile f(fileName);
				if (f.open(QFile::WriteOnly))
				{
					f.write(cover);
					QMPlay2GUI.setCurrentPth(Functions::filePath(fileName));
				}
			}
		}
	}
}

void QMPlay2GUIClass::setTreeWidgetItemIcon(QTreeWidgetItem *tWI, const QIcon &icon, const int column, QTreeWidget *treeWidget)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
	bool setDefaultIcon = icon.name().isEmpty();
	if (!setDefaultIcon)
	{
		// Icons from theme may not be in correct size in tree widget, so do a workaround.
		if (!treeWidget)
			treeWidget = tWI->treeWidget();
		if (treeWidget)
			tWI->setData(column, Qt::DecorationRole, icon.pixmap(treeWidget->window()->windowHandle(), treeWidget->iconSize()));
		else
			setDefaultIcon = true;
	}
	if (setDefaultIcon)
#else
	Q_UNUSED(treeWidget)
#endif
	{
		tWI->setIcon(column, icon);
	}
}

#ifdef UPDATER
void QMPlay2GUIClass::runUpdate(const QString &UpdateFile)
{
	settings->set("UpdateFile", QString("remove:" + UpdateFile));
	ShellExecuteW(nullptr, L"open", (const wchar_t *)UpdateFile.utf16(), L"--Auto", nullptr, SW_SHOWNORMAL);
}
#endif

void QMPlay2GUIClass::setStyle()
{
	QString defaultStyle;
#if defined Q_OS_ANDROID
	defaultStyle = "fusion"; //Android style is awful in Qt (tested on Qt 5.4 and Qt 5.5)
#endif
	QApplication::setStyle(settings->getString("Style", defaultStyle));
}

void QMPlay2GUIClass::loadIcons()
{
	deleteIcons();

	// QtSvg doesn't support effects. This workaround generates drop shadow effect similar to
	// drop shadow in SVG file. Remove / modify this workaround if QMPlay2 SVG icon changes!
	const QSize size(128, 128);
	const QPixmap pixmap = QIcon(":/QMPlay2.svgz").pixmap(size);
	const qreal sizeRatio = (((qreal)pixmap.size().width()  / (qreal)size.width()) + ((qreal)pixmap.size().height() / (qreal)size.height())) / 2.0;
	qmplay2Icon = new QIcon(Functions::applyDropShadow(pixmap, 10.0 * sizeRatio, QPointF(3.0 * sizeRatio, 3.0 * sizeRatio), QColor(0, 0, 0, 127)));

	groupIcon = new QIcon(getIconFromTheme("folder-orange"));
	mediaIcon = new QIcon(getIconFromTheme("applications-multimedia"));
	folderIcon = new QIcon(getIconFromTheme("folder"));
}
void QMPlay2GUIClass::deleteIcons()
{
	delete qmplay2Icon;
	delete groupIcon;
	delete mediaIcon;
	delete folderIcon;
	qmplay2Icon = groupIcon = mediaIcon = folderIcon = nullptr;
}

QString QMPlay2GUIClass::getCurrentPth(QString pth, bool leaveFilename)
{
	if (pth.startsWith("file://"))
		pth.remove(0, 7);
	if (!leaveFilename)
		pth = Functions::filePath(pth);
	if (!QFileInfo::exists(pth))
		pth = settings->getString("currPth");
	return pth;
}
void QMPlay2GUIClass::setCurrentPth(const QString &pth)
{
	settings->set("currPth", Functions::cleanPath(pth));
}

void QMPlay2GUIClass::restoreGeometry(const QString &pth, QWidget *w, const int defaultSizePercent)
{
	const QRect geo = settings->getRect(pth);
	QDesktopWidget *desktop = QApplication::desktop();
	if (geo.isValid())
	{
		w->setGeometry(geo);
		if (!desktop->screenGeometry(w).contains(w->pos()))
			w->move(desktop->width() / 2 - w->width() / 2, desktop->height() / 2 - w->height() / 2);
	}
	else
	{
		const QSize defaultSize = desktop->availableGeometry(w).size() * defaultSizePercent / 100;
		w->move(desktop->width() / 2 - defaultSize.width() / 2, desktop->height() / 2 - defaultSize.height() / 2);
		w->resize(defaultSize);
	}
}

void QMPlay2GUIClass::updateInDockW()
{
	qobject_cast<MainWidget *>(mainW)->videoDock->updateIDW();
}

const QWidget *QMPlay2GUIClass::getVideoDock() const
{
	return qobject_cast<MainWidget *>(mainW)->videoDock;
}

QMPlay2GUIClass::QMPlay2GUIClass() :
	groupIcon(nullptr), mediaIcon(nullptr), folderIcon(nullptr),
	mainW(nullptr),
	screenSaver(nullptr),
	shortcutHandler(nullptr)
{}
QMPlay2GUIClass::~QMPlay2GUIClass()
{}

/**/

static QString fileArg(const QString &arg)
{
	if (!arg.contains("://"))
	{
		const QFileInfo argInfo(arg);
		if (!argInfo.isAbsolute())
			return argInfo.absoluteFilePath();
	}
	return arg;
}
static void parseArguments(QStringList &arguments)
{
	QString param;
	while (arguments.count())
	{
		const QString arg = arguments.takeFirst();
		if (arg.startsWith('-'))
		{
			param = arg;
			while (param.startsWith('-'))
				param.remove(0, 1);
			if (!param.isEmpty() && !g_arguments.first.contains(param))
			{
				g_arguments.first  += param;
				g_arguments.second += QString();
			}
			else
			{
				param.clear();
			}
		}
		else if (!param.isEmpty())
		{
			QString &data = g_arguments.second.last();
			if (!data.isEmpty())
				data += '\n';
			if (param == "open" || param == "enqueue")
				data += fileArg(arg);
			else
				data += arg;
		}
		else if (!g_arguments.first.contains("open"))
		{
			param = "open";
			g_arguments.first  += param;
			g_arguments.second += fileArg(arg);
		}
	}
}
static void showHelp()
{
	QFile f;
	f.open(stdout, QFile::WriteOnly);
	f.write("QMPlay2 - Qt Media Player 2 (" + Version::get() + ")\n");
	f.write(QObject::tr(
"  Parameters list:\n"
"    -open         \"address\"\n"
"    -enqueue      \"address\"\n"
"    -profile      \"name\" - starts application with given profile name\n"
"    -noplay     - doesn't play after run (bypass \"Remember playback position\" option)\n"
"    -toggle     - toggles play/pause\n"
"    -show       - ensures that the window will be visible if the application is running\n"
"    -fullscreen - toggles fullscreen\n"
"    -volume     - sets volume [0..100]\n"
"    -speed      - sets playback speed [0.05..100.0]\n"
"    -seek       - seeks to the specified value [s]\n"
"    -stop       - stops playback\n"
"    -next       - plays next on the list\n"
"    -prev       - plays previous on the list\n"
"    -quit       - terminates the application"
	).toLocal8Bit() + "\n");
}
static bool writeToSocket(IPCSocket &socket)
{
	bool ret = false;
	for (int i = g_arguments.first.count() - 1; i >= 0; i--)
	{
		if (g_arguments.first[i] == "noplay" || g_arguments.first[i] == "profile")
			continue;
		else if (g_arguments.first[i] == "open" || g_arguments.first[i] == "enqueue")
		{
			if (!g_arguments.second[i].isEmpty())
				g_arguments.second[i] = Functions::Url(g_arguments.second[i]);
#ifdef Q_OS_WIN
			if (g_arguments.second[i].startsWith("file://"))
				g_arguments.second[i].remove(0, 7);
#endif
		}
		socket.write(QString(g_arguments.first[i] + '\t' + g_arguments.second[i]).toUtf8() + '\0');
		ret = true;
	}
	return ret;
}

static inline void exitProcedure()
{
#ifdef Q_OS_MAC
	if (g_rcdLoad)
	{
		// Load RCD service again (allow to run iTunes on "Play" key)
		QProcess::startDetached("launchctl load " + g_rcdPath);
		g_rcdLoad = false;
	}
#endif

	delete g_screenSaver;
	g_screenSaver = nullptr;
}

#if QT_VERSION >= 0x050000 && !defined Q_OS_WIN
	#define QT5_NOT_WIN
	#include <csetjmp>
	static jmp_buf env;
	static bool qAppOK;
	static bool canDeleteApp = true;
#endif

static inline void forceKill()
{
#ifdef Q_OS_WIN
	const int s = SIGBREAK;
#else
	const int s = SIGKILL;
#endif
	exitProcedure();
	raise(s);
}
static inline void callDefaultSignalHandler(int s)
{
	exitProcedure();
	signal(s, SIG_DFL);
	raise(s);
}
static void signal_handler(int s)
{
	const char *crashSignal = nullptr;
	switch (s)
	{
		case SIGINT:
		case SIGTERM:
			if (!qApp)
				forceKill();
			else
			{
				static bool first = true;
				QWidget *modalW = QApplication::activeModalWidget();
				if (!modalW && QMPlay2GUI.mainW && first)
				{
					QMPlay2GUI.mainW->close();
					first = false;
				}
				else
				{
					forceKill();
				}
			}
			break;
		case SIGABRT:
#ifdef QT5_NOT_WIN
			if (!qAppOK && g_useGui)
			{
				canDeleteApp = g_useGui = false;
				longjmp(env, 1);
			}
#endif
			QMPlay2Core.log("QMPlay2 has been aborted (SIGABRT)", ErrorLog | AddTimeToLog | (qApp ? SaveLog : DontShowInGUI));
			callDefaultSignalHandler(s);
			break;
		case SIGILL:
			crashSignal = "SIGILL";
			break;
		case SIGFPE:
			crashSignal = "SIGFPE";
			break;
#ifndef Q_OS_WIN
		case SIGBUS:
			crashSignal = "SIGBUS";
			break;
#endif
		case SIGSEGV:
			crashSignal = "SIGSEGV";
			break;
	}
	if (crashSignal)
	{
		QMPlay2Core.log(QString("QMPlay2 crashed (%1)").arg(crashSignal), ErrorLog | AddTimeToLog | (qApp ? SaveLog : DontShowInGUI));
		callDefaultSignalHandler(s);
	}
}

static inline void noAutoPlay()
{
	g_arguments.first += "noplay";
	g_arguments.second += QString();
}

#ifdef Q_OS_WIN
static LRESULT CALLBACK MMKeysHookProc(int code, WPARAM wparam, LPARAM lparam)
{
	if (code == HC_ACTION && wparam == WM_KEYDOWN)
	{
		PKBDLLHOOKSTRUCT kbd = (PKBDLLHOOKSTRUCT)lparam;
		if (kbd)
		{
			switch (kbd->vkCode)
			{
				case VK_MEDIA_NEXT_TRACK:
					emit QMPlay2Core.processParam("next");
					return 1;
				case VK_MEDIA_PREV_TRACK:
					emit QMPlay2Core.processParam("prev");
					return 1;
				case VK_MEDIA_STOP:
					emit QMPlay2Core.processParam("stop");
					return 1;
				case VK_MEDIA_PLAY_PAUSE:
					emit QMPlay2Core.processParam("toggle");
					return 1;
			}
		}
	}
	return CallNextHookEx(nullptr, code, wparam, lparam);
}
#endif

int main(int argc, char *argv[])
{
	signal(SIGINT, signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGFPE, signal_handler);
#ifndef Q_OS_WIN
	signal(SIGBUS, signal_handler);
#endif
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
	atexit(exitProcedure);

#if defined(Q_OS_MAC) && (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
	QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#endif

#ifdef Q_WS_X11
	g_useGui = getenv("DISPLAY");
#endif
#ifdef QT5_NOT_WIN
	if (!setjmp(env))
#endif
#if QT_VERSION < 0x050000
		new QApplication(argc, argv, g_useGui);
#else
		new QApplication(argc, argv);
#endif
#ifdef QT5_NOT_WIN
	qAppOK = true;
#endif
#if QT_VERSION < 0x050000
	QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
#endif
	QCoreApplication::setApplicationName("QMPlay2");

	QMPlay2GUIClass &qmplay2Gui = QMPlay2GUI; //Create "QMPlay2GUI" instance

	QStringList arguments = QCoreApplication::arguments();
	arguments.removeFirst();
	const bool help = arguments.contains("-help") || arguments.contains("-h");
	if (!help)
	{
		IPCSocket socket(qmplay2Gui.getPipe());
		parseArguments(arguments);
		if (socket.open(IPCSocket::WriteOnly))
		{
			if (writeToSocket(socket))
				g_useGui = false;
			socket.close();
		}
#ifndef Q_OS_WIN
		else if (QFile::exists(qmplay2Gui.getPipe()))
		{
			QFile::remove(qmplay2Gui.getPipe());
			noAutoPlay();
		}
#endif

		if (!g_useGui)
		{
#ifdef QT5_NOT_WIN
			if (canDeleteApp)
#endif
				delete qApp;
			return 0;
		}
	}

	for (int i = 0; i < g_arguments.first.count(); ++i)
	{
		if (g_arguments.first.at(i) == "profile")
		{
			qmplay2Gui.cmdLineProfile = g_arguments.second.at(i);
			g_arguments.first.removeAt(i);
			g_arguments.second.removeAt(i);
			break;
		}
	}

	QString libPath, sharePath = QCoreApplication::applicationDirPath();
	bool cmakeBuildFound = false;
#ifdef Q_OS_MAC
	if (QDir(sharePath).exists("../../../CMakeFiles/QMPlay2.dir"))
		sharePath += "/../../.."; //Probably CMake not-installed build in Bundle
#endif
	if (QDir(sharePath).exists("CMakeFiles/QMPlay2.dir"))
	{
		//We're in CMake not-installed build
		libPath = sharePath + "/.."; //Here is "modules" directory
		sharePath += "/../.."; //Here is "lang" directory
		if (QDir(libPath).exists("modules") || QDir(sharePath).exists("lang"))
			cmakeBuildFound = true;
	}
	if (!cmakeBuildFound)
	{
#if !defined Q_OS_WIN && !defined Q_OS_MAC && !defined Q_OS_ANDROID
		sharePath = QCoreApplication::applicationDirPath() + "/../share/qmplay2";
		libPath = QMPlay2CoreClass::getLibDir();
		if (libPath.isEmpty() || !QDir(libPath).exists("qmplay2"))
		{
			libPath += "/../";
			if (sizeof(void *) == 8 && QDir(libPath).exists("lib64/qmplay2"))
				libPath += "lib64";
			else if (sizeof(void *) == 4 && QDir(libPath).exists("lib32/qmplay2"))
				libPath += "lib32";
			else
				libPath += "lib";
		}
		libPath += "/qmplay2";
#elif defined Q_OS_MAC
		libPath = QCoreApplication::applicationDirPath();
		sharePath += "/../share/qmplay2";
#else
		libPath = sharePath;
#endif
	}

	qRegisterMetaType<VideoFrame>("VideoFrame");

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
	QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
	QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

	QDir::setCurrent(QCoreApplication::applicationDirPath()); //Is it really needed?

	if (g_useGui)
	{
		qmplay2Gui.screenSaver = g_screenSaver = new ScreenSaver;
		QApplication::setQuitOnLastWindowClosed(false);
	}

#ifdef Q_OS_WIN
	HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, MMKeysHookProc, GetModuleHandle(nullptr), 0);
#endif

#ifdef Q_OS_MAC
	// Unload RCD service (prevent run iTunes on "Play" key)
	{
		QProcess launchctl;
		launchctl.start("launchctl unload " + g_rcdPath);
		if (launchctl.waitForFinished() && launchctl.exitStatus() == QProcess::NormalExit)
			g_rcdLoad = !launchctl.readAllStandardError().startsWith(g_rcdPath);
	}
#endif

	qsrand(time(nullptr));

	do
	{
		/* QMPlay2GUI musi być stworzone już wcześniej */
		QMPlay2Core.init(!help, cmakeBuildFound, libPath, sharePath, qmplay2Gui.cmdLineProfile);

		if (!qmplay2Gui.cmdLineProfile.isEmpty() && QMPlay2Core.getSettingsProfile() == "/")
			qmplay2Gui.cmdLineProfile = QMPlay2Core.getSettingsProfile(); // Default profile

		Settings &settings = QMPlay2Core.getSettings();
		QByteArray lastVer = settings.getByteArray("Version", Version::get());
		settings.set("Version", Version::get());
		settings.set("LastQMPlay2Path", QCoreApplication::applicationDirPath());

		if (Functions::parseVersion(lastVer) < QDate(2015, 10, 9))
		{
			QFile::remove(QMPlay2Core.getSettingsDir() + "OpenGL.ini");
			settings.remove("audioWriters");
			settings.remove("videoWriters");
		}
		if (settings.contains("Volume"))
		{
			const int vol = settings.getInt("Volume");
			settings.set("VolumeL", vol);
			settings.set("VolumeR", vol);
			settings.remove("Volume");
		}

		if (help)
		{
			showHelp();
			break;
		}

		qmplay2Gui.loadIcons();
		{
			const QIcon scaledIcon = QMPlay2Core.getQMPlay2Icon();
			const QIcon    svgIcon = QIcon(":/QMPlay2.svgz");
			if (scaledIcon.isNull() && !svgIcon.isNull())
				QMessageBox::critical(nullptr, QString(), QObject::tr("QtSvg module doesn't exist.\nQMPlay2 will not display icons!"));
			else if (!svgIcon.availableSizes().isEmpty())
				QMessageBox::warning(nullptr, QString(), QObject::tr("QtSvg icon engine plugin doesn't exist.\nQMPlay2 will not scale up icons!"));
		}

		qmplay2Gui.pipe = new IPCServer(qmplay2Gui.getPipe());
		if
		(
#ifdef Q_OS_WIN
			WaitNamedPipeW((const wchar_t *)qmplay2Gui.getPipe().utf16(), NMPWAIT_USE_DEFAULT_WAIT) ||
#endif
			!qmplay2Gui.pipe->listen()
		)
		{
			delete qmplay2Gui.pipe;
			qmplay2Gui.pipe = nullptr;
#ifdef QMPLAY2_ALLOW_ONLY_ONE_INSTANCE
			if (settings.getBool("AllowOnlyOneInstance"))
			{
				QMPlay2Core.quit();
				IPCSocket socket(qmplay2Gui.getPipe());
				if (socket.open(IPCSocket::WriteOnly))
				{
					socket.write(QByteArray("show\t") + '\0');
					socket.close();
				}
				break;
			}
#endif
		}

		QApplication::setWindowIcon(QMPlay2Core.getIconFromTheme("QMPlay2", QMPlay2Core.getQMPlay2Icon()));
		qmplay2Gui.setStyle();

#ifdef UPDATER
		QString UpdateFile = settings.getString("UpdateFile");
		if (UpdateFile.startsWith("remove:"))
		{
			UpdateFile.remove(0, 7);
			if (lastVer != Version::get())
			{
				const QString updateString = QObject::tr("QMPlay2 has been updated to version") + " " + Version::get();
				QMPlay2Core.logInfo(updateString);
				QMessageBox::information(nullptr, QCoreApplication::applicationName(), updateString);
				settings.remove("UpdateVersion");
			}
			else
			{
				const QString message = QObject::tr("QMPlay2 hasn't been updated. Do you want to run the update (recommended)?");
				if (QMessageBox::question(nullptr, QCoreApplication::applicationName(), message, QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
				{
					qmplay2Gui.runUpdate(UpdateFile);
					QMPlay2Core.quit();
					break;
				}
			}
			QFile::remove(UpdateFile);
			settings.remove("UpdateFile");
			settings.flush();
		}
		else if (QFileInfo(UpdateFile).size())
		{
			qmplay2Gui.runUpdate(UpdateFile);
			QMPlay2Core.quit();
			break;
		}
		UpdateFile.clear();
#endif

		lastVer.clear();

		qmplay2Gui.restartApp = qmplay2Gui.removeSettings = qmplay2Gui.noAutoPlay = false;
		qmplay2Gui.newProfileName.clear();
		new MainWidget(g_arguments);
		do
		{
			QCoreApplication::exec();
			// Go back to event queue and allow to close the main window (and playback) if
			// QCoreApplication exits too early. Reproducible on macOS when closing from dock.
		} while (qmplay2Gui.mainW);

		const QString settingsDir = QMPlay2Core.getSettingsDir();
		const QString profile = QMPlay2Core.getSettingsProfile();

		QMPlay2Core.quit();

		if (qmplay2Gui.removeSettings)
		{
			const QString settingsDirProfile = settingsDir + profile;
			for (const QString &fName : QDir(settingsDirProfile).entryList({"*.ini"}))
				QFile::remove(settingsDirProfile + fName);
			if (profile != "/")
				QDir(settingsDir).rmdir(profile);
		}
		else if (!qmplay2Gui.newProfileName.isEmpty())
		{
			const QString srcDir = settingsDir + profile;
			const QString dstDir = settingsDir + "Profiles/" + qmplay2Gui.newProfileName + "/";
			for (const QString &fName : QDir(srcDir).entryList({"*.ini"}, QDir::Files))
			{
				if (fName != "Profile.ini")
					QFile::copy(srcDir + fName, dstDir + fName);
			}
		}

		if (qmplay2Gui.noAutoPlay)
			noAutoPlay();

		delete qmplay2Gui.pipe;
	} while (qmplay2Gui.restartApp);

	qmplay2Gui.deleteIcons();

#ifdef Q_OS_WIN
	UnhookWindowsHookEx(keyboardHook);
#endif

	exitProcedure();

#ifdef QT5_NOT_WIN
	if (canDeleteApp)
#endif
		delete qApp;
	return 0;
}
