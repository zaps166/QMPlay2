/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2021  Błażej Szczygieł

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
#include <Functions.hpp>

#include <QRegularExpression>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QMutex>
#include <QFile>

constexpr const char *g_name = "YouTubeDL";
static bool g_mustUpdate = true;
static QMutex g_mutex(QMutex::Recursive);

QString YouTubeDL::getFilePath()
{
    return QMPlay2Core.getSettingsDir() + "yt-dlp"
#ifdef Q_OS_WIN
    "_x86.exe"
#endif
    ;
}
QStringList YouTubeDL::getCommonArgs()
{
    QStringList commonArgs {
        "--no-check-certificate", // Ignore SSL errors
        "--user-agent", Functions::getUserAgent(),
    };

    const char *httpProxy = getenv("http_proxy");
    if (httpProxy && *httpProxy)
        commonArgs += {"--proxy", httpProxy};

    return commonArgs;
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

YouTubeDL::YouTubeDL()
    : m_ytDlPath(getFilePath())
    , m_commonArgs(getCommonArgs())
    , m_aborted(false)
{}
YouTubeDL::~YouTubeDL()
{}

void YouTubeDL::addr(const QString &url, const QString &param, QString *streamUrl, QString *name, QString *extension, QString *err)
{
    if (!streamUrl && !name)
        return;

    QStringList paramList {"-e"};
    if (!param.isEmpty())
        paramList << "-f" << param;
    QStringList ytdlStdout = exec(url, paramList, err);
    if (ytdlStdout.isEmpty())
        return;

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
            for (const QString &tmpUrl : qAsConst(ytdlStdout))
                *streamUrl += "[" + tmpUrl + "]";
            *streamUrl += "}";
        }
    }
    if (name && !title.isEmpty())
        *name = title;
    if (extension)
    {
        QStringList extensions;
        for (const QString &tmpUrl : qAsConst(ytdlStdout))
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
        else for (const QString &tmpExt : qAsConst(extensions))
            *extension += "[" + tmpExt + "]";
    }
}

QStringList YouTubeDL::exec(const QString &url, const QStringList &args, QString *silentErr, bool rawOutput)
{
    if (!prepare())
        return {};

    QStringList processArgs;
    processArgs += url;
    if (!rawOutput)
        processArgs += "-g";
    processArgs += args;
    processArgs += m_commonArgs;
    if (!rawOutput)
        processArgs += "-j";

    startProcess(processArgs);
    if (!m_process.waitForStarted() && !m_aborted)
    {
        if (!onProcessCantStart())
            return {};
        startProcess(processArgs);
    }

    if (!m_process.waitForFinished() || m_aborted)
        return {};

    QStringList result;

    bool isOk = (m_process.exitCode() == 0);
    QString error;

    if (isOk)
    {
        result = QStringList(m_process.readAllStandardOutput());
        if (rawOutput)
        {
            result += m_process.readAllStandardError();
        }
        else
        {
            result = result.constFirst().split('\n', QString::SkipEmptyParts);

            // Verify if URLs has printable characters, because sometimes we
            // can get binary garbage at output (especially on Openload).
            for (const QString &line : qAsConst(result))
            {
                if (line.startsWith("http"))
                {
                    for (const QChar &c : line)
                    {
                        if (!c.isPrint())
                        {
                            error = "Invalid stream URL";
                            isOk = false;
                            break;
                        }
                    }
                    if (!isOk)
                        break;
                }
            }
        }
    }

    if (!isOk)
    {
        result.clear();
        const QString newError = m_process.readAllStandardError();
        if (error.isEmpty())
        {
            error = newError;
            if (error.indexOf("ERROR: ") == 0)
                error.remove(0, 7);
        }
        if (!m_aborted)
        {
            if (silentErr)
                *silentErr = error;
            else
                emit QMPlay2Core.sendMessage(error, g_name, 3, 0);
        }
        return {};
    }

    if (!rawOutput)
    {
        // [Title], url, JSON, [url, JSON]
        for (int i = result.count() - 1; i >= 0; --i)
        {
            if (i > 0 && result.at(i).startsWith('{'))
            {
                const QString url = result.at(i - 1);

                const QJsonDocument json = QJsonDocument::fromJson(result.at(i).toUtf8());
                for (const QJsonValue &formats : json["formats"].toArray())
                {
                    if (url == formats["url"].toString())
                        QMPlay2Core.addCookies(url, formats["http_headers"]["Cookie"].toString().toUtf8());
                }

                result.removeAt(i);
            }
        }
    }

    return result;
}

void YouTubeDL::abort()
{
    m_reply.abort();
    m_process.kill();
    m_aborted = true;
}

