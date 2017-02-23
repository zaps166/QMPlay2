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

class NetworkAccess;
class NetworkReply;
class QTreeWidget;

class MusicBrowserInterface
{
public:
	virtual ~MusicBrowserInterface() = default;


	virtual QString name() const = 0;
	virtual QIcon icon() const = 0;


	virtual void prepareWidget(QTreeWidget *treeW) = 0;


	virtual QString getQMPlay2Url(const QString &text) = 0;

	virtual NetworkReply *getSearchReply(const QString &text, const qint32 page, NetworkAccess &net) = 0;
	virtual void addSearchResults(const QByteArray &reply, QTreeWidget *treeW) = 0;

	virtual bool hasWebpage() = 0;
	virtual QString getWebpageUrl(const QString &text) = 0;

	virtual bool hasCompleter() = 0;
	virtual NetworkReply *getCompleterReply(const QString &text, NetworkAccess &net) = 0;
	virtual QStringList getCompletions(const QByteArray &reply) = 0;


	virtual QMPlay2Extensions::AddressPrefix addressPrefixList(bool img) const = 0;

	virtual QAction *getAction() const = 0;

	virtual bool convertAddress(const QString &prefix, const QString &url, QString *streamUrl, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl) = 0;
};
