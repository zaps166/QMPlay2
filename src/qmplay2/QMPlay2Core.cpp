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

#include <QMPlay2Core.hpp>

#include <VideoFilters.hpp>
#include <Functions.hpp>
#include <Playlist.hpp>
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
#if defined Q_OS_WIN
	#include <windows.h>
	#include <powrprof.h>
#elif defined Q_OS_MAC
	#include <QStandardPaths>
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	#define QT_VERSION_MAJOR 4
	#define QT_VERSION_MINOR 8 // Qt 4.8.0 is the oldest supported Qt version
#endif

#include <cstdio>

extern "C"
{
	#include <libavformat/avformat.h>
	#include <libavutil/log.h>
}

/**/

template<typename Data>
static QByteArray getCookiesOrResource(const QString &url, Data &data)
{
	auto it = data.find(url);
	if (it == data.end())
		return QByteArray();
	const QByteArray ret = it.value().first;
	if (it.value().second)
		data.erase(it);
	return ret;
}

/**/

QMPlay2CoreClass *QMPlay2CoreClass::qmplay2Core;

QMPlay2CoreClass::QMPlay2CoreClass()
{
	qmplay2Core = this;

	QFile f(":/Languages.csv");
	if (f.open(QFile::ReadOnly))
	{
		for (const QByteArray &line : f.readAll().split('\n'))
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
		for (const QByteArray &line : f.readAll().split('\n'))
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
#elif defined Q_OS_WIN || defined Q_OS_MAC
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
#elif defined Q_OS_MAC
	Q_UNUSED(system("pmset sleepnow > /dev/null 2>&1 &"));
#endif
}

void QMPlay2CoreClass::init(bool loadModules, bool modulesInSubdirs, const QString &libPath, const QString &sharePath, const QString &profileName)
{
	if (!settingsDir.isEmpty())
		return;

	const QString libDir = Functions::cleanPath(libPath);
	shareDir = Functions::cleanPath(sharePath);
	langDir = shareDir + "lang/";

#ifdef QMPLAY2_PORTABLE
	settingsDir = QCoreApplication::applicationDirPath() + "/settings/";
#else
	#if defined(Q_OS_WIN)
		settingsDir = QFileInfo(QSettings(QSettings::IniFormat, QSettings::UserScope, QString()).fileName()).absolutePath() + "/QMPlay2/";
	#elif defined(Q_OS_MAC)
		settingsDir = Functions::cleanPath(QStandardPaths::standardLocations(QStandardPaths::DataLocation).value(0, settingsDir));
	#else
		settingsDir = QDir::homePath() + "/.qmplay2/";
	#endif
#endif
	QDir(settingsDir).mkpath(".");

	{
		settingsProfile = Functions::cleanFileName(profileName, QString());
		if (settingsProfile.isEmpty())
		{
			const QSettings profileSettings(settingsDir + "Profile.ini", QSettings::IniFormat);
			settingsProfile = profileSettings.value("Profile", "/").toString();
		}
		else if (settingsProfile.compare("default", Qt::CaseInsensitive) == 0)
		{
			settingsProfile = "/";
		}

		if (settingsProfile != "/")
		{
			settingsProfile.prepend("Profiles/");
			QDir(settingsDir).mkpath(settingsProfile);
		}

		if (!settingsProfile.endsWith('/'))
			settingsProfile.append('/');
	}

	logFilePath = settingsDir + "QMPlay2.log";

	/* Rename config files */
	{
		const QString oldFFmpegConfig = settingsDir + "FFMpeg.ini";
		const QString newFFmpegConfig = settingsDir + "FFmpeg.ini";
		if (!QFile::exists(newFFmpegConfig) && QFile::exists(oldFFmpegConfig))
			QFile::rename(oldFFmpegConfig, newFFmpegConfig);

		const QString oldNotifiesConfig = settingsDir + "Notifies.ini";
		const QString newFNotifyConfig  = settingsDir + "Notify.ini";
		if (!QFile::exists(newFNotifyConfig) && QFile::exists(oldNotifiesConfig))
			QFile::rename(oldNotifiesConfig, newFNotifyConfig);
	}

	settings = new Settings("QMPlay2");

	translator = new QTranslator;
	qtTranslator = new QTranslator;
	QCoreApplication::installTranslator(translator);
	QCoreApplication::installTranslator(qtTranslator);
	setLanguage();

#ifdef Q_OS_WIN
	timeBeginPeriod(1); //Set the timer for 1ms resolution (for Sleep ())
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
			for (const QFileInfo &dInfo : QDir(libDir + "modules/").entryInfoList(QDir::Dirs))
				for (const QFileInfo &fInfo : QDir(dInfo.filePath()).entryInfoList(QDir::Files))
					if (QLibrary::isLibrary(fInfo.fileName()))
						pluginsList += fInfo;
		}

		QStringList pluginsName;
		for (const QFileInfo &fInfo : pluginsList)
		{
			if (QLibrary::isLibrary(fInfo.filePath()))
			{
				QLibrary lib(fInfo.filePath());
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
				// Don't override global symbols if they are different in libraries (e.g. Qt5 vs Qt4)
				lib.setLoadHints(QLibrary::DeepBindHint);
#endif
				if (!lib.load())
					log(lib.errorString(), AddTimeToLog | ErrorLog | SaveLog);
				else
				{
					using CreateQMPlay2ModuleInstance = Module  *(*)();
					using GetQMPlay2ModuleAPIVersion  = quint32  (*)();

					GetQMPlay2ModuleAPIVersion  getQMPlay2ModuleAPIVersion  = (GetQMPlay2ModuleAPIVersion )lib.resolve("getQMPlay2ModuleAPIVersion" );
					CreateQMPlay2ModuleInstance createQMPlay2ModuleInstance = (CreateQMPlay2ModuleInstance)lib.resolve("createQMPlay2ModuleInstance");

					const auto checkModuleAPIVersion = [&](const quint32 v)->bool {
						const quint8 moduleApiVersion = (v & 0xFF);
						const quint8   qtMajorVersion = ((v >> 24) & 0xFF);
						const quint8   qtMinorVersion = ((v >> 16) & 0xFF);
						if (moduleApiVersion != QMPLAY2_MODULES_API_VERSION)
						{
							log(fInfo.fileName() + " - " + tr("mismatch module API version"), AddTimeToLog | ErrorLog | SaveLog);
							return false;
						}
						if (qtMajorVersion != QT_VERSION_MAJOR || qtMinorVersion < QT_VERSION_MINOR)
						{
							log(fInfo.fileName() + " - " + tr("mismatch module Qt version"), AddTimeToLog | ErrorLog | SaveLog);
							return false;
						}
						return true;
					};

					if (!getQMPlay2ModuleAPIVersion || !createQMPlay2ModuleInstance)
					{
#ifndef Q_OS_ANDROID
						if (lib.resolve("qmplay2PluginInstance"))
							log(fInfo.fileName() + " - " + tr("too old QMPlay2 library"), AddTimeToLog | ErrorLog | SaveLog);
						else
							log(fInfo.fileName() + " - " + tr("invalid QMPlay2 library"), AddTimeToLog | ErrorLog | SaveLog);
#endif
					}
					else if (checkModuleAPIVersion(getQMPlay2ModuleAPIVersion()))
					{
						if (Module *moduleInstance = createQMPlay2ModuleInstance())
						{
							const QString name = moduleInstance->name();
							if (pluginsName.contains(name))
							{
								log(fInfo.fileName() + " (" + name + ") - " + tr("duplicated module name"), AddTimeToLog | ErrorLog | SaveLog);
								delete moduleInstance;
							}
							else
							{
								pluginsName += moduleInstance->name();
								pluginsInstance += moduleInstance;
							}
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
	for (Module *pluginInstance : pluginsInstance)
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
	for (Module *module : pluginsInstance)
		for (const Module::Info &moduleInfo : module->getModulesInfo())
			if ((moduleInfo.type == Module::WRITER && moduleInfo.extensions.contains(moduleType)) || (moduleInfo.type == Module::DECODER && moduleType == "decoder"))
				availableModules += moduleInfo.name;
	QStringList modules;
	for (const QString &module : settings->getStringList(type, defaultModules))
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
	QStringList langs = QDir(langDir).entryList({"*.qm"}, QDir::NoDotAndDotDot | QDir::Files | QDir::NoSymLinks);
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
	for (const QPointer<QWidget> &w : videoFilters)
		if (w)
			ret.append(w);
	return ret;
}

void QMPlay2CoreClass::addCookies(const QString &url, const QByteArray &newCookies, const bool removeAfterUse)
{
	if (!url.isEmpty())
	{
		QMutexLocker locker(&cookies.mutex);
		if (newCookies.isEmpty())
			cookies.data.remove(url);
		else
			cookies.data[url] = {newCookies, removeAfterUse};
	}
}
QByteArray QMPlay2CoreClass::getCookies(const QString &url)
{
	QMutexLocker locker(&cookies.mutex);
	return getCookiesOrResource(url, cookies.data);
}

void QMPlay2CoreClass::addResource(const QString &url, const QByteArray &data)
{
	if (url.length() > 10 && url.startsWith("QMPlay2://"))
	{
		QMutexLocker locker(&resources.mutex);
		if (data.isNull())
			resources.data.remove(url);
		else
			resources.data[url] = {data, false};
	}
}
void QMPlay2CoreClass::modResource(const QString &url, const bool removeAfterUse)
{
	QMutexLocker locker(&resources.mutex);
	auto it = resources.data.find(url);
	if (it != resources.data.end())
		it.value().second = removeAfterUse;
}
bool QMPlay2CoreClass::hasResource(const QString &url) const
{
	QMutexLocker locker(&resources.mutex);
	return resources.data.contains(url);
}
QByteArray QMPlay2CoreClass::getResource(const QString &url)
{
	QMutexLocker locker(&resources.mutex);
	return getCookiesOrResource(url, resources.data);
}

void QMPlay2CoreClass::loadPlaylistGroup(const QString &name, const QMPlay2CoreClass::GroupEntries &entries, bool enqueue, const QString &context)
{
	if (!entries.isEmpty())
	{
		const QString resourceName = "QMPlay2://" + context + "/" + name + ".pls";
		Playlist::Entries playlistEntries;
		for (const auto &entry : entries)
			playlistEntries += {entry.first, entry.second};
		if (Playlist::write(playlistEntries, resourceName))
		{
			modResource(resourceName, true);
			emit processParam(enqueue ? "enqueue" : "open", resourceName);
		}
	}
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
