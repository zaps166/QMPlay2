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

#include <MusicBrowser/MusicBrowserInterface.hpp>

#include <QCoreApplication>

class SoundCloud : public MusicBrowserInterface
{
	Q_DECLARE_TR_FUNCTIONS(SoundCloud)

public:
	void prepareWidget(QTreeWidget *treeW) override final;


	QString name() const override final;
	QIcon icon() const override final;


	QString getQMPlay2Url(const QString &text) override final;

	NetworkReply *getSearchReply(const QString &text, const qint32 page, NetworkAccess &net) override final;
	void addSearchResults(const QByteArray &reply, QTreeWidget *treeW) override final;

	bool hasWebpage() override final;
	QString getWebpageUrl(const QString &text) override final;

	bool hasCompleter() override final;
	NetworkReply *getCompleterReply(const QString &text, NetworkAccess &net) override final;
	QStringList getCompletions(const QByteArray &reply) override final;


	QMPlay2Extensions::AddressPrefix addressPrefixList(bool img) const override final;

	QAction *getAction() const override final;

	bool convertAddress(const QString &prefix, const QString &url, QString *streamUrl, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl) override final;
};
