/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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
#include <QSet>

#include <vector>

class MediaBrowserJS;

/**/

class MediaBrowserResults final : public QTreeWidget
{
    Q_OBJECT

public:
    MediaBrowserResults(MediaBrowserJS *&mediaBrowser);
    ~MediaBrowserResults();

    void setCurrentName(const QString &name, const QString &pageName);

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

    MediaBrowserJS *&m_mediaBrowser;
    QString m_currentName;
    QMenu m_menu;
};

/**/

class QToolButton;
class QComboBox;

class MediaBrowserPages final : public QWidget
{
    Q_OBJECT

public:
    MediaBrowserPages();
    ~MediaBrowserPages();

    inline QComboBox *getPagesList() const;

    void setPage(const int page, bool gui);
    void setPages(const QStringList &pages);

    inline int getCurrentPage() const;
    inline QString getCurrentPageName() const;

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

class MediaBrowser final : public QWidget, public QMPlay2Extensions
{
    Q_OBJECT

public:
    MediaBrowser(Module &module);
    ~MediaBrowser();

private:
    DockWidget *getDockWidget() override;

    bool canConvertAddress() const override;

    QList<AddressPrefix> addressPrefixList(bool) const override;
    void convertAddress(const QString &prefix, const QString &url, const QString &param, QString *streamUrl, QString *name, QIcon *icon, QString *extension, IOController<> *ioCtrl) override;

    QVector<QAction *> getActions(const QString &, double, const QString &, const QString &, const QString &) override;

private:
    void initScripts();

    inline void setCompleterListCallback();
    void completionsReady();

    bool scanScripts();
    void downloadScripts(const QByteArray &jsonData);
    void saveScript(const QByteArray &data, const QString &scriptPath);

private slots:
    void visibilityChanged(bool v);

    void providerChanged(int idx);

    void searchTextEdited(const QString &text);
    void search();

    void netFinished(NetworkReply *reply);

    void searchMenu();

private:
    void loadSearchResults(const QByteArray &replyData = QByteArray());

    std::vector<MediaBrowserJS *> m_mediaBrowsers;
    MediaBrowserJS *m_mediaBrowser = nullptr;

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

    QPointer<NetworkReply> m_jsonReply, m_autocompleteReply, m_searchReply, m_imageReply;
    QSet<NetworkReply *> m_scriptReplies;
    NetworkAccess m_net;

    bool m_loadScripts = true, m_canUpdateScripts = false, m_updateScripts = true;
};

#define MediaBrowserName "MediaBrowser"
