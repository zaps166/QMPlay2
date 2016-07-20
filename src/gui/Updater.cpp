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

#include <Updater.hpp>

#include <Functions.hpp>
#include <Settings.hpp>
#include <Version.hpp>
#include <Main.hpp>
#include <CPU.hpp>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSettings>
#include <QLabel>
#include <QDir>

Updater::Updater(QWidget *parent) :
	QDialog(parent),
	net(this),
	busy(false)
{
	setWindowTitle(tr("QMPlay2 updates"));
	infoFile.setFileName(QDir::tempPath() + "/QMPlay2UpdaterURLs." + QString::number(qrand()));
	updateFile.setFileName(QMPlay2Core.getSettingsDir() + "QMPlay2Installer.exe");

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
}
Updater::~Updater()
{
	if (busy)
	{
		infoFile.remove();
		updateFile.remove();
	}
}

bool Updater::downloading() const
{
	return busy && updateFile.isOpen() && updateFile.size() > 0;
}

void Updater::downloadUpdate()
{
	if (!busy && infoFile.open(QFile::WriteOnly | QFile::Truncate))
	{
		if (sender() == downloadUpdateB)
			QMPlay2Core.getSettings().remove("UpdateVersion");

		QNetworkRequest request(QUrl("https://raw.githubusercontent.com/zaps166/QMPlay2OnlineContents/master/Updater"));
		request.setRawHeader("User-Agent", QMPlay2UserAgent);

		QNetworkReply *reply = net.get(request);
		connect(reply, SIGNAL(finished()), this, SLOT(infoFinished()));
		reply->ignoreSslErrors();

		infoL->setText(tr("Checking for updates"));
		progressB->setRange(0, 0);
		downloadUpdateB->hide();
		progressB->show();
		busy = true;
	}
}

void Updater::infoFinished()
{
	QNetworkReply *reply = (QNetworkReply *)sender();
	if (!reply->error())
	{
		infoFile.write(reply->readAll());
		infoFile.close();

		QSettings info(infoFile.fileName(), QSettings::IniFormat, this);

		Settings &settings = QMPlay2Core.getSettings();

		const QString NewVersion  = info.value("Version").toString();
		const QDate NewVersionDate = Functions::parseVersion(NewVersion);

		if (NewVersionDate.isValid())
		{
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
					QString FileURL = info.value(QString("Win%1").arg(sizeof(void *) << 3)).toString();
					if (FileURL.isEmpty())
						endWork(tr("No update available"));
					else if (updateFile.open(QFile::WriteOnly | QFile::Truncate))
						getFile(QUrl(FileURL.replace("$Version", NewVersion)))->setProperty("UpdateVersion", NewVersion);
					else
						endWork(tr("Error creating update file"));
				}
				else
					endWork(tr("This auto-update is ignored, press the button to update"));
			}
			else
				endWork(tr("Application is up-to-date"));
		}
		else
			endWork(tr("Error checking for updates"));
	}
	else
		endWork(tr("Error checking for updates"));
	infoFile.remove();
	reply->deleteLater();
}
void Updater::headerChanged()
{
	QNetworkReply *reply = (QNetworkReply *)sender();
	const QUrl newUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
	if (!newUrl.isEmpty() && reply->url() != newUrl)
	{
		disconnect(reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
		reply->abort();
		reply->deleteLater();
		getFile(newUrl);
	}
}
void Updater::writeToFile()
{
	QNetworkReply *reply = (QNetworkReply *)sender();
	QByteArray arr = reply->readAll();
	bool err = false;
	if (firstChunk)
	{
		err = !arr.startsWith("MZ");
		firstChunk = false;
	}
	if (err || updateFile.write(arr) != arr.size())
		reply->abort();
}
void Updater::downloadprogress(qint64 bytesReceived, qint64 bytesTotal)
{
	progressB->setMaximum(bytesTotal);
	progressB->setValue(bytesReceived);
}
void Updater::downloadFinished()
{
	QNetworkReply *reply = (QNetworkReply *)sender();
	if (updateFile.size() && !reply->error())
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

QNetworkReply *Updater::getFile(const QUrl &url)
{
	QNetworkRequest request(url);
	request.setRawHeader("User-Agent", QMPlay2UserAgent);
	QNetworkReply *reply = net.get(request);
	connect(reply, SIGNAL(metaDataChanged()), this, SLOT(headerChanged()));
	connect(reply, SIGNAL(readyRead()), this, SLOT(writeToFile()));
	connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(downloadprogress(qint64, qint64)));
	connect(reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
	reply->ignoreSslErrors();
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
