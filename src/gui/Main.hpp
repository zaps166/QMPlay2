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

#define MUTEXWAIT_TIMEOUT 1250
#define TERMINATE_TIMEOUT (MUTEXWAIT_TIMEOUT*3)

#include <QMPlay2Core.hpp>

#include <QCoreApplication>

class VideoAdjustmentW;
class ShortcutHandler;
class QTreeWidgetItem;
class QTreeWidget;
class ScreenSaver;
class IPCServer;
class VideoDock;
class MenuBar;
class QWidget;

class QMPlay2GUIClass final : private QMPlay2CoreClass
{
	Q_DECLARE_TR_FUNCTIONS(QMPlay2GUIClass)

public:
	static QMPlay2GUIClass &instance();

	static QString getPipe();
	static void saveCover(QByteArray cover);

	static void setTreeWidgetItemIcon(QTreeWidgetItem *tWI, const QIcon &icon, const int column = 0, QTreeWidget *treeWidget = nullptr);

#ifdef UPDATER
	void runUpdate(const QString &);
#endif

	void setStyle();

	void loadIcons();
	void deleteIcons();

	QString getCurrentPth(QString pth = QString(), bool leaveFilename = false);
	void setCurrentPth(const QString &);

	void restoreGeometry(const QString &pth, QWidget *w, const int defaultSizePercent);

	inline QIcon getIcon(const QIcon &icon)
	{
		return icon.isNull() ? getQMPlay2Icon() : icon;
	}

	void updateInDockW();

	const QWidget *getVideoDock() const override;

	QColor grad1, grad2, qmpTxt;
	QIcon *groupIcon, *mediaIcon, *folderIcon;

	MenuBar *menuBar;
	QWidget *mainW;
	IPCServer *pipe;
	ScreenSaver *screenSaver;
	VideoAdjustmentW *videoAdjustment;
	ShortcutHandler *shortcutHandler;

	bool restartApp, removeSettings, noAutoPlay;
	QString newProfileName, cmdLineProfile;
private:
	QMPlay2GUIClass();
	~QMPlay2GUIClass();
};

#define QMPlay2GUI \
	QMPlay2GUIClass::instance()
