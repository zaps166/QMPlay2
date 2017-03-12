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
#include <QPointer>
#include <QMenu>

#include <memory>
#include <vector>

class MediaBrowserCommon;

/**/

class MediaBrowserResults : public QTreeWidget
{
	Q_OBJECT

public:
	MediaBrowserResults(MediaBrowserCommon *&mediaBrowser);
	~MediaBrowserResults() final;

	inline void setCurrentName(const QString &name);

private slots:
	void enqueueSelected();
	void playSelected();
	void playAll();
	void openPage();
	void copyPageURL();

	void playEntry(QTreeWidgetItem *tWI);

	void contextMenu(const QPoint &p);

private:
	QList<QTreeWidgetItem *> getItems(bool selected) const;

	void QMPlay2Action(const QString &action, const QList<QTreeWidgetItem *> &items);

	MediaBrowserCommon *&m_mediaBrowser;
	QString m_currentName;
	QMenu m_menu;
};

/**/

class QToolButton;
class QComboBox;

class MediaBrowserPages : public QWidget
{
	Q_OBJECT

public:
	MediaBrowserPages();
	~MediaBrowserPages();

	void setPage(const int page, bool gui);
	void setPages(const QStringList &pages);

	inline int getCurrentPage() const;

private slots:
	void maybeSwitchPage();
	void prevPage();
	void nextPage();

signals:
	void pageSwitched();

private:
	void setPageInGui(const int page);

	void maybeSetCurrentPage(const int page);

	int getPageFromUi() const;

	QToolButton *m_prevPage, *m_nextPage;
	QLineEdit *m_currentPage;
	QComboBox *m_list;
	int m_page;
};

/**/

class QStringListModel;
class QProgressBar;
class QCompleter;
class QTextEdit;
class LineEdit;

class MediaBrowser : public QWidget, public QMPlay2Extensions
{
	Q_OBJECT

public:
	MediaBrowser(Module &module);
	~MediaBrowser() final;

private:
	bool set() override final;

	DockWidget *getDockWidget() override final;

	QList<AddressPrefix> addressPrefixList(bool) const override final;
	void convertAddress(const QString &prefix, const QString &url, const QString &param, QString *streamUrl, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl) override final;

	QVector<QAction *> getActions(const QString &, double, const QString &, const QString &, const QString &) override final;


	inline void setCompleterListCallback();
	void completionsReady();

private slots:
	void visibilityChanged(bool v);

	void providerChanged(int idx);

	void searchTextEdited(const QString &text);
	void search();

	void netFinished(NetworkReply *reply);

	void searchMenu();

private:
	std::vector<std::unique_ptr<MediaBrowserCommon>> m_mediaBrowsers;
	MediaBrowserCommon *m_mediaBrowser;

	DockWidget *m_dW;


	QComboBox *m_providersB, *m_searchCB;
	LineEdit *m_searchE;
	QToolButton *m_searchB, *m_loadAllB;
	MediaBrowserPages *m_pages;
	QProgressBar *m_progressB;
	MediaBrowserResults *m_resultsW;
	QTextEdit *m_descr;

	QStringListModel *m_completerModel;
	QCompleter *m_completer;
	QString m_lastName;

	QPointer<NetworkReply> m_autocompleteReply, m_searchReply, m_imageReply;
	NetworkAccess m_net;

	bool m_visible, m_first, m_overrideVisibility;
};

#define MediaBrowserName "MediaBrowser"
