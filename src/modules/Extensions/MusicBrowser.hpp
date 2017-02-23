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

#include <NetworkAccess.hpp>

#include <QTreeWidget>
#include <QMenu>

#include <memory>
#include <vector>

class MusicBrowserInterface;

/**/

class MusicBrowserResults : public QTreeWidget
{
	Q_OBJECT

public:
	MusicBrowserResults(MusicBrowserInterface *&musicBrowser);
	~MusicBrowserResults();

private slots:
	void enqueue();
	void playCurrentEntry();
	void openPage();
	void copyPageURL();

	void playEntry(QTreeWidgetItem *tWI);

	void contextMenu(const QPoint &p);

private:
	MusicBrowserInterface *&m_musicBrowser;
	QMenu m_menu;
};

/**/

class QProgressBar;
class QToolButton;
class QCompleter;
class QComboBox;
class LineEdit;

class MusicBrowserW : public QWidget
{
	friend class MusicBrowser;
	Q_OBJECT

public:
	MusicBrowserW();
	~MusicBrowserW();

private slots:
	void providerChanged(int idx);

	void next();

	void searchTextEdited(const QString &text);
	void search();

	void netFinished(NetworkReply *reply);

	void searchMenu();

private:
	std::vector<std::unique_ptr<MusicBrowserInterface>> m_musicBrowsers;
	MusicBrowserInterface *m_musicBrowser;

	DockWidget *m_dW;

	QComboBox *m_providersB;
	LineEdit *m_searchE;
	QToolButton *m_searchB, *m_nextPageB;
	QProgressBar *m_progressB;
	MusicBrowserResults *m_resultsW;

	QCompleter *m_completer;
	QString m_lastName;
	qint32 m_currPage;

	NetworkReply *m_autocompleteReply, *m_searchReply;
	NetworkAccess m_net;
};

/**/

class MusicBrowser : public QMPlay2Extensions
{
public:
	MusicBrowser(Module &module);

	DockWidget *getDockWidget() override;

	QList<AddressPrefix> addressPrefixList(bool) const override;
	void convertAddress(const QString &prefix, const QString &url, const QString &param, QString *stream_url, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl) override;

	QVector<QAction *> getActions(const QString &, double, const QString &, const QString &, const QString &) override;
private:
	MusicBrowserW m_w;
};

#define MusicBrowserName "MusicBrowser"
