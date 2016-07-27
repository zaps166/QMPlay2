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

#include <Main.hpp>

#include <VideoFrame.hpp>
#include <MainWidget.hpp>
#include <PlayClass.hpp>
#include <Functions.hpp>
#include <VideoDock.hpp>
#include <Settings.hpp>
#include <Version.hpp>
#include <Module.hpp>

#include <QDesktopWidget>
#include <QStyleFactory>
#include <QStyleOption>
#include <QLocalServer>
#include <QLocalSocket>
#include <QApplication>
#include <QImageReader>
#include <QMessageBox>
#include <QFileDialog>
#include <QPainter>
#include <QBuffer>
#include <QFile>
#include <QDir>
#if QT_VERSION < 0x050000
	#include <QTextCodec>
#endif

#include <time.h>

static bool useGui = true;

QMPlay2GUIClass &QMPlay2GUIClass::instance()
{
	static QMPlay2GUIClass qmplay2Gui;
	return qmplay2Gui;
}

QString QMPlay2GUIClass::getPipe()
{
#ifdef Q_OS_WIN
	return "\\\\.\\pipe\\QMPlay2";
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
void QMPlay2GUIClass::drawPixmap(QPainter &p, QWidget *w, QPixmap pixmap)
{
	if (pixmap.width() > w->width() || pixmap.height() > w->height())
		pixmap = pixmap.scaled(w->width(), w->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	if (!w->isEnabled())
	{
		QStyleOption opt;
		opt.initFrom(w);
		pixmap = w->style()->generatedIconPixmap(QIcon::Disabled, pixmap, &opt);
	}
	p.drawPixmap(w->width() / 2 - pixmap.width() / 2, w->height() / 2 - pixmap.height() / 2, pixmap);
}

#ifdef UPDATER
void QMPlay2GUIClass::runUpdate(const QString &UpdateFile)
{
	settings->set("UpdateFile", "remove:" + UpdateFile);
	QMPlay2Core.run(UpdateFile, "--Auto");
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
	groupIcon = new QIcon(getIconFromTheme("folder-orange"));
	mediaIcon = new QIcon(getIconFromTheme("applications-multimedia"));
	folderIcon = new QIcon(getIconFromTheme("folder"));
}

QString QMPlay2GUIClass::getCurrentPth(QString pth, bool leaveFilename)
{
	if (pth.startsWith("file://"))
		pth.remove(0, 7);
	if (!leaveFilename)
		pth = Functions::filePath(pth);
	if (!QFileInfo(pth).exists())
		pth = settings->getString("currPth");
	return pth;
}
void QMPlay2GUIClass::setCurrentPth(const QString &pth)
{
	settings->set("currPth", Functions::cleanPath(pth));
}

void QMPlay2GUIClass::restoreGeometry(const QString &pth, QWidget *w, const QSize &def_size)
{
	QRect geo = settings->get(pth).toRect();
	if (geo.isValid())
		w->setGeometry(geo);
	else
	{
		w->move(QApplication::desktop()->width()/2 - def_size.width()/2, QApplication::desktop()->height()/2 - def_size.height()/2);
		w->resize(def_size);
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
	groupIcon(NULL), mediaIcon(NULL), folderIcon(NULL),
	mainW(NULL)
{
	qmp2Pixmap = useGui ? new QPixmap(":/QMPlay2") : NULL;
}
QMPlay2GUIClass::~QMPlay2GUIClass()
{
	if (useGui)
	{
		delete qmp2Pixmap;
		deleteIcons();
	}
}

void QMPlay2GUIClass::deleteIcons()
{
	delete groupIcon;
	delete mediaIcon;
	delete folderIcon;
	groupIcon = mediaIcon = folderIcon = NULL;
}

/**/

static QPair<QStringList, QStringList> QMPArguments;

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
		QString arg = arguments.takeFirst();
		if (arg.startsWith('-'))
		{
			param = arg;
			while (param.startsWith('-'))
				param.remove(0, 1);
			if (!param.isEmpty() && !QMPArguments.first.contains(param))
			{
				QMPArguments.first  += param;
				QMPArguments.second += QString();
			}
			else
				param.clear();
		}
		else if (!param.isEmpty())
		{
			QString &data = QMPArguments.second.last();
			if (!data.isEmpty())
				data += '\n';
			if (param == "open" || param == "enqueue")
				data += fileArg(arg);
			else
				data += arg;
		}
		else if (!QMPArguments.first.contains("open"))
		{
			param = "open";
			QMPArguments.first  += param;
			QMPArguments.second += fileArg(arg);
		}
	}
}
static void showHelp(const QByteArray &ver)
{
	QFile f;
	f.open(stdout, QFile::WriteOnly);
	f.write("QMPlay2 - Qt Media Player 2 (" + ver + ")\n");
	f.write(QObject::tr(
"  Parameters list:\n"
"    -open         \"address\"\n"
"    -enqueue      \"address\"\n"
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
	).toUtf8() + "\n");
}
static bool writeToSocket(QLocalSocket &socket)
{
	bool ret = false;
	for (int i = QMPArguments.first.count() - 1; i >= 0; i--)
	{
		if (QMPArguments.first[i] == "noplay")
			continue;
		else if (QMPArguments.first[i] == "open" || QMPArguments.first[i] == "enqueue")
		{
			if (!QMPArguments.second[i].isEmpty())
				QMPArguments.second[i] = Functions::Url(QMPArguments.second[i]);
#ifdef Q_OS_WIN
			if (QMPArguments.second[i].startsWith("file://"))
				QMPArguments.second[i].remove(0, 7);
#endif
		}
		socket.write(QString(QMPArguments.first[i] + '\t' + QMPArguments.second[i]).toUtf8() + '\0');
		ret = true;
	}
	return ret;
}

#if QT_VERSION >= 0x050000 && !defined Q_OS_WIN
	#define QT5_NOT_WIN
	#include <setjmp.h>
	static jmp_buf env;
	static bool qAppOK;
	static bool canDeleteApp = true;
#endif
#include <signal.h>
static void signal_handler(int s)
{
#ifdef Q_OS_WIN
	const int SC = SIGBREAK;
#else
	const int SC = SIGKILL;
#endif
	switch (s)
	{
		case SIGINT:
			if (!qApp)
				raise(SC);
			else
			{
				QWidget *modalW = QApplication::activeModalWidget();
				if (!modalW && QMPlay2GUI.mainW)
				{
					QMPlay2GUI.mainW->close();
					QMPlay2GUI.mainW = NULL;
				}
				else
					raise(SC);
			}
			break;
		case SIGABRT:
#ifdef QT5_NOT_WIN
			if (!qAppOK && useGui)
			{
				canDeleteApp = useGui = false;
				longjmp(env, 1);
			}
#endif
			QMPlay2Core.log("QMPlay2 has been aborted (SIGABRT)", ErrorLog | AddTimeToLog | (qApp ? SaveLog : DontShowInGUI));
#ifndef Q_OS_WIN
			raise(SC);
#endif
			break;
		case SIGFPE:
			QMPlay2Core.log("QMPlay2 crashes (SIGFPE)", ErrorLog | AddTimeToLog | (qApp ? SaveLog : DontShowInGUI));
#ifndef Q_OS_WIN
			raise(SC);
#endif
			break;
		case SIGSEGV:
			QMPlay2Core.log("QMPlay2 crashes (SIGSEGV)", ErrorLog | AddTimeToLog | (qApp ? SaveLog : DontShowInGUI));
#ifndef Q_OS_WIN
			raise(SC);
#endif
			break;
	}
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
	return CallNextHookEx(NULL, code, wparam, lparam);
}
#endif

int main(int argc, char *argv[])
{
	signal(SIGINT, signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGSEGV, signal_handler);

#ifdef Q_WS_X11
	useGui = getenv("DISPLAY");
#endif
#ifdef QT5_NOT_WIN
	if (!setjmp(env))
#endif
#if QT_VERSION < 0x050000
		new QApplication(argc, argv, useGui);
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

	QStringList arguments = QCoreApplication::arguments();
	arguments.removeFirst();
	const bool help = arguments.contains("--help") || arguments.contains("-h");
	if (!help)
	{
		QLocalSocket socket;
		socket.connectToServer(QMPlay2GUI.getPipe(), QIODevice::WriteOnly);

		parseArguments(arguments);

		if (socket.waitForConnected(1000))
		{
			if (writeToSocket(socket))
			{
				socket.waitForBytesWritten(1000);
				useGui = false;
			}
			socket.disconnectFromServer();
		}
#ifndef Q_OS_WIN
		else if (QFile::exists(QMPlay2GUI.getPipe()))
		{
			QFile::remove(QMPlay2GUI.getPipe());
			QMPArguments.first += "noplay";
			QMPArguments.second += QString();
		}
#endif

		if (!useGui)
		{
#ifdef QT5_NOT_WIN
			if (canDeleteApp)
#endif
				delete qApp;
			return 0;
		}
	}

	QString libPath, sharePath = QCoreApplication::applicationDirPath();
	bool cmakeBuildFound = false;
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
		sharePath += "/../share/qmplay2";
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
		QString cdUp;
		if (!QDir(sharePath).exists("share"))
			cdUp = "/..";
		libPath = sharePath + cdUp + "/lib/qmplay2";
		sharePath += cdUp + "/share/qmplay2";
#else
		libPath = sharePath;
#endif
	}

	qRegisterMetaType<VideoFrame>("VideoFrame");

	QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

	QDir::setCurrent(QCoreApplication::applicationDirPath()); //Is it really needed?

	if (useGui)
		QApplication::setQuitOnLastWindowClosed(false);

#ifdef Q_OS_WIN
	HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, MMKeysHookProc, GetModuleHandle(NULL), 0);
#endif

	qsrand(time(NULL));

	do
	{
		/* QMPlay2GUI musi być stworzone już wcześniej */
		QMPlay2Core.init(!help, cmakeBuildFound, libPath, sharePath);

		Settings &settings = QMPlay2Core.getSettings();
		QString lastVer = settings.getString("Version", QMPlay2Version);
		settings.set("Version", QMPlay2Version);
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
			showHelp(QMPlay2Version);
			break;
		}

		QMPlay2GUI.loadIcons();

		QMPlay2GUI.pipe = new QLocalServer;
		if
		(
#ifdef Q_OS_WIN
			WaitNamedPipeA(QMPlay2GUI.getPipe().toLocal8Bit(), NMPWAIT_USE_DEFAULT_WAIT) ||
#endif
			!QMPlay2GUI.pipe->listen(QMPlay2GUI.getPipe())
		)
		{
			delete QMPlay2GUI.pipe;
			QMPlay2GUI.pipe = NULL;
			if (settings.getBool("AllowOnlyOneInstance"))
			{
				QMPlay2Core.quit();
				QLocalSocket socket;
				socket.connectToServer(QMPlay2GUI.getPipe(), QIODevice::WriteOnly);
				if (socket.waitForConnected(1000))
				{
					socket.write(QByteArray("show\t") + '\0');
					socket.waitForBytesWritten(1000);
					socket.disconnectFromServer();
				}
				break;
			}
		}

		QApplication::setWindowIcon(QMPlay2Core.getIconFromTheme("QMPlay2", QMPlay2Core.getQMPlay2Pixmap()));
		QMPlay2GUI.setStyle();

#ifdef UPDATER
		QString UpdateFile = settings.getString("UpdateFile");
		if (UpdateFile.startsWith("remove:"))
		{
			UpdateFile.remove(0, 7);
			if (lastVer != QMPlay2Version)
			{
				const QString updateString = QObject::tr("QMPlay2 has been updated to version") + " " + QMPlay2Version;
				QMPlay2Core.logInfo(updateString);
				QMessageBox::information(NULL, QCoreApplication::applicationName(), updateString);
				settings.remove("UpdateVersion");
			}
			else
			{
				const QString message = QObject::tr("QMPlay2 hasn't been updated. Do you want to run the update (recommended)?");
				if (QMessageBox::question(NULL, QCoreApplication::applicationName(), message, QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
				{
					QMPlay2GUI.runUpdate(UpdateFile);
					QMPlay2Core.quit();
					break;
				}
			}
			QFile::remove(UpdateFile);
			settings.remove("UpdateFile");
		}
		else if (QFileInfo(UpdateFile).size())
		{
			QMPlay2GUI.runUpdate(UpdateFile);
			QMPlay2Core.quit();
			break;
		}
		UpdateFile.clear();
#endif

		lastVer.clear();

		QMPlay2GUI.restartApp = QMPlay2GUI.removeSettings = false;
		new MainWidget(QMPArguments);
		QCoreApplication::exec();

		const QString settingsDir = QMPlay2Core.getSettingsDir();
		QMPlay2Core.quit();
		if (QMPlay2GUI.removeSettings)
			foreach (const QString &fName, QDir(settingsDir).entryList(QStringList("*.ini")))
				QFile::remove(settingsDir + fName);

		delete QMPlay2GUI.pipe;
	} while (QMPlay2GUI.restartApp);

#ifdef Q_OS_WIN
	UnhookWindowsHookEx(keyboardHook);
#endif

#ifdef QT5_NOT_WIN
	if (canDeleteApp)
#endif
		delete qApp;
	return 0;
}
