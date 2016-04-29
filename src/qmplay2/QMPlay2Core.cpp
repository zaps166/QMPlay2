#include <QMPlay2Core.hpp>

#include <VideoFilters.hpp>
#include <Functions.hpp>
#include <Module.hpp>

#include <QApplication>
#include <QDateTime>
#include <QLibrary>
#include <QFile>
#include <QDir>
#ifdef Q_OS_WIN
	#include <windows.h>
	#include <shlwapi.h>
	#include <powrprof.h>
#endif

QMPlay2CoreClass *QMPlay2CoreClass::qmplay2Core;

QMPlay2CoreClass::QMPlay2CoreClass()
{
	qmplay2Core = this;
#ifdef Q_OS_WIN
	logFilePath = QDir::tempPath() + "/QMPlay2.log";
#else
	logFilePath = QDir::tempPath() + "/QMPlay2." + QString(getenv("USER")) + ".log";
#endif
#if defined Q_OS_MAC
	unixOpenCommand = "open ";
#elif defined Q_OS_UNIX
	unixOpenCommand = "xdg-open ";
#endif

	QFile f(":/Languages.csv");
	if (f.open(QFile::ReadOnly))
	{
		foreach (const QByteArray &line, f.readAll().split('\n'))
		{
			const QList<QByteArray> lineSplitted = line.split(',');
			if (lineSplitted.count() == 2)
				languages[lineSplitted[0]] = lineSplitted[1];
		}
	}
}
QMPlay2CoreClass::~QMPlay2CoreClass()
{}

bool QMPlay2CoreClass::canSuspend()
{
#if defined Q_OS_LINUX
	return !system("systemctl --help 2> /dev/null | grep suspend &> /dev/null");
#elif defined Q_OS_WIN
	return true;
#else
	return false;
#endif
}
void QMPlay2CoreClass::suspend()
{
#if defined Q_OS_LINUX
	system("systemctl suspend &> /dev/null &");
#elif defined Q_OS_WIN
	SetSuspendState(false, false, false);
#endif
}

void QMPlay2CoreClass::init(bool loadModules, const QString &_qmplay2Dir, const QString &_settingsDir)
{
	if (!settingsDir.isEmpty())
		return;

	qmplay2Dir = Functions::cleanPath(_qmplay2Dir);
	if (_settingsDir.isEmpty())
	{
#ifdef Q_OS_WIN
		settingsDir = QFileInfo(QSettings(QSettings::IniFormat, QSettings::UserScope, QString()).fileName()).absolutePath() + "/QMPlay2/";
#else
		settingsDir = QDir::homePath() + "/.qmplay2/";
#endif
	}
	else
		settingsDir = Functions::cleanPath(_settingsDir);
	QDir(settingsDir).mkpath(".");

	/* Rename config file */
	{
		const QString oldFFmpegConfig = settingsDir + "FFMpeg.ini";
		const QString newFFmpegConfig = settingsDir + "FFmpeg.ini";
		if (!QFile::exists(newFFmpegConfig) && QFile::exists(oldFFmpegConfig))
			QFile::rename(oldFFmpegConfig, newFFmpegConfig);
	}

#ifdef Q_OS_WIN
	timeBeginPeriod(1); //ustawianie rozdzielczości timera na 1ms (dla Sleep())
#endif

	if (loadModules)
	{
		QStringList pluginsName;
		QFileInfoList pluginsList;
#ifndef Q_OS_ANDROID
		QDir(settingsDir).mkdir("Modules");
		pluginsList += QDir(settingsDir + "Modules/").entryInfoList(QDir::Files | QDir::NoSymLinks);
		pluginsList += QDir( qmplay2Dir + "modules/").entryInfoList(QDir::Files | QDir::NoSymLinks);
#else
		pluginsList += QDir(qmplay2Dir).entryInfoList(QDir::Files | QDir::NoSymLinks);
#endif
		foreach (const QFileInfo &fInfo, pluginsList)
			if (QLibrary::isLibrary(fInfo.filePath()))
			{
				QLibrary lib(fInfo.filePath());
				if (!lib.load())
					log(lib.errorString(), AddTimeToLog | ErrorLog | SaveLog);
				else
				{
					typedef Module *(*QMPlay2PluginInstance)();
					QMPlay2PluginInstance qmplay2PluginInstance = (QMPlay2PluginInstance)lib.resolve("qmplay2PluginInstance");
					if (!qmplay2PluginInstance)
					{
#ifndef Q_OS_ANDROID
						log(fInfo.fileName() + " - " + tr("invalid QMPlay2 library"), AddTimeToLog | ErrorLog | SaveLog);
#endif
					}
					else
					{
						Module *pluginInstance = qmplay2PluginInstance();
						if (pluginInstance && !pluginsName.contains(pluginInstance->name()))
						{
							pluginsName += pluginInstance->name();
							pluginsInstance += pluginInstance;
						}
					}
				}
			}
	}

	VideoFilters::init();

	settings = new Settings("QMPlay2");

	connect(this, SIGNAL(restoreCursor()), this, SLOT(restoreCursorSlot()));
	connect(this, SIGNAL(waitCursor()), this, SLOT(waitCursorSlot()));
	connect(this, SIGNAL(busyCursor()), this, SLOT(busyCursorSlot()));
}
void QMPlay2CoreClass::quit()
{
	if (settingsDir.isEmpty())
		return;
	while (!pluginsInstance.isEmpty())
		delete pluginsInstance.takeFirst();
	qmplay2Dir.clear();
	settingsDir.clear();
#ifdef Q_OS_WIN
	timeEndPeriod(1);
#endif
	delete settings;
}

