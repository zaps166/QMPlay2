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
#include <QTranslator>
#include <QPainter>
#include <QLocale>
#include <QBuffer>
#include <QFile>
#include <QDir>
#if QT_VERSION < 0x050000
	#include <QTextCodec>
#endif

#include <time.h>

static QTranslator *translator = NULL;
static bool useGui = true;

QMPlay2GUIClass &QMPlay2GUIClass::instance()
{
	static QMPlay2GUIClass qmplay2Gui;
	return qmplay2Gui;
}

QString QMPlay2GUIClass::getLongFromShortLanguage(const QString &lng)
{
	const QString lang = QLocale::languageToString(QLocale(lng).language());
	return lang == "C" ? lng : lang;
}
QStringList QMPlay2GUIClass::getModules(const QString &type, int typeLen)
{
	QStringList defaultModules;
#if defined Q_OS_LINUX
	if (type == "videoWriters")
		defaultModules << "OpenGL 2" << "XVideo";
	else if (type == "audioWriters")
		defaultModules << "PulseAudio" << "ALSA";
#elif defined Q_OS_WIN
	if (type == "videoWriters")
		defaultModules << "OpenGL 2" << "DirectDraw";
#elif defined Q_OS_ANDROID
	if (type == "videoWriters")
		defaultModules << "QPainter" << "OpenGL 2";
#endif
	QStringList availableModules;
	const QString moduleType = type.mid(0, typeLen);
	foreach (Module *module, QMPlay2Core.getPluginsInstance())
		foreach (const Module::Info &moduleInfo, module->getModulesInfo())
			if ((moduleInfo.type == Module::WRITER && moduleInfo.extensions.contains(moduleType)) || (moduleInfo.type == Module::DECODER && moduleType == "decoder"))
				availableModules += moduleInfo.name;
	QStringList modules;
	foreach (const QString &module, QMPlay2Core.getSettings().get(type, defaultModules).toStringList())
	{
		const int idx = availableModules.indexOf(module);
		if (idx > -1)
		{
			availableModules.removeAt(idx);
			modules += module;
		}
	}
	return modules + availableModules;
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

QStringList QMPlay2GUIClass::getLanguages()
{
	QStringList langs = QDir(langPath).entryList(QDir::NoDotAndDotDot | QDir::Files | QDir::NoSymLinks);
	for (int i = 0; i < langs.size(); i++)
	{
		int idx = langs[i].indexOf('.');
		if (idx > 0)
			langs[i].remove(idx, langs[i].size() - idx);
	}
	return langs;
}
void QMPlay2GUIClass::setLanguage()
{
	QString systemLang = QLocale::system().name();
	const int idx = systemLang.indexOf('_');
	if (idx > -1)
		systemLang.remove(idx, systemLang.size() - idx);
	lang = settings->get("Language", systemLang).toString();
	if (lang.isEmpty())
		lang = systemLang;
	if (!translator->load(lang, langPath))
		lang = "en";
}

void QMPlay2GUIClass::setStyle()
{
	QString defaultStyle;
#if defined Q_OS_ANDROID
	defaultStyle = "fusion"; //Android style is awful in Qt (tested on Qt 5.4 and Qt 5.5)
#endif
	QApplication::setStyle(settings->get("Style", defaultStyle).toString());
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
		pth = settings->get("currPth").toString();
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
			if (QMPArguments.second[i].left(7) == "file://")
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
#ifndef Q_OS_WIN
		case SIGTSTP:
			raise(SC);
			break;
#endif
		case SIGINT:
		{
			QWidget *modalW = QApplication::activeModalWidget();
			if (!modalW && QMPlay2GUI.mainW)
			{
				QMPlay2GUI.mainW->close();
				QMPlay2GUI.mainW = NULL;
			}
			else
				raise(SC);
		} break;
		case SIGSEGV:
			QMPlay2Core.logError("Program się wysypał (SIGSEGV)");
#ifndef Q_OS_WIN
			raise(SC);
#endif
			break;
		case SIGABRT:
#ifdef QT5_NOT_WIN
			if (!qAppOK && useGui)
			{
				canDeleteApp = useGui = false;
				longjmp(env, 1);
			}
#endif
			QMPlay2Core.logError("Program przerwał działanie (SIGABRT)");
#ifndef Q_OS_WIN
			raise(SC);
#endif
			break;
		case SIGFPE:
			QMPlay2Core.logError("Program się wysypał (SIGFPE)");
#ifndef Q_OS_WIN
			raise(SC);
#endif
			break;
	}
}


int main(int argc, char *argv[])
{
	signal(SIGINT, signal_handler);
#ifndef Q_OS_WIN
	signal(SIGTSTP, signal_handler);
#endif
	signal(SIGFPE, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGABRT, signal_handler);
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

	QString sharePath = QCoreApplication::applicationDirPath();
	QString   libPath = QCoreApplication::applicationDirPath();
#if !defined Q_OS_WIN && !defined Q_OS_MAC && !defined Q_OS_ANDROID
	sharePath += "/../share/qmplay2";
	libPath += "/../";
	if (sizeof(void *) == 8 && QDir(libPath).exists("lib64"))
		libPath += "lib64";
	else if (sizeof(void *) == 4 && QDir(libPath).exists("lib32"))
		libPath += "lib32";
	else
		libPath += "lib";
	libPath += "/qmplay2";
#endif

	qRegisterMetaType<VideoFrame>("VideoFrame");

	QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

	QDir::setCurrent(QCoreApplication::applicationDirPath()); //Is it really needed?

	translator = new QTranslator;
	QCoreApplication::installTranslator(translator);
	if (useGui)
		QApplication::setQuitOnLastWindowClosed(false);
	QMPlay2GUI.langPath = sharePath + "/lang/";

	qsrand(time(NULL));

	do
	{
		/* QMPlay2GUI musi być stworzone już wcześniej */
		QMPlay2Core.init(!help, libPath, sharePath);

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

		QMPlay2GUI.setLanguage();

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

		QApplication::setWindowIcon(QMPlay2Core.getQMPlay2Pixmap());
		QMPlay2GUI.setStyle();

#ifdef UPDATER
		QString UpdateFile = settings.getString("UpdateFile");
		if (UpdateFile.left(7) == "remove:")
		{
			UpdateFile.remove(0, 7);
			if (lastVer != QMPlay2Version)
			{
				QString updateString = QObject::tr("QMPlay2 has been updated to version") + " " + QMPlay2Version;
				QMPlay2Core.logInfo(updateString);
				QMessageBox::information(NULL, QCoreApplication::applicationName(), updateString);
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

	delete translator;

#ifdef QT5_NOT_WIN
	if (canDeleteApp)
#endif
		delete qApp;
	return 0;
}
