/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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

#include <YouTubeDL.hpp>

#include <NetworkAccess.hpp>
#include <QMPlay2Core.hpp>
#include <CppUtils.hpp>
#include <Version.hpp>
#ifdef Q_OS_WIN
	#include <Functions.hpp>
#endif

#include <QReadWriteLock>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

constexpr const char *g_name = "YouTubeDL";

static QReadWriteLock g_lock;

QString YouTubeDL::getFilePath()
{
	return QMPlay2Core.getSettingsDir() + "youtube-dl"
#ifdef Q_OS_WIN
	".exe"
#endif
	;
}

bool YouTubeDL::fixUrl(const QString &url, QString &outUrl, IOController<> *ioCtrl, QString *name, QString *extension, QString *error)
{
	IOController<YouTubeDL> &ytDl = ioCtrl->toRef<YouTubeDL>();
	if (ytDl.assign(new YouTubeDL))
	{
		QString newUrl, newError;
		ytDl->addr(url, QString(), &newUrl, name, extension, error ? &newError : nullptr);
		ytDl.reset();
		if (!newError.isEmpty() && !error->contains(newError))
		{
			if (!error->isEmpty())
				error->append("\n");
			error->append(newError);
		}
		if (!newUrl.isEmpty())
		{
			outUrl = newUrl;
			return true;
		}
	}
	return false;
}

YouTubeDL::YouTubeDL() :
	m_aborted(false)
{}
YouTubeDL::~YouTubeDL()
{}

void YouTubeDL::addr(const QString &url, const QString &param, QString *streamUrl, QString *name, QString *extension, QString *err)
{
	if (streamUrl || name)
	{
		QStringList paramList {"-e"};
		if (!param.isEmpty())
			paramList << "-f" << param;
		QStringList ytdlStdout = exec(url, paramList, err);
		if (!ytdlStdout.isEmpty())
		{
			QString title;
			if (ytdlStdout.count() > 1 && !ytdlStdout.at(0).contains("://"))
				title = ytdlStdout.takeFirst();
			if (streamUrl)
			{
				if (ytdlStdout.count() == 1)
					*streamUrl = ytdlStdout.at(0);
				else
				{
					*streamUrl = "FFmpeg://{";
					for (const QString &tmpUrl : asConst(ytdlStdout))
						*streamUrl += "[" + tmpUrl + "]";
					*streamUrl += "}";
				}
			}
			if (name && !title.isEmpty())
				*name = title;
			if (extension)
			{
				QStringList extensions;
				for (const QString &tmpUrl : asConst(ytdlStdout))
				{
					if (tmpUrl.contains("mp4"))
						extensions += ".mp4";
					else if (tmpUrl.contains("webm"))
						extensions += ".webm";
					else if (tmpUrl.contains("mkv"))
						extensions += ".mkv";
					else if (tmpUrl.contains("mpg"))
						extensions += ".mpg";
					else if (tmpUrl.contains("mpeg"))
						extensions += ".mpeg";
					else if (tmpUrl.contains("flv"))
						extensions += ".flv";
				}
				if (extensions.count() == 1)
					*extension = extensions.at(0);
				else for (const QString &tmpExt : asConst(extensions))
					*extension += "[" + tmpExt + "]";
			}
		}
	}
}