QIcon QMPlay2CoreClass::getIconFromTheme(const QString &icon)
{
	return settings->getBool("IconsFromTheme", true) ? QIcon::fromTheme(icon, QIcon(":/Icons/" + icon)) : QIcon(":/Icons/" + icon);
}

bool QMPlay2CoreClass::run(const QString &command, const QString &args)
{
	if (!command.isEmpty())
#ifdef Q_OS_WIN
		return (quintptr)ShellExecuteW(NULL, L"open", (WCHAR *)command.utf16(), (WCHAR *)args.utf16(), NULL, SW_SHOWNORMAL) > 32;
#else
	{
		if (args.isEmpty() && !unixOpenCommand.isEmpty())
			return !system(QString(unixOpenCommand + "\"" + command + "\" &").toLocal8Bit());
		else if (!args.isEmpty())
			return !system(QString("\"" + command + "\" " + args + " &").toLocal8Bit());
	}
#endif
	return false;
}

void QMPlay2CoreClass::log(const QString &txt, int logFlags)
{
	QString date;
	if (logFlags & LogOnce)
	{
		if (logs.contains(txt))
			return;
		else
			logs.append(txt);
	}
	if (logFlags & AddTimeToLog)
		date = "[" + QDateTime::currentDateTime().toString("dd MMM yyyy hh:mm:ss") + "] ";
	if (logFlags & InfoLog)
	{
		fprintf(stdout, "%s%s\n", date.toLocal8Bit().data(), txt.toLocal8Bit().data());
		fflush(stdout);
	}
	else if (logFlags & ErrorLog)
	{
		fprintf(stderr, "%s%s\n", date.toLocal8Bit().data(), txt.toLocal8Bit().data());
		fflush(stderr);
	}
	if (logFlags & SaveLog)
	{
		QFile logFile(logFilePath);
		if (logFile.open(QFile::Append))
		{
			logFile.write(date.toUtf8() + txt.toUtf8() + '\n');
			logFile.close();
		}
		else
			log(tr("Can't open log file"), ErrorLog | AddTimeToLog);
	}
	if (!(logFlags & DontShowInGUI))
		emit logSignal(txt);
}

void QMPlay2CoreClass::restoreCursorSlot()
{
	QApplication::restoreOverrideCursor();
}
void QMPlay2CoreClass::waitCursorSlot()
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}
void QMPlay2CoreClass::busyCursorSlot()
{
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
}