bool YouTubeDL::prepare()
{
#ifdef Q_OS_ANDROID
    return false;
#endif

    while (!g_mutex.tryLock(100))
    {
        if (m_aborted)
            return false;
    }

    if (!QFileInfo(m_ytDlPath).exists())
    {
        if (!download())
        {
            qCritical() << "Unable to download \"youtube-dl\"";
            g_mutex.unlock();
            return false;
        }
        g_mustUpdate = false;
    }

    if (g_mustUpdate)
    {
        const bool processOk = update();
        if (m_aborted)
        {
            g_mutex.unlock();
            return false;
        }
        if (!processOk)
        {
            const bool ret = onProcessCantStart();
            g_mutex.unlock();
            return ret;
        }
        g_mustUpdate = false;
    }

    ensureExecutable();

    g_mutex.unlock();
    return true;
}

bool YouTubeDL::download()
{
    // Mutex must be locked here

    const QString downloadUrl = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp"
#ifdef Q_OS_WIN
    "_x86.exe"
#endif
    ;

    QMPlay2Core.setWorking(true);

    NetworkAccess net;
    if (net.start(m_reply, downloadUrl))
    {
        emit QMPlay2Core.sendMessage(tr("Downloading \"youtube-dl\", please wait..."), g_name);

        m_reply->waitForFinished();

        const QByteArray replyData = m_reply->readAll();
        const bool hasError = m_reply->hasError();

        m_reply.reset();

        if (m_aborted)
        {
            emit QMPlay2Core.sendMessage(tr("\"youtube-dl\" download has been aborted!"), g_name, 2);
        }
        else if (!hasError)
        {
            QFile f(m_ytDlPath);
            if (f.open(QFile::WriteOnly | QFile::Truncate))
            {
                if (f.write(replyData) != replyData.size())
                {
                    f.remove();
                }
                else
                {
                    emit QMPlay2Core.sendMessage(tr("\"youtube-dl\" has been successfully downloaded!"), g_name);
                    QMPlay2Core.setWorking(false);
                    return true;
                }
            }
        }
    }

    if (!m_aborted)
        emit QMPlay2Core.sendMessage(tr("\"youtube-dl\" download has failed!"), g_name, 3);

    QMPlay2Core.setWorking(false);
    return false;
}
bool YouTubeDL::update()
{
    // Mutex must be locked here

    qDebug() << "\"youtube-dl\" updates will be checked";
    QMPlay2Core.setWorking(true);

    ensureExecutable();
    startProcess(QStringList() << "-U" << m_commonArgs);
    if (!m_process.waitForStarted())
    {
        QMPlay2Core.setWorking(false);
        return false;
    }

    QString updateOutput;
    bool updating = false;

    if (m_process.waitForReadyRead() && !m_aborted)
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
        {
            qCritical() << "youtube-dl update failed:" << updateOutput;
        }
        else if (m_process.exitCode() == 0 && !updateOutput.contains(QRegularExpression(R"(up\Wto\Wdate)")))
        {
            QMPlay2Core.setWorking(false);
            emit QMPlay2Core.sendMessage(tr("\"youtube-dl\" has been successfully updated!"), g_name);
            return true;
        }
    }
    else if (updating && m_aborted)
    {
        emit QMPlay2Core.sendMessage(tr("\"youtube-dl\" update has been aborted!"), g_name, 2);
    }

    QMPlay2Core.setWorking(false);
    return true;
}

void YouTubeDL::ensureExecutable()
{
#ifndef Q_OS_WIN
    if (!QFileInfo(m_ytDlPath).isExecutable())
    {
        QFile file(m_ytDlPath);
        file.setPermissions(file.permissions() | QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther);
    }
#endif
}

bool YouTubeDL::onProcessCantStart()
{
    if (!QFile::remove(m_ytDlPath))
    {
        qCritical() << "Can't start \"youtube-dl\" process";
        return false;
    }

    qCritical() << "Can't start \"youtube-dl\" process, forced \"youtube-dl\" download";
    return prepare();
}

void YouTubeDL::startProcess(QStringList args)
{
    QString program = m_ytDlPath;

#ifndef Q_OS_WIN
    QFile ytDlFile(program);
    if (ytDlFile.open(QFile::ReadOnly))
    {
        const auto shebang = ytDlFile.readLine(99).trimmed();
        const int idx = shebang.lastIndexOf("python");
        if (shebang.startsWith("#!") && idx > -1)
        {
            const auto pythonCmd = shebang.mid(idx);
            if (!QStandardPaths::findExecutable(pythonCmd).endsWith(pythonCmd))
            {
                QStringList pythonCmdsToCheck {
                    "python",
                    "python3",
                };
                pythonCmdsToCheck.removeOne(pythonCmd);
                for (auto &&pythonCmd : qAsConst(pythonCmdsToCheck))
                {
                    if (QStandardPaths::findExecutable(pythonCmd).endsWith(pythonCmd))
                    {
                        args.prepend(program);
                        program = pythonCmd;
                        break;
                    }
                }
            }
        }
        ytDlFile.close();
    }
#endif

    m_process.start(program, args);
}
