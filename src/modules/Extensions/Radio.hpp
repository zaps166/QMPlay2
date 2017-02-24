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

class QListWidgetItem;
class QProgressBar;
class QListWidget;
class QLabel;
class NetworkAccess;

#include <QMenu>
#include <QIcon>

class Radio : public QWidget, public QMPlay2Extensions
{
	Q_OBJECT
public:
	Radio(Module &);
	~Radio();

	DockWidget *getDockWidget() override final;
private slots:
	void visibilityChanged(bool);
	void popup(const QPoint &);
	void removeStation();

	void openLink();

	void downloadProgress(int bytesReceived, int bytesTotal);
	void finished();
private:
	void addGroup(const QString &);
	void addStation(const QString &nazwa, const QString &URL, const QString &groupName, const QByteArray &img = QByteArray());

	DockWidget *dw;

	bool once;
	NetworkAccess *net;

	QListWidget *lW;
	QLabel *infoL;
	QProgressBar *progressB;

	QMenu popupMenu;

	QIcon qmp2Icon;
	const QString wlasneStacje;
	QListWidgetItem *nowaStacjaLWI;
};

#define RadioName "Radio Browser"
