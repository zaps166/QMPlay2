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

#pragma once

#include <QMPlay2Extensions.hpp>
#include <IOController.hpp>

#include <QTreeWidget>
#include <QToolButton>
#include <QThread>

class QLabel;
class QGridLayout;
class QProgressBar;
class QTreeWidgetItem;
class DownloaderThread;

class DownloadItemW : public QWidget
{
	Q_OBJECT
public:
	DownloadItemW(DownloaderThread *downloaderThr, QString name, const QIcon &icon = QIcon(), QDataStream *stream = nullptr);
	~DownloadItemW() final;

	void setName(const QString &);
	void setSizeAndFilePath(qint64, const QString &);
	void setPos(int);
	void setSpeed(int);
	void finish(bool f = true);
	void error();

	inline QString getFilePath() const
	{
		return filePath;
	}

	inline bool isFinished() const
	{
		return finished;
	}

	inline void ssBEnable()
	{
		ssB->setEnabled(true);
	}

	void write(QDataStream &);

	bool dontDeleteDownloadThr;
signals:
	void start();
	void stop();
private slots:
	void toggleStartStop();
private:
	void downloadStop(bool);

	DownloaderThread *downloaderThr;

	QLabel *titleL, *sizeL, *iconL;
	QToolButton *ssB;

	class SpeedProgressWidget : public QWidget
	{
	public:
		~SpeedProgressWidget() final = default;

		QLabel *speedL;
		QProgressBar *progressB;
	} *speedProgressW;

	bool finished, readyToPlay;
	QString filePath;
};

/**/

class DownloadListW : public QTreeWidget
{
	friend class Downloader;
public:
	inline QString getDownloadsDirPath()
	{
		return downloadsDirPath;
	}
private:
	QString downloadsDirPath;
};

/**/

class DownloaderThread : public QThread
{
	Q_OBJECT
	enum {ADD_ENTRY, NAME, SET, SET_POS, SET_SPEED, DOWNLOAD_ERROR, FINISH};
public:
	DownloaderThread(QDataStream *, const QString &, DownloadListW *, const QString &name = QString(), const QString &prefix = QString(), const QString &param = QString());
	~DownloaderThread() final;

	void write(QDataStream &stream)
	{
		stream << url << prefix << param;
	}
signals:
	void listSig(int, qint64 val = 0, const QString &filePath = QString());
private slots:
	void listSlot(int, qint64, const QString &);
	void stop();
	void finished();
private:
	void run() override final;

	QIcon getIcon();

	QString url, name, prefix, param;
	DownloadItemW *downloadItemW;
	DownloadListW *downloadLW;
	QTreeWidgetItem *item;
	IOController<> ioCtrl;
};

/**/

class Downloader : public QWidget, public QMPlay2Extensions
{
	Q_OBJECT

public:
	Downloader(Module &module);
	~Downloader() final;

	void init() override final;

	DockWidget *getDockWidget() override final;

	QVector<QAction *> getActions(const QString &, double, const QString &, const QString &, const QString &) override final;

private slots:
	void setDownloadsDir();
	void clearFinished();
	void addUrl();
	void download();
	void itemDoubleClicked(QTreeWidgetItem *);

private:
	DockWidget *dw;

	QGridLayout *layout;
	DownloadListW *downloadLW;
	QToolButton *setDownloadsDirB, *clearFinishedB, *addUrlB;
};

#define DownloaderName "QMPlay2 Downloader"