QStringList YouTubeDL::exec(const QString &url, const QStringList &args, QString *silentErr, bool canUpdate)
{
#ifndef Q_OS_ANDROID
	enum class Lock
	{
		Read,
		Write
	};

	const auto doLock = [this](const Lock lockType, const bool unlock)->bool {
		if (unlock)
			g_lock.unlock();
		for (;;)
		{
			if (m_aborted)
				return false;
			bool locked = false;
			switch (lockType)
			{
				case Lock::Read:
					locked = g_lock.tryLockForRead(100);
					break;
				case Lock::Write:
					locked = g_lock.tryLockForWrite(100);
					break;
			}
			if (m_aborted)
			{
				if (locked)
					g_lock.unlock();
				return false;
			}
			if (locked)
				break;
		}
		return true;
	};

	const QString ytDlPath = getFilePath();

	if (!doLock(Lock::Read, false))
		return {};

#ifndef Q_OS_WIN
	QFile file(ytDlPath);
	if (file.exists())
	{
		if (!doLock(Lock::Write, true)) // Unlock for read and lock for write
			return {};
		const QFile::Permissions exeFlags = QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther;
		if ((file.permissions() & exeFlags) != exeFlags)
			file.setPermissions(file.permissions() | exeFlags);
		if (!doLock(Lock::Read, true)) // Unlock for write and lock for read
			return {};
	}
#endif

	QStringList commonArgs {
		"--no-check-certificate", //Ignore SSL errors
	};

	const bool isVIDFile = url.contains("vidfile.net/");
	constexpr const char *mozillaUserAgent = "Mozilla";

	if (isVIDFile)
		commonArgs += {"--user-agent", mozillaUserAgent};

	const char *httpProxy = getenv("http_proxy");
	if (httpProxy && *httpProxy)
		commonArgs += {"--proxy", httpProxy};

	m_process.start(ytDlPath, QStringList() << url << "-g" << args << commonArgs << "-j");
	if (m_process.waitForFinished() && !m_aborted)
	{
		const auto finishWithError = [&](const QString &error) {
			if (!m_aborted)
			{
				if (silentErr)
					*silentErr = error;
				else
					emit QMPlay2Core.sendMessage(error, g_name, 3, 0);
			}
			g_lock.unlock(); // Unlock for read
		};

		QStringList result;

		bool isExitOk = (m_process.exitCode() == 0);
		QString error;

		if (isExitOk)
		{
			result = QString(m_process.readAllStandardOutput()).split('\n', QString::SkipEmptyParts);

			// Verify if URLs has printable characters, because sometimes we
			// can get binary garbage at output (especially on Openload).
			for (const QString &line : asConst(result))
			{
				if (line.startsWith("http"))
				{
					for (const QChar &c : line)
					{
						if (!c.isPrint())
						{
							error = "Invalid stream URL";
							isExitOk = false;
							break;
						}
					}
					if (!isExitOk)
						break;
				}
			}
		}

		if (!isExitOk)
		{
			result.clear();
			const QString newError = m_process.readAllStandardError();
			if (error.isEmpty())
			{
				error = newError;
				if (error.indexOf("ERROR: ") == 0)
					error.remove(0, 7);
			}
			if (canUpdate && !error.contains("said:")) // Probably update can fix the error, so do it!
			{
				if (!doLock(Lock::Write, true)) // Unlock for read and lock for write
					return {};
				QMPlay2Core.setWorking(true);
				m_process.start(ytDlPath, QStringList() << "-U" << commonArgs);
				QString updateOutput;
				bool updating = false;
				if (m_process.waitForStarted() && m_process.waitForReadyRead() && !m_aborted)
				{
					updateOutput = m_process.readAllStandardOutput();
					if (updateOutput.contains("Updating"))
					{
						emit QMPlay2Core.sendMessage(tr("Updating \"youtube-dl\", please wait..."), g_name);
						updating = true;
					}
				}
				if (!m_aborted && m_process.waitForFinished(-1) && !m_aborted)
				{
					updateOutput += m_process.readAllStandardOutput() + m_process.readAllStandardError();
					if (updateOutput.contains("ERROR:") || updateOutput.contains("package manager"))
						error += "\n" + updateOutput;
					else if (m_process.exitCode() == 0 && !updateOutput.contains("up-to-date"))
					{
#ifdef Q_OS_WIN
						const QString updatedFile = ytDlPath + ".new";
						QFile::remove(Functions::filePath(ytDlPath) + "youtube-dl-updater.bat");
						if (QFile::exists(updatedFile))
						{
							Functions::s_wait(0.2); //Wait 200 ms to be sure that file is closed
							QFile::remove(ytDlPath);
							if (QFile::rename(updatedFile, ytDlPath))
#endif
							{
								QMPlay2Core.setWorking(false);
								emit QMPlay2Core.sendMessage(tr("\"youtube-dl\" has been successfully updated!"), g_name);
								g_lock.unlock(); // Unlock for write
								return exec(url, args, silentErr, false);
							}
#ifdef Q_OS_WIN
						}
						else
						{
							error += "\nUpdated youtube-dl file: \"" + updatedFile + "\" not found!";
						}
#endif
					}
				}
				else if (updating && m_aborted)
				{
					emit QMPlay2Core.sendMessage(tr("\"youtube-dl\" update has been aborted!"), g_name, 2);
				}
				QMPlay2Core.setWorking(false);
				if (!doLock(Lock::Read, true)) // Unlock for write and lock for read
					return {};
			}
			finishWithError(error);
			return {};
		}

		//[Title], url, JSON, [url, JSON]
		for (int i = result.count() - 1; i >= 0; --i)
		{
			if (i > 0 && result.at(i).startsWith('{'))
			{
				const QString url = result.at(i - 1);

				if (isVIDFile)
					QMPlay2Core.addRawHeaders(url, QString("User-Agent: %1\r\nReferer: https://vidfile.net/v/\r\n").arg(mozillaUserAgent).toUtf8());

				const QJsonDocument json = QJsonDocument::fromJson(result.at(i).toUtf8());
				for (const QJsonValue &formats : json.object()["formats"].toArray())
				{
					if (url == formats.toObject()["url"].toString())
						QMPlay2Core.addCookies(url, formats.toObject()["http_headers"].toObject()["Cookie"].toString().toUtf8());
				}

				result.removeAt(i);
			}
		}

		g_lock.unlock(); // Unlock for read
		return result;
	}
	else if (canUpdate && !m_aborted && m_process.error() == QProcess::FailedToStart)
	{
		const QString downloadUrl = "https://yt-dl.org/downloads/latest/youtube-dl"
#ifdef Q_OS_WIN
		".exe"
#endif
		;

		NetworkAccess net;
		if (net.start(m_reply, downloadUrl))
		{
			if (!doLock(Lock::Write, true)) // Unlock for read and lock for write
			{
				m_reply.reset();
				return {};
			}
			QMPlay2Core.setWorking(true);
			emit QMPlay2Core.sendMessage(tr("Downloading \"youtube-dl\", please wait..."), g_name);
			m_reply->waitForFinished();
			const QByteArray replyData = m_reply->readAll();
			const bool hasError = m_reply->hasError();
			m_reply.reset();
			if (m_aborted)
				emit QMPlay2Core.sendMessage(tr("\"youtube-dl\" download has been aborted!"), g_name, 2);
			else if (!hasError)
			{
				QFile f(ytDlPath);
				if (f.open(QFile::WriteOnly | QFile::Truncate))
				{
					if (f.write(replyData) != replyData.size())
						f.remove();
					else
					{
						f.close();
						emit QMPlay2Core.sendMessage(tr("\"youtube-dl\" has been successfully downloaded!"), g_name);
						QMPlay2Core.setWorking(false);
						g_lock.unlock(); // Unlock for write
						return exec(url, args, silentErr, false);
					}
				}
			}
			QMPlay2Core.setWorking(false);
		}
	}

	g_lock.unlock(); // Unlock for read or for write (if download has failed)
#else
	Q_UNUSED(url)
	Q_UNUSED(args)
	Q_UNUSED(silentErr)
	Q_UNUSED(canUpdate)
#endif // Q_OS_ANDROID
	return {};
}

void YouTubeDL::abort()
{
	m_reply.abort();
	m_process.kill();
	m_aborted = true;
}
