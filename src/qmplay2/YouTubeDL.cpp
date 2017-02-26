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

#include <YouTubeDL.hpp>

#include <NetworkAccess.hpp>
#include <QMPlay2Core.hpp>
#include <Json11.hpp>

#include <QFile>

static bool youtubeDlUpdating;

static void exportCookiesFromJSON(const QString &jsonData, const QString &url)
{
	const Json json = Json::parse(jsonData.toUtf8());
	const QByteArray urlData = url.toUtf8();
	for (const Json &formats : json["formats"].array_items())
	{
		if (urlData == formats["url"].string_value())
			QMPlay2Core.addCookies(url, formats["http_headers"]["Cookie"].string_value());
	}
}

/**/

bool YouTubeDL::isUpdating()
{
	return youtubeDlUpdating;
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
					for (const QString &tmpUrl : ytdlStdout)
						*streamUrl += "[" + tmpUrl + "]";
					*streamUrl += "}";
				}
			}
			if (name && !title.isEmpty())
				*name = title;
			if (extension)
			{
				QStringList extensions;
				for (const QString &tmpUrl : ytdlStdout)
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
				else for (const QString &tmpExt : extensions)
					*extension += "[" + tmpExt + "]";
			}
		}
	}
}

QStringList YouTubeDL::exec(const QString &url, const QStringList &args, QString *silentErr, bool canUpdate)
{
	const QString ytDlPath = QMPlay2Core.getSettingsDir() + "youtube-dl"
#ifdef Q_OS_WIN
	".exe"
#endif
	;

	if (isUpdating())
		return {};

#ifndef Q_OS_WIN
	QFile file(ytDlPath);
	if (file.exists())
	{
		QFile::Permissions exeFlags = QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther;
		if ((file.permissions() & exeFlags) != exeFlags)
			file.setPermissions(file.permissions() | exeFlags);
	}
#endif

	QStringList commonArgs("--no-check-certificate"); //Ignore SSL errors

	const char *httpProxy = getenv("http_proxy");
	if (httpProxy && *httpProxy)
		commonArgs << "--proxy" << httpProxy;

	m_process.start(ytDlPath, QStringList() << url << "-g" << args << commonArgs << "-j");
	if (m_process.waitForFinished() && !m_aborted)
	{
		if (m_process.exitCode() != 0)
		{
			QString error = m_process.readAllStandardError();
			const int idx = error.indexOf("ERROR:");
			if (idx > -1)
				error.remove(0, idx);
			if (canUpdate && !error.contains("said:")) // Update is necessary
			{
				youtubeDlUpdating = true;
				m_process.start(ytDlPath, QStringList() << "-U" << commonArgs);
				if (m_process.waitForFinished(-1) && !m_aborted)
				{
					const QString error2 = m_process.readAllStandardOutput() + m_process.readAllStandardError();
					if (error2.contains("ERROR:") || error2.contains("package manager"))
						error += "\n" + error2;
					else if (m_process.exitCode() == 0 && !error2.contains("up-to-date"))
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
								youtubeDlUpdating = false;
								QMPlay2Core.sendMessage(tr("\"youtube-dl\" has been successfully updated!"), "YouTubeDL");
								return exec(url, args, silentErr, false);
							}
							youtubeDlUpdating = false;
#ifdef Q_OS_WIN
						}
						else
						{
							error += "\nUpdated youtube-dl file: \"" + updatedFile + "\" not found!";
						}
#endif
					}
				}
				youtubeDlUpdating = false;
			}
			if (!m_aborted)
			{
				if (silentErr)
					*silentErr = error;
				else
					emit QMPlay2Core.sendMessage(error, "YouTubeDL", 3, 0);
			}
			return {};
		}

		//[Title], url, JSON, [url, JSON]
		QStringList result = QString(m_process.readAllStandardOutput()).split('\n', QString::SkipEmptyParts);
		for (int i = result.count() - 1; i >= 0; --i)
		{
			if (i > 0 && result.at(i).startsWith('{'))
			{
				exportCookiesFromJSON(result.at(i), result.at(i - 1));
				result.removeAt(i);
			}
		}

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
			youtubeDlUpdating = true;
			m_reply->waitForFinished();
			const QByteArray replyData = m_reply->readAll();
			m_reply.clear();
			if (!m_reply->hasError())
			{
				QFile f(ytDlPath);
				if (f.open(QFile::WriteOnly | QFile::Truncate))
				{
					if (f.write(replyData) != replyData.size())
						f.remove();
					else
					{
						f.close();
						QMPlay2Core.sendMessage(tr("\"youtube-dl\" has been successfully downloaded!"), "YouTubeDL");
						youtubeDlUpdating = false;
						return exec(url, args, silentErr, false);
					}
				}
			}
			youtubeDlUpdating = false;
		}
	}
	return {};
}

void YouTubeDL::abort()
{
	m_reply.abort();
	m_process.kill();
	m_aborted = true;
}
