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

#pragma once

#include <QMPlay2Extensions.hpp>
#include <IOController.hpp>

#include <QTreeWidget>
#include <QToolButton>
#include <QThread>

class QLabel;
class QProcess;
class QGridLayout;
class QProgressBar;
class QTreeWidgetItem;
class DownloaderThread;

class DownloadItemW final : public QWidget
{
	Q_OBJECT
public:
	DownloadItemW(DownloaderThread *downloaderThr, QString name, const QIcon &icon, QDataStream *stream, QString preset);
	~DownloadItemW();

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

	void startConversion();
	void deleteConvertProcess();

	DownloaderThread *downloaderThr;

	QLabel *titleL, *sizeL, *iconL;
	QToolButton *ssB;

	class SpeedProgressWidget : public QWidget
	{
	public:
		~SpeedProgressWidget() final = default;

		QLabel *speedL;
		QProgressBar *progressB;
	} *speedProgressW = nullptr;

	QProcess *m_convertProcess = nullptr;
	QMetaObject::Connection m_convertProcessConn[2];
	bool finished, readyToPlay, m_needsConversion = false;
	QString m_convertPreset;
	QString filePath;
	QString m_convertedFilePath;
};

/**/

class DownloadListW final : public QTreeWidget
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

class DownloaderThread final : public QThread
{
	Q_OBJECT
	enum {ADD_ENTRY, NAME, SET, SET_POS, SET_SPEED, DOWNLOAD_ERROR, FINISH};
public:
	DownloaderThread(QDataStream *stream, const QString &url, DownloadListW *downloadLW, const QMenu *convertsMenu, const QString &name = QString(), const QString &prefix = QString(), const QString &param = QString(), const QString &preset = QString());
	~DownloaderThread();

	void serialize(QDataStream &stream);

	const QList<QAction *> convertActions();
signals:
	void listSig(int, qint64 val = 0, const QString &filePath = QString());
private slots:
	void listSlot(int, qint64, const QString &);
	void stop();
	void finished();
private:
	void run() override;

	QIcon getIcon();

	QString url, name, prefix, param, preset;
	DownloadItemW *downloadItemW;
	DownloadListW *downloadLW;
	QTreeWidgetItem *item;
	const QMenu *m_convertsMenu;
	IOController<> ioCtrl;
};

/**/

class Downloader final : public QWidget, public QMPlay2Extensions
{
	Q_OBJECT

public:
	Downloader(Module &module);
	~Downloader();

	void init() override;

	DockWidget *getDockWidget() override;

	QVector<QAction *> getActions(const QString &, double, const QString &, const QString &, const QString &) override;

private:
	void addConvertPreset();
	void editConvertAction();
	bool modifyConvertAction(QAction *action, bool addRemoveButton = true);

private slots:
	void setDownloadsDir();
	void clearFinished();
	void addUrl();
	void download();
	void itemDoubleClicked(QTreeWidgetItem *);

private:
	Settings m_sets;

	DockWidget *dw;

	QGridLayout *layout;
	DownloadListW *downloadLW;
	QToolButton *setDownloadsDirB, *clearFinishedB, *addUrlB;

	QToolButton *m_convertsPresetsB;
	QMenu *m_convertsMenu;
};

#define DownloaderName "QMPlay2 Downloader"
