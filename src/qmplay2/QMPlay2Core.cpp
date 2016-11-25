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

#include <QMPlay2Core.hpp>

#include <VideoFilters.hpp>
#include <Functions.hpp>
#include <Module.hpp>

#include <QApplication>
#include <QLibraryInfo>
#include <QTranslator>
#include <QDateTime>
#include <QLibrary>
#include <QPointer>
#include <QLocale>
#include <QFile>
#include <QDir>
#ifdef Q_OS_WIN
	#include <windows.h>
	#include <powrprof.h>
#endif

#include <stdio.h>

extern "C"
{
	#include <libavformat/avformat.h>
	#include <libavutil/log.h>
}

QMPlay2CoreClass *QMPlay2CoreClass::qmplay2Core;

QMPlay2CoreClass::QMPlay2CoreClass()
{
	systemTray = NULL;
	qmplay2Core = this;

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

#ifdef Q_OS_UNIX
QString QMPlay2CoreClass::getLibDir()
{
	QFile f;
	if (QFile::exists("/proc/self/maps")) //Linux
		f.setFileName("/proc/self/maps");
	else if (QFile::exists("/proc/curproc/map")) //FreeBSD
		f.setFileName("/proc/curproc/map");
	if (!f.fileName().isEmpty() && f.open(QFile::ReadOnly | QFile::Text))
	{
		const quintptr funcAddr = (quintptr)QMPlay2CoreClass::getLibDir;
		foreach (const QByteArray &line, f.readAll().split('\n'))
			if (!line.isEmpty())
			{
				quintptr addrBegin, addrEnd;
				char c; //'-' on Linux, ' ' on FreeBSD
				const int n = sscanf(line.data(), "%p%c%p", (void **)&addrBegin, &c, (void **)&addrEnd);
				if (n != 3)
					continue;
				if (funcAddr >= addrBegin && funcAddr <= addrEnd)
				{
					const int idx1 = line.indexOf('/');
					const int idx2 = line.lastIndexOf('/');
					if (idx1 > -1 && idx2 > idx1)
						return line.mid(idx1, idx2 - idx1);
					break;
				}
			}
	}
	return QString();
}
#endif

QString QMPlay2CoreClass::getLongFromShortLanguage(const QString &lng)
{
	const QString lang = QLocale::languageToString(QLocale(lng).language());
	return lang == "C" ? lng : lang;
}

bool QMPlay2CoreClass::canSuspend()
{
#if defined Q_OS_LINUX
	return !system("systemctl --help 2> /dev/null | grep -q suspend");
#elif defined Q_OS_WIN
	return true;
#else
	return false;
#endif
}
void QMPlay2CoreClass::suspend()
{
#if defined Q_OS_LINUX
	Q_UNUSED(system("systemctl suspend > /dev/null 2>&1 &"));
#elif defined Q_OS_WIN
	SetSuspendState(false, false, false);
#endif
}

void QMPlay2CoreClass::init(bool loadModules, bool modulesInSubdirs, const QString &libPath, const QString &sharePath, const QString &settingsPath)
{
	if (!settingsDir.isEmpty())
		return;

	const QString libDir = Functions::cleanPath(libPath);
	shareDir = Functions::cleanPath(sharePath);
	langDir = shareDir + "lang/";

	if (settingsPath.isEmpty())
	{
#ifdef Q_OS_WIN
		settingsDir = QFileInfo(QSettings(QSettings::IniFormat, QSettings::UserScope, QString()).fileName()).absolutePath() + "/QMPlay2/";
#else
		settingsDir = QDir::homePath() + "/.qmplay2/";
#endif
	}
	else
		settingsDir = Functions::cleanPath(settingsPath);
	QDir(settingsDir).mkpath(".");

	logFilePath = settingsDir + "QMPlay2.log";

	/* Rename config file */
	{
		const QString oldFFmpegConfig = settingsDir + "FFMpeg.ini";
		const QString newFFmpegConfig = settingsDir + "FFmpeg.ini";
		if (!QFile::exists(newFFmpegConfig) && QFile::exists(oldFFmpegConfig))
			QFile::rename(oldFFmpegConfig, newFFmpegConfig);
	}

	settings = new Settings("QMPlay2");

	translator = new QTranslator;
	qtTranslator = new QTranslator;
	QCoreApplication::installTranslator(translator);
	QCoreApplication::installTranslator(qtTranslator);
	setLanguage();

#ifdef Q_OS_WIN
	timeBeginPeriod(1); //ustawianie rozdzielczości timera na 1ms (dla Sleep())
#endif

#ifndef QT_DEBUG
	av_log_set_level(AV_LOG_FATAL);
#endif
	av_register_all();
	avformat_network_init();

	if (loadModules)
	{
		QFileInfoList pluginsList;
		QDir(settingsDir).mkdir("Modules");
		pluginsList += QDir(settingsDir + "Modules/").entryInfoList(QDir::Files);
		if (!modulesInSubdirs)
		{
#ifndef Q_OS_ANDROID
			pluginsList += QDir(libDir + "modules/").entryInfoList(QDir::Files);
#else
			pluginsList += QDir(libDir + "/").entryInfoList(QDir::Files);
#endif
		}
		else
		{
			//Scan for modules in subdirectories - designed for CMake non-installed build
			foreach (const QFileInfo &dInfo, QDir(libDir + "modules/").entryInfoList(QDir::Dirs))
				foreach (const QFileInfo &fInfo, QDir(dInfo.filePath()).entryInfoList(QDir::Files))
					if (QLibrary::isLibrary(fInfo.fileName()))
						pluginsList += fInfo;
		}

		QStringList pluginsName;
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
						if (pluginInstance)
						{
							if (pluginsName.contains(pluginInstance->name()))
								delete pluginInstance;
							else
							{
								pluginsName += pluginInstance->name();
								pluginsInstance += pluginInstance;
							}
						}
					}
				}
			}
	}

	VideoFilters::init();

	connect(this, SIGNAL(restoreCursor()), this, SLOT(restoreCursorSlot()));
	connect(this, SIGNAL(waitCursor()), this, SLOT(waitCursorSlot()));
	connect(this, SIGNAL(busyCursor()), this, SLOT(busyCursorSlot()));
}
void QMPlay2CoreClass::quit()
{
	if (settingsDir.isEmpty())
		return;
	foreach (Module *pluginInstance, pluginsInstance)
		delete pluginInstance;
	pluginsInstance.clear();
	videoFilters.clear();
	settingsDir.clear();
	shareDir.clear();
	langDir.clear();
	avformat_network_deinit();
#ifdef Q_OS_WIN
	timeEndPeriod(1);
#endif
	QCoreApplication::removeTranslator(qtTranslator);
	QCoreApplication::removeTranslator(translator);
	delete qtTranslator;
	delete translator;
	delete settings;
}

