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

#ifndef UPDATER_HPP
#define UPDATER_HPP

#include <QNetworkAccessManager>
#include <QDialog>
#include <QFile>

class QUrl;
class QLabel;
class QProgressBar;
class QPushButton;

class Updater : public QDialog
{
	Q_OBJECT
public:
	Updater(QWidget *);
	~Updater();

	bool downloading() const;
public slots:
	void downloadUpdate();
private slots:
	void infoFinished();
	void headerChanged();
	void writeToFile();
	void downloadprogress(qint64, qint64);
	void downloadFinished();

	void applyUpdate();
private:
	QNetworkReply *getFile(const QUrl &);
	void endWork(const QString &);

	QFile infoFile, updateFile;
	QNetworkAccessManager net;
	QLabel *infoL;
	QProgressBar *progressB;
	QPushButton *downloadUpdateB, *installB;
	bool busy, firstChunk;
};

#endif //UPDATER_HPP
