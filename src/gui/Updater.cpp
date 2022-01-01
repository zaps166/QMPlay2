/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <Updater.hpp>

#include <Functions.hpp>
#include <Settings.hpp>
#include <Notifies.hpp>
#ifndef UPDATER
    #define endWork(no_Op)
#else
    #include <Version.hpp>
    #include <Main.hpp>

    #include <QProgressBar>
    #include <QPushButton>
    #include <QVBoxLayout>
    #include <QLabel>
#endif
#include <QCoreApplication>
#include <QSysInfo>
#include <QDir>

#ifdef UPDATER
Updater::Updater(QWidget *parent) :
    QDialog(parent),
#else
Updater::Updater(QObject *parent) :
    QObject(parent),
#endif
    net(this),
    busy(false)
{
    infoFile.setFileName(QString("%1/%2.%3").arg(QDir::tempPath(), "QMPlay2UpdaterURLs").arg(QCoreApplication::applicationPid()));
#ifdef UPDATER
    updateFile.setFileName(QMPlay2Core.getSettingsDir() + "QMPlay2Installer.exe");

    setWindowTitle(tr("QMPlay2 updates"));

    infoL = new QLabel(windowTitle());
    progressB = new QProgressBar;
    downloadUpdateB = new QPushButton(tr("Download update"));
    installB = new QPushButton(tr("Apply update"));
    connect(downloadUpdateB, SIGNAL(clicked()), this, SLOT(downloadUpdate()));
    connect(installB, SIGNAL(clicked()), this, SLOT(applyUpdate()));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(2);
    layout->addWidget(infoL);
    layout->addWidget(downloadUpdateB);
    layout->addWidget(progressB);
    layout->addWidget(installB);

    progressB->hide();
    installB->hide();

    resize(350, 0);
#endif
}
Updater::~Updater()
{
    if (busy)
    {
        infoFile.remove();
#ifdef UPDATER
        updateFile.remove();
#endif
    }
}

#ifdef UPDATER
bool Updater::downloading() const
{
    return busy && updateFile.isOpen() && updateFile.size() > 0;
}
#endif

void Updater::downloadUpdate()
{
    if (!busy && infoFile.open(QFile::WriteOnly | QFile::Truncate))
    {
#ifdef UPDATER
        if (sender() == downloadUpdateB)
            QMPlay2Core.getSettings().remove("UpdateVersion");

        infoL->setText(tr("Checking for updates"));
        progressB->setRange(0, 0);
        downloadUpdateB->hide();
        progressB->show();
#endif

        NetworkReply *reply = net.start("https://raw.githubusercontent.com/zaps166/QMPlay2OnlineContents/master/Updater");
        connect(reply, SIGNAL(finished()), this, SLOT(infoFinished()));

        busy = true;
    }
}

void Updater::infoFinished()
{
    NetworkReply *reply = (NetworkReply *)sender();
    if (!reply->hasError())
    {
        infoFile.write(reply->readAll());
        infoFile.close();

        const QSettings info(infoFile.fileName(), QSettings::IniFormat, this);

        QString osName;
#ifdef Q_OS_WIN
        if (QSysInfo::windowsVersion() != QSysInfo::WV_XP && QSysInfo::windowsVersion() != QSysInfo::WV_2003)
        {
            if (QSysInfo::WordSize == 32)
            {
                using IsWow64ProcessProc = BOOL(WINAPI *)(HANDLE, BOOL *);
                if (auto IsWow64Process = (IsWow64ProcessProc)GetProcAddress(GetModuleHandleA("kernel32"), "IsWow64Process"))
                {
                    BOOL bIsWow64 = FALSE;
                    if (IsWow64Process(GetCurrentProcess(), &bIsWow64) && bIsWow64)
                        osName = "Win64";
                }
                if (osName.isEmpty())
                    osName = "WinNew32";
            }
            else
            {
                osName = "Win64";
            }
        }
        else
        {
            osName = "Win32";
        }
#endif

        const QString NewVersion = info.value("Version" + osName).toString();
        const QDate NewVersionDate = Functions::parseVersion(NewVersion);

        if (NewVersionDate.isValid())
        {
            Settings &settings = QMPlay2Core.getSettings();
            if (NewVersionDate > Functions::parseVersion(settings.getString("Version")))
            {
                bool canUpdate = true;
                if (settings.contains("UpdateVersion"))
                {
                    const QDate UpdateVersionDate = Functions::parseVersion(settings.getString("UpdateVersion"));
                    if (UpdateVersionDate.isValid() && UpdateVersionDate == NewVersionDate)
                        canUpdate = false;
                }
                if (canUpdate)
                {
                    const QString updateIsAvailable = tr("Update is available for QMPlay2!");

                    const auto notify = [&] {
                        Notifies::notify(updateIsAvailable, tr("New QMPlay2 version: %1").arg(NewVersion), 0, 1);
                        settings.set("UpdateVersion", NewVersion);
                    };

#ifdef UPDATER
                    QString FileURL = info.value(osName).toString();
                    if (FileURL.isEmpty())
                        endWork(tr("No update available"));
                    else if (Version::isPortable())
                    {
                        notify();
                        endWork(updateIsAvailable);
                    }
                    else if (updateFile.open(QFile::WriteOnly | QFile::Truncate))
                        getFile(FileURL.replace("$Version", NewVersion))->setProperty("UpdateVersion", NewVersion);
                    else
                        endWork(tr("Error creating update file"));
#else
                    notify();
#endif
                }
                else
                {
                    endWork(tr("This auto-update is ignored, press the button to update"));
                }
            }
            else
            {
                endWork(tr("Application is up-to-date"));
            }
        }
        else
        {
            endWork(tr("Error checking for updates"));
        }
    }
    else
    {
        endWork(tr("Error checking for updates"));
    }
    infoFile.remove();
    reply->deleteLater();
}
#ifdef UPDATER
void Updater::writeToFile()
{
    NetworkReply *reply = (NetworkReply *)sender();
    const QByteArray arr = reply->readAll();
    bool err = false;
    if (firstChunk)
    {
        err = !arr.startsWith("MZ");
        firstChunk = false;
    }
    if (err || updateFile.write(arr) != arr.size())
        reply->abort();
}
void Updater::downloadprogress(int bytesReceived, int bytesTotal)
{
    progressB->setMaximum(bytesTotal);
    progressB->setValue(bytesReceived);
    writeToFile();
}
void Updater::downloadFinished()
{
    NetworkReply *reply = (NetworkReply *)sender();
    if (updateFile.size() && !reply->hasError())
    {
        Settings &settings = QMPlay2Core.getSettings();
        updateFile.close();
        settings.set("UpdateFile", updateFile.fileName());
        settings.set("UpdateVersion", reply->property("UpdateVersion").toString());
        infoL->setText(tr("The update has been downloaded"));
        installB->show();
        busy = false;
    }
    else
    {
        updateFile.remove();
        endWork(tr("Error downloading the update"));
    }
    reply->deleteLater();
}

void Updater::applyUpdate()
{
    QMPlay2GUI.runUpdate(updateFile.fileName());
    close();
    QMPlay2GUI.mainW->close();
}

NetworkReply *Updater::getFile(const QString &url)
{
    NetworkReply *reply = net.start(url);
    connect(reply, SIGNAL(downloadProgress(int, int)), this, SLOT(downloadprogress(int, int)));
    connect(reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
    firstChunk = true;
    return reply;
}
void Updater::endWork(const QString &str)
{
    infoL->setText(str);
    progressB->hide();
    downloadUpdateB->show();
    busy = false;
}
#endif