QStringList QMPlay2CoreClass::getModules(const QString &type, int typeLen) const
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
#endif
	if (type == "decoders")
		defaultModules << "FFmpeg Decoder";
	QStringList availableModules;
	const QString moduleType = type.mid(0, typeLen);
	foreach (Module *module, pluginsInstance)
		foreach (const Module::Info &moduleInfo, module->getModulesInfo())
			if ((moduleInfo.type == Module::WRITER && moduleInfo.extensions.contains(moduleType)) || (moduleInfo.type == Module::DECODER && moduleType == "decoder"))
				availableModules += moduleInfo.name;
	QStringList modules;
	foreach (const QString &module, settings->getStringList(type, defaultModules))
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

QIcon QMPlay2CoreClass::getIconFromTheme(const QString &iconName, const QIcon &fallback)
{
	const QIcon defaultIcon(fallback.isNull() ? QIcon(":/" + iconName) : fallback);
	return settings->getBool("IconsFromTheme") ? QIcon::fromTheme(iconName, defaultIcon) : defaultIcon;
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
		else if (!logFilePath.isEmpty())
			log(tr("Can't open log file"), ErrorLog | AddTimeToLog);
	}
	if (!(logFlags & DontShowInGUI))
		emit statusBarMessage(txt, 2500);
}

QStringList QMPlay2CoreClass::getLanguages() const
{
	QStringList langs = QDir(langDir).entryList(QStringList() << "*.qm", QDir::NoDotAndDotDot | QDir::Files | QDir::NoSymLinks);
	for (int i = 0; i < langs.size(); i++)
	{
		const int idx = langs.at(i).indexOf('.');
		if (idx > 0)
			langs[i].remove(idx, langs.at(i).size() - idx);
	}
	return langs;
}
void QMPlay2CoreClass::setLanguage()
{
	QString systemLang = QLocale::system().name();
	const int idx = systemLang.indexOf('_');
	if (idx > -1)
		systemLang.remove(idx, systemLang.size() - idx);
	lang = settings->getString("Language", systemLang);
	if (lang.isEmpty())
		lang = systemLang;
	if (!translator->load(lang, langDir))
		lang = "en";
	qtTranslator->load("qt_" + lang, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
}

void QMPlay2CoreClass::addVideoDeintMethod(QWidget *w)
{
	videoFilters.append(w);
}
QList<QWidget *> QMPlay2CoreClass::getVideoDeintMethods() const
{
	QList<QWidget *> ret;
	foreach (const QPointer<QWidget> &w, videoFilters)
		if (w)
			ret.append(w);
	return ret;
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
