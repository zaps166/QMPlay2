/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2021  Błażej Szczygieł

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
#include <QMutex>

class NetworkReply;
class QProgressBar;
class QActionGroup;
class QToolButton;
class QCompleter;
class YouTubeDL;
class QSpinBox;
class LineEdit;
class QLabel;

/**/

class ResultsYoutube final : public QTreeWidget
{
    Q_OBJECT
public:
    ResultsYoutube();
    ~ResultsYoutube();

signals:
    /// Signals when related items are requested for an item.
    /** Is listened to by YouTube::fetchRelated(). */
    void requestRelated(const QString contentId);

private:
    void playOrEnqueue(const QString &param, QTreeWidgetItem *tWI, const QString &addrParam = QString());

private slots:
    void playEntry(QTreeWidgetItem *tWI);

    void openPage();
    void copyPageURL();

    /// Called when user clicks on "Show related" in context menu, gets the item's contentId and triggers requestRelated().
    void showRelated();

    void contextMenu(const QPoint &p);
};

/**/

class YouTube;

class PageSwitcher final : public QWidget
{
    Q_OBJECT
public:
    PageSwitcher(YouTube *youTubeW);

    QToolButton *nextB;
    QLabel *currPageB;
};

/**/

using ItagNames = QPair<QStringList, QList<int>>;

class YouTube final : public QWidget, public QMPlay2Extensions
{
    Q_OBJECT

public:
    static const QStringList getQualityPresets();

public:
    YouTube(Module &module);
    ~YouTube();

    bool set() override;

    DockWidget *getDockWidget() override;

    bool canConvertAddress() const override;

    QString matchAddress(const QString &url) const override;
    QList<AddressPrefix> addressPrefixList(bool) const override;
    void convertAddress(const QString &, const QString &, const QString &, QString *, QString *, QIcon *, QString *, IOController<> *ioCtrl) override;

    QVector<QAction *> getActions(const QString &, double, const QString &, const QString &, const QString &) override;

public:
    void chPage();

private slots:
    void searchTextEdited(const QString &text);
    void search();

    /// Fetches related items for video given by contentId.
    /** Creates a network request for related items. The reply is handled by handleRelatedReply(). */
    void fetchRelated(const QString &contentId);

    /// Handles reply for request sent by fetchRelated().
    void handleRelatedReply(QByteArray replyData);

    void netFinished(NetworkReply *reply);

    void searchMenu();

private:
    void setItags(int qualityIdx);

    void deleteReplies();

    void clearContinuation();
    QByteArray getContinuationJson();

    void setAutocomplete(const QByteArray &data);
    void setSearchResults(const QJsonObject &jsonObj, bool isContinuation);

    /// Writes list of related items into YT browser widget.
    void setRelatedResults(const QJsonObject &jsonObj, bool isContinuation);

    QStringList getYouTubeVideo(const QString &param, const QString &url, IOController<YouTubeDL> &youTubeDL);

    void preparePlaylist(const QByteArray &data, QTreeWidgetItem *tWI);

    QJsonDocument getYtInitialData(const QByteArray &data);

private:
    DockWidget *dw;

    QIcon youtubeIcon, videoIcon;

    LineEdit *searchE;
    QToolButton *searchB;
    ResultsYoutube *resultsW;
    QProgressBar *progressB;
    PageSwitcher *pageSwitcher;

    QString lastTitle;
    QCompleter *completer;

    QPointer<NetworkReply> autocompleteReply, searchReply, continuationReply, relatedReply;
    QList<NetworkReply *> linkReplies, imageReplies;
    NetworkAccess net;

    bool m_allowSubtitles;

    QActionGroup *m_qualityGroup = nullptr, *m_sortByGroup = nullptr;

    int m_sortByIdx = 0;

    QMutex m_itagsMutex;
    bool m_preferredH264 = false;
    QVector<int> m_videoItags, m_audioItags, m_hlsItags, m_singleUrlItags;

    QString m_apiKey, m_clientName, m_clientVersion, m_continuationToken;
    int m_currPage = 1;
};

#define YouTubeName "YouTube Browser"
