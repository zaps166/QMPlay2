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

#include <YouTube.hpp>

#include <YouTubeDL.hpp>
#include <LineEdit.hpp>

#include <QLoggingCategory>
#include <QStringListModel>
#include <QDesktopServices>
#include <QJsonParseError>
#include <QTextDocument>
#include <QJsonDocument>
#include <QProgressBar>
#include <QApplication>
#include <QJsonObject>
#include <QHeaderView>
#include <QGridLayout>
#include <QToolButton>
#include <QJsonArray>
#include <QCompleter>
#include <QClipboard>
#include <QMimeData>
#include <QSpinBox>
#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QUrl>

Q_LOGGING_CATEGORY(youtube, "Extensions/YouTube")

#define YOUTUBE_URL "https://www.youtube.com"

static inline QString toPercentEncoding(const QString &txt)
{
    return txt.toUtf8().toPercentEncoding();
}

static inline QString getYtUrl(const QString &title, const int sortByIdx)
{
    static constexpr const char *sortBy[4] {
        "",             // Relevance ("&sp=CAA%253D")
        "&sp=CAI%253D", // Upload date
        "&sp=CAM%253D", // View count
        "&sp=CAE%253D", // Rating
    };
    Q_ASSERT(sortByIdx >= 0 && sortByIdx <= 3);
    return QString(YOUTUBE_URL "/results?search_query=%1%2").arg(toPercentEncoding(title), sortBy[sortByIdx]);
}
static inline QString getAutocompleteUrl(const QString &text)
{
    return QString("http://suggestqueries.google.com/complete/search?client=firefox&ds=yt&q=%1").arg(toPercentEncoding(text));
}

static inline bool isPlaylist(QTreeWidgetItem *tWI)
{
    return tWI->data(1, Qt::UserRole).toBool();
}

/**/

ResultsYoutube::ResultsYoutube()
{
    setAnimated(true);
    setIndentation(12);
    setIconSize({100, 100});
    setExpandsOnDoubleClick(false);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    headerItem()->setText(0, tr("Title"));
    headerItem()->setText(1, tr("Length"));
    headerItem()->setText(2, tr("User"));

    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(0, QHeaderView::Stretch);
    header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(playEntry(QTreeWidgetItem *)));
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenu(const QPoint &)));
    setContextMenuPolicy(Qt::CustomContextMenu);
}
ResultsYoutube::~ResultsYoutube()
{}

void ResultsYoutube::playOrEnqueue(const QString &param, QTreeWidgetItem *tWI, const QString &addrParam)
{
    if (!tWI)
        return;
    if (!isPlaylist(tWI))
    {
        emit QMPlay2Core.processParam(param, "YouTube://{" + tWI->data(0, Qt::UserRole).toString() + "}" + addrParam);
    }
    else
    {
        const QStringList ytPlaylist = tWI->data(0, Qt::UserRole + 1).toStringList();
        QMPlay2CoreClass::GroupEntries entries;
        for (int i = 0; i < ytPlaylist.count() ; i += 2)
            entries += {ytPlaylist[i+1], "YouTube://{" YOUTUBE_URL "/watch?v=" + ytPlaylist[i+0] + "}" + addrParam};
        if (!entries.isEmpty())
        {
            const bool enqueue = (param == "enqueue");
            QMPlay2Core.loadPlaylistGroup(YouTubeName "/" + QString(tWI->text(0)).replace('/', '_'), entries, enqueue);
        }
    }
}

void ResultsYoutube::playEntry(QTreeWidgetItem *tWI)
{
    playOrEnqueue("open", tWI);
}

void ResultsYoutube::openPage()
{
    QTreeWidgetItem *tWI = currentItem();
    if (tWI)
        QDesktopServices::openUrl(tWI->data(0, Qt::UserRole).toString());
}
void ResultsYoutube::copyPageURL()
{
    QTreeWidgetItem *tWI = currentItem();
    if (tWI)
    {
        QMimeData *mimeData = new QMimeData;
        mimeData->setText(tWI->data(0, Qt::UserRole).toString());
        QApplication::clipboard()->setMimeData(mimeData);
    }
}

void ResultsYoutube::contextMenu(const QPoint &point)
{
    QTreeWidgetItem *tWI = currentItem();
    if (!tWI)
        return;

    const QString name = tWI->text(0);
    const QString url = tWI->data(0, Qt::UserRole).toString();

    auto menu = new QMenu(this);
    connect(menu, &QMenu::aboutToHide,
            menu, &QMenu::deleteLater);

    for (int i = 0; i < 2; ++i)
    {
        auto subMenu = menu->addMenu(
            (i == 0) ? QIcon(":/video.svgz") : QIcon(":/audio.svgz"),
            (i == 0) ? tr("Audio and video") : tr("Audio only")
        );

        if (!tWI->isDisabled())
        {
            const auto param = i == 0 ? QString() : QString("audio");
            subMenu->addAction(tr("Play"), this, [=] {
                playOrEnqueue("open", currentItem(), param);
            });
            subMenu->addAction(tr("Enqueue"), this, [=] {
                playOrEnqueue("enqueue", currentItem(), param);
            });
            subMenu->addSeparator();
        }

        if (i == 0)
        {
            subMenu->addAction(tr("Open the page in the browser"), this, SLOT(openPage()));
            subMenu->addAction(tr("Copy page address"), this, SLOT(copyPageURL()));
            subMenu->addSeparator();
        }

        if (isPlaylist(tWI))
            continue;

        for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
        {
            if (dynamic_cast<YouTube *>(QMPlay2Ext))
                continue;

            for (QAction *act : QMPlay2Ext->getActions(name, -2, url, "YouTube", i == 0 ? QString() : QString("audio")))
            {
                act->setParent(subMenu);
                subMenu->addAction(act);
            }
        }
    }

    menu->popup(viewport()->mapToGlobal(point));
}

/**/

PageSwitcher::PageSwitcher(YouTube *youTubeW)
{
    currPageB = new QLabel;

    nextB = new QToolButton;
    connect(nextB, &QToolButton::clicked,
            youTubeW, &YouTube::chPage);
    nextB->setAutoRaise(true);
    nextB->setArrowType(Qt::RightArrow);

    QHBoxLayout *hLayout = new QHBoxLayout(this);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->addWidget(currPageB);
    hLayout->addWidget(nextB);
}

/**/

const QStringList YouTube::getQualityPresets()
{
    return {
        "4320p 60FPS",
        "2160p 60FPS",
        "1440p 60FPS",
        "1080p 60FPS",
        "720p 60FPS",
        "2160p",
        "1440p",
        "1080p",
        "720p",
        "480p",
    };
}

YouTube::YouTube(Module &module) :
    completer(new QCompleter(new QStringListModel(this), this)),
    net(this)
{
    youtubeIcon = QIcon(":/youtube.svgz");
    videoIcon = QIcon(":/video.svgz");

    dw = new DockWidget;
    connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(setEnabled(bool)));
    dw->setWindowTitle("YouTube");
    dw->setObjectName(YouTubeName);
    dw->setWidget(this);

    completer->setCaseSensitivity(Qt::CaseInsensitive);

    searchE = new LineEdit;
#ifndef Q_OS_ANDROID
    connect(searchE, SIGNAL(textEdited(const QString &)), this, SLOT(searchTextEdited(const QString &)));
#endif
    connect(searchE, SIGNAL(clearButtonClicked()), this, SLOT(search()));
    connect(searchE, SIGNAL(returnPressed()), this, SLOT(search()));
    searchE->setCompleter(completer);

    searchB = new QToolButton;
    connect(searchB, SIGNAL(clicked()), this, SLOT(search()));
    searchB->setIcon(QMPlay2Core.getIconFromTheme("edit-find"));
    searchB->setToolTip(tr("Search"));
    searchB->setAutoRaise(true);

    QToolButton *showSettingsB = new QToolButton;
    connect(showSettingsB, &QToolButton::clicked, this, [] {
        emit QMPlay2Core.showSettings("Extensions");
    });
    showSettingsB->setIcon(QMPlay2Core.getIconFromTheme("configure"));
    showSettingsB->setToolTip(tr("Settings"));
    showSettingsB->setAutoRaise(true);

    m_qualityGroup = new QActionGroup(this);
    for (auto &&qualityPreset : getQualityPresets())
        m_qualityGroup->addAction(qualityPreset);

    QMenu *qualityMenu = new QMenu(this);
    int qualityIdx = 0;
    for (QAction *act : m_qualityGroup->actions())
    {
        connect(act, &QAction::triggered, this, [=] {
            sets().set("YouTube/QualityPreset", act->text());
        });
        connect(act, &QAction::toggled, this, [=](bool checked) {
            if (checked)
                setItags(qualityIdx);
        });
        act->setCheckable(true);
        qualityMenu->addAction(act);
        ++qualityIdx;
    }
    qualityMenu->insertSeparator(qualityMenu->actions().at(5));

    QToolButton *qualityB = new QToolButton;
    qualityB->setPopupMode(QToolButton::InstantPopup);
    qualityB->setToolTip(tr("Preferred quality"));
    qualityB->setIcon(QMPlay2Core.getIconFromTheme("video-display"));
    qualityB->setMenu(qualityMenu);
    qualityB->setAutoRaise(true);

    m_sortByGroup = new QActionGroup(this);
    m_sortByGroup->addAction(tr("Relevance"));
    m_sortByGroup->addAction(tr("Upload date"));
    m_sortByGroup->addAction(tr("View count"));
    m_sortByGroup->addAction(tr("Rating"));

    QMenu *sortByMenu = new QMenu(this);
    int sortByIdx = 0;
    for (QAction *act : m_sortByGroup->actions())
    {
        connect(act, &QAction::triggered, this, [=] {
            if (m_sortByIdx != sortByIdx)
            {
                m_sortByIdx = sortByIdx;
                sets().set("YouTube/SortBy", m_sortByIdx);
                search();
            }
        });
        act->setCheckable(true);
        sortByMenu->addAction(act);
        ++sortByIdx;
    }

    QToolButton *sortByB = new QToolButton;
    sortByB->setPopupMode(QToolButton::InstantPopup);
    sortByB->setToolTip(tr("Sort search results by ..."));
    {
        // FIXME: Add icon
        QFont f(sortByB->font());
        f.setBold(true);
        f.setPointSize(qMax(7, f.pointSize() - 1));
        sortByB->setFont(f);
        sortByB->setText("A-z");
    }
    sortByB->setMenu(sortByMenu);
    sortByB->setAutoRaise(true);

    resultsW = new ResultsYoutube;

    progressB = new QProgressBar;
    progressB->hide();

    pageSwitcher = new PageSwitcher(this);
    pageSwitcher->hide();

    connect(&net, SIGNAL(finished(NetworkReply *)), this, SLOT(netFinished(NetworkReply *)));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(showSettingsB, 0, 0, 1, 1);
    layout->addWidget(qualityB, 0, 1, 1, 1);
    layout->addWidget(sortByB, 0, 2, 1, 1);
    layout->addWidget(searchE, 0, 3, 1, 1);
    layout->addWidget(searchB, 0, 4, 1, 1);
    layout->addWidget(pageSwitcher, 0, 5, 1, 1);
    layout->addWidget(resultsW, 1, 0, 1, 6);
    layout->addWidget(progressB, 2, 0, 1, 6);
    layout->setSpacing(3);
    setLayout(layout);

    SetModule(module);
}
YouTube::~YouTube()
{}

bool YouTube::set()
{
    const auto qualityActions = m_qualityGroup->actions();
    const auto qualityText = sets().getString("YouTube/QualityPreset");
    bool qualityActionChecked = false;
    if (!qualityText.isEmpty())
    {
        for (auto &&qualityAction : qualityActions)
        {
            if (qualityAction->text() == qualityText)
            {
                qualityAction->setChecked(true);
                qualityActionChecked = true;
                break;
            }
        }
    }
    if (!qualityActionChecked)
        qualityActions[3]->setChecked(true);

    resultsW->setColumnCount(sets().getBool("YouTube/ShowUserName") ? 3 : 2);
    m_allowSubtitles = sets().getBool("YouTube/Subtitles");
    m_sortByIdx = qBound(0, sets().getInt("YouTube/SortBy"), 3);
    m_sortByGroup->actions().at(m_sortByIdx)->setChecked(true);
    return true;
}

DockWidget *YouTube::getDockWidget()
{
    return dw;
}

bool YouTube::canConvertAddress() const
{
    return true;
}

QString YouTube::matchAddress(const QString &url) const
{
    const QUrl qurl(url);
    if (qurl.scheme().startsWith("http") && (qurl.host().contains("youtube.") || qurl.host().contains("youtu.be")))
        return "YouTube";
    if (qurl.scheme().startsWith("http") && qurl.host().contains("twitch.tv"))
        return "youtube-dl";
    return QString();
}
QList<YouTube::AddressPrefix> YouTube::addressPrefixList(bool img) const
{
    return {
        AddressPrefix("YouTube", img ? youtubeIcon : QIcon()),
        AddressPrefix("youtube-dl", img ? videoIcon : QIcon())
    };
}
void YouTube::convertAddress(const QString &prefix, const QString &url, const QString &param, QString *stream_url, QString *name, QIcon *icon, QString *extension, IOController<> *ioCtrl)
{
    if (!stream_url && !name && !icon)
        return;
    if (prefix == "YouTube")
    {
        if (icon)
            *icon = youtubeIcon;
        if (ioCtrl && (stream_url || name))
        {
            auto &youTubeDl = ioCtrl->toRef<YouTubeDL>();
            const QStringList youTubeVideo = getYouTubeVideo(param, url, youTubeDl);
            if (youTubeVideo.count() == 3)
            {
                if (stream_url)
                    *stream_url = youTubeVideo[0];
                if (name && !youTubeVideo[2].isEmpty())
                    *name = youTubeVideo[2];
                if (extension)
                    *extension = youTubeVideo[1];
            }
            youTubeDl.reset();
        }
    }
    else if (prefix == "youtube-dl")
    {
        if (icon)
            *icon = videoIcon;
        if (ioCtrl)
        {
            IOController<YouTubeDL> &youTubeDL = ioCtrl->toRef<YouTubeDL>();
            if (ioCtrl->assign(new YouTubeDL))
            {
                youTubeDL->addr(url, param, stream_url, name, extension);
                ioCtrl->reset();
            }
        }
    }
}

QVector<QAction *> YouTube::getActions(const QString &name, double, const QString &url, const QString &, const QString &)
{
    if (name != url)
    {
        QAction *act = new QAction(YouTube::tr("Search on YouTube"), nullptr);
        act->connect(act, SIGNAL(triggered()), this, SLOT(searchMenu()));
        act->setIcon(youtubeIcon);
        act->setProperty("name", name);
        return {act};
    }
    return {};
}

void YouTube::chPage()
{
    search();
}

void YouTube::searchTextEdited(const QString &text)
{
    if (autocompleteReply)
        autocompleteReply->deleteLater();
    if (text.isEmpty())
        ((QStringListModel *)completer->model())->setStringList({});
    else
        autocompleteReply = net.start(getAutocompleteUrl(text));
}
void YouTube::search()
{
    const QString title = searchE->text();
    deleteReplies();
    if (autocompleteReply)
        autocompleteReply->deleteLater();
    if (searchReply)
        searchReply->deleteLater();
    if (continuationReply)
        continuationReply->deleteLater();
    resultsW->clear();
    if (!title.isEmpty())
    {
        pageSwitcher->setEnabled(false);
        if (lastTitle != title || sender() == searchE || sender() == searchB || qobject_cast<QAction *>(sender()))
        {
            m_currPage = 1;
            searchReply = net.start(getYtUrl(title, m_sortByIdx), QByteArray(), "Cookie: \r\n");
        }
        else
        {
            continuationReply = net.start(
                QString(YOUTUBE_URL "/youtubei/v1/search?key=%1").arg(m_apiKey),
                getContinuationJson()
            );
        }
        progressB->setRange(0, 0);
        progressB->show();
    }
    else
    {
        pageSwitcher->hide();
        progressB->hide();
        clearContinuation();
    }
    lastTitle = title;
}

void YouTube::netFinished(NetworkReply *reply)
{
    if (reply->hasError())
    {
        if (reply == searchReply)
        {
            deleteReplies();
            resultsW->clear();
            lastTitle.clear();
            progressB->hide();
            pageSwitcher->hide();
            clearContinuation();
            emit QMPlay2Core.sendMessage(tr("Connection error"), YouTubeName, 3);
        }
    }
    else
    {
        QTreeWidgetItem *tWI = ((QTreeWidgetItem *)reply->property("tWI").value<void *>());
        const QByteArray replyData = reply->readAll();
        if (reply == autocompleteReply)
        {
            setAutocomplete(replyData);
        }
        else if (reply == searchReply)
        {
            m_apiKey = QRegularExpression(R"|("INNERTUBE_API_KEY"\s*:\s*"(.+?)")|").match(replyData).captured(1);
            m_clientName = QRegularExpression(R"|("INNERTUBE_CLIENT_NAME"\s*:\s*"(.+?)")|").match(replyData).captured(1);
            m_clientVersion = QRegularExpression(R"|("INNERTUBE_CLIENT_VERSION"\s*:\s*"(.+?)")|").match(replyData).captured(1);
            setSearchResults(getYtInitialData(replyData).object(), false);
        }
        else if (reply == continuationReply)
        {
            ++m_currPage;
            setSearchResults(QJsonDocument::fromJson(replyData).object(), true);
        }
        else if (linkReplies.contains(reply))
        {
            if (isPlaylist(tWI))
                preparePlaylist(replyData, tWI);
        }
        else if (imageReplies.contains(reply))
        {
            QPixmap p;
            if (p.loadFromData(replyData))
                tWI->setIcon(0, p);
        }
    }

    if (linkReplies.contains(reply))
    {
        linkReplies.removeOne(reply);
        progressB->setValue(progressB->value() + 1);
    }
    else if (imageReplies.contains(reply))
    {
        imageReplies.removeOne(reply);
        progressB->setValue(progressB->value() + 1);
    }

    if (progressB->isVisible() && linkReplies.isEmpty() && imageReplies.isEmpty())
        progressB->hide();

    reply->deleteLater();
}

void YouTube::searchMenu()
{
    const QString name = sender()->property("name").toString();
    if (!name.isEmpty())
    {
        if (!dw->isVisible())
            dw->show();
        dw->raise();
        searchE->setText(name);
        search();
    }
}

void YouTube::setItags(int qualityIdx)
{
#if 0 // Itag info (incomplete)
    Video + Audio:
    43 = 360p WebM (VP8 + Vorbis 128kbps)
    36 = 180p MP4 (MPEG4 + AAC 32kbps)
    22 = 720p MP4 (H.264 + AAC 192kbps)
    18 = 360p MP4 (H.264 + AAC 96kbps)
     5 = 240p FLV (FLV + MP3 64kbps)

    Video only:
    247 = Video  720p (VP9)
    248 = Video 1080p (VP9)
    271 = Video 1440p (VP9)
    313 = Video 2160p (VP9)
    272 = Video 4320p/2160p (VP9)

    302 = Video  720p 60fps (VP9)
    303 = Video 1080p 60fps (VP9)
    308 = Video 1440p 60fps (VP9)
    315 = Video 2160p 60fps (VP9)

    298 = Video  720p 60fps (H.264)
    299 = Video 1080p 60fps (H.264)

    135 = Video  480p (H.264)
    136 = Video  720p (H.264)
    137 = Video 1080p (H.264)
    264 = Video 1440p (H.264)
    266 = Video 2160p (H.264)

    170 = Video  480p (VP8)
    168 = Video  720p (VP8)
    170 = Video 1080p (VP8)

    Audio only:
    139 = Audio (AAC 48kbps)
    140 = Audio (AAC 128kbps)
    141 = Audio (AAC 256kbps) //?

    171 = Audio (Vorbis 128kbps)
    172 = Audio (Vorbis 256kbps) //?

    249 = Audio (Opus 50kbps)
    250 = Audio (Opus 70kbps)
    251 = Audio (Opus 160kbps)
#endif

    enum
    {
        _4320p60,
        _2160p60,
        _1440p60,
        _1080p60,
        _720p60,
        _2160p,
        _1440p,
        _1080p,
        _720p,
        _480p,
        QualityPresetsCount,
    };

    QList<int> qualityPresets[QualityPresetsCount];
    {
        qualityPresets[_720p60]  << 298 << 302;
        qualityPresets[_1080p60] << 299 << 303 << qualityPresets[_720p60];
        qualityPresets[_1440p60] << 308 << qualityPresets[_1080p60];
        qualityPresets[_2160p60] << 315 << qualityPresets[_1440p60];
        qualityPresets[_4320p60] << 272 << qualityPresets[_2160p60];

        qualityPresets[_480p]  << 135 << 134 << 133;
        qualityPresets[_720p]  << 136 << 247 << qualityPresets[_480p];
        qualityPresets[_1080p] << 137 << 248 << qualityPresets[_720p];
        qualityPresets[_1440p] << 264 << 271 << qualityPresets[_1080p];
        qualityPresets[_2160p] << 266 << 313 << qualityPresets[_1440p];

        // Append also non-60 FPS itags to 60 FPS itags
        qualityPresets[_720p60]  += qualityPresets[_720p];
        qualityPresets[_1080p60] += qualityPresets[_1080p];
        qualityPresets[_1440p60] += qualityPresets[_1440p];
        qualityPresets[_2160p60] += qualityPresets[_2160p];
        qualityPresets[_4320p60] += qualityPresets[_2160p];
    }

    QList<int> liveQualityPresets[QualityPresetsCount];
    {
        liveQualityPresets[_720p60]  << 300;
        liveQualityPresets[_1080p60] << 301 << liveQualityPresets[_720p60];
        liveQualityPresets[_1440p60] << liveQualityPresets[_1080p60];
        liveQualityPresets[_2160p60] << liveQualityPresets[_1440p60];
        liveQualityPresets[_4320p60] << liveQualityPresets[_2160p60];

        liveQualityPresets[_480p]  << 94 << 93 << 92 << 91;
        liveQualityPresets[_720p]  << 95 << liveQualityPresets[_480p];
        liveQualityPresets[_1080p] << 96 << liveQualityPresets[_720p];
        liveQualityPresets[_1440p] << 265 << liveQualityPresets[_1080p];
        liveQualityPresets[_2160p] << 267 << liveQualityPresets[_1440p];

        // Append also non-60 FPS itags to 60 FPS itags
        liveQualityPresets[_720p60]  += liveQualityPresets[_720p];
        liveQualityPresets[_1080p60] += liveQualityPresets[_1080p];
        liveQualityPresets[_1440p60] += liveQualityPresets[_1440p];
        liveQualityPresets[_2160p60] += liveQualityPresets[_2160p];
        liveQualityPresets[_4320p60] += liveQualityPresets[_2160p];
    }

    QMutexLocker locker(&m_itagsMutex);
    m_videoItags = qualityPresets[qualityIdx];
    m_audioItags = {251, 171, 140, 250, 249};
    m_hlsItags = liveQualityPresets[qualityIdx];
    m_singleUrlItags = {43, 18};
    if (qualityIdx != _480p)
        m_singleUrlItags.prepend(22);
}

void YouTube::deleteReplies()
{
    while (!linkReplies.isEmpty())
        linkReplies.takeFirst()->deleteLater();
    while (!imageReplies.isEmpty())
        imageReplies.takeFirst()->deleteLater();
}

void YouTube::clearContinuation()
{
    m_apiKey.clear();
    m_clientName.clear();
    m_clientVersion.clear();
    m_continuationToken.clear();
}
QByteArray YouTube::getContinuationJson()
{
    QJsonObject client;
    client["clientName"] = m_clientName;
    client["clientVersion"] = m_clientVersion;

    QJsonObject context;
    context["client"] = client;

    QJsonObject continuation;
    continuation["continuation"] = m_continuationToken;
    continuation["context"] = context;

    return QJsonDocument(continuation).toJson(QJsonDocument::Compact);
}

void YouTube::setAutocomplete(const QByteArray &data)
{
    QJsonParseError jsonErr;
    const QJsonDocument json = QJsonDocument::fromJson(data, &jsonErr);
    if (jsonErr.error != QJsonParseError::NoError)
    {
        qCWarning(youtube) << "Cannot parse autocomplete JSON:" << jsonErr.errorString();
        return;
    }
    const QJsonArray mainArr = json.array();
    if (mainArr.count() < 2)
    {
        qCWarning(youtube) << "Invalid autocomplete JSON array";
        return;
    }
    const QJsonArray arr = mainArr.at(1).toArray();
    if (arr.isEmpty())
        return;
    QStringList list;
    list.reserve(arr.count());
    for (const QJsonValue &val : arr)
        list += val.toString();
    ((QStringListModel *)completer->model())->setStringList(list);
    if (searchE->hasFocus())
        completer->complete();
}
void YouTube::setSearchResults(const QJsonObject &jsonObj, bool isContinuation)
{
    QJsonArray items;

    if (isContinuation)
    {
        const auto onResponseReceivedCommands = jsonObj
            ["onResponseReceivedCommands"].toArray()
        ;
        for (auto &&val : onResponseReceivedCommands)
        {
            items = val
                ["appendContinuationItemsAction"]
                ["continuationItems"].toArray()
            ;
            if (!items.isEmpty())
                break;
        }
    }
    else
    {
        items = jsonObj
            ["contents"]
            ["twoColumnSearchResultsRenderer"]
            ["primaryContents"]
            ["sectionListRenderer"]
            ["contents"].toArray()
        ;
    }

    for (const QJsonValue &obj : items)
    {
        const auto contents = obj
            ["itemSectionRenderer"]
            ["contents"].toArray()
        ;

        const auto token = obj
            ["continuationItemRenderer"]
            ["continuationEndpoint"]
            ["continuationCommand"]
            ["token"].toString()
        ;
        if (!token.isEmpty())
            m_continuationToken = token;

        for (auto &&obj : contents)
        {
            const auto videoRenderer = obj["videoRenderer"].toObject();
            const auto playlistRenderer = obj["playlistRenderer"].toObject();

            const bool isVideo = !videoRenderer.isEmpty() && playlistRenderer.isEmpty();

            QString title, contentId, length, user, publishTime, viewCount, thumbnail, url;

            if (isVideo)
            {
                title = videoRenderer["title"]["runs"].toArray().at(0)["text"].toString();
                contentId = videoRenderer["videoId"].toString();
                if (title.isEmpty() || contentId.isEmpty())
                    continue;

                length = videoRenderer["lengthText"]["simpleText"].toString();
                user = videoRenderer["ownerText"]["runs"].toArray().at(0)["text"].toString();
                publishTime = videoRenderer["publishedTimeText"]["simpleText"].toString();
                viewCount = videoRenderer["shortViewCountText"]["simpleText"].toString();
                thumbnail = videoRenderer["thumbnail"]["thumbnails"].toArray().at(0)["url"].toString();

                url = YOUTUBE_URL "/watch?v=" + contentId;
            }
            else
            {
                title = playlistRenderer["title"]["simpleText"].toString();
                contentId = playlistRenderer["playlistId"].toString();
                if (title.isEmpty() || contentId.isEmpty())
                    continue;

                user = playlistRenderer["longBylineText"]["runs"].toArray().at(0)["text"].toString();
                thumbnail = playlistRenderer
                    ["thumbnailRenderer"]
                    ["playlistVideoThumbnailRenderer"]
                    ["thumbnail"]
                    ["thumbnails"].toArray().at(0)
                    ["url"].toString()
                ;

                url = YOUTUBE_URL "/playlist?list=" + contentId;
            }

            auto tWI = new QTreeWidgetItem(resultsW);

            tWI->setText(0, title);
            tWI->setText(1, isVideo ? length : tr("Playlist"));
            tWI->setText(2, user);

            QString tooltip;
            tooltip += QString("%1: %2\n").arg(resultsW->headerItem()->text(0), tWI->text(0));
            tooltip += QString("%1: %2\n").arg(isVideo ? resultsW->headerItem()->text(1) : tr("Playlist"), isVideo ? tWI->text(1) : tr("yes"));
            tooltip += QString("%1: %2").arg(resultsW->headerItem()->text(2), tWI->text(2));
            if (isVideo)
            {
                tooltip += QString("\n%1: %2\n").arg(tr("Publish time"), publishTime);
                tooltip += QString("%1: %2").arg(tr("View count"), viewCount);
            }
            tWI->setToolTip(0, tooltip);

            tWI->setData(0, Qt::UserRole, url);
            tWI->setData(1, Qt::UserRole, !isVideo);

            if (!isVideo)
            {
                tWI->setDisabled(true);

                auto linkReply = net.start(url, QByteArray(), "Cookie: \r\n");
                linkReply->setProperty("tWI", QVariant::fromValue((void *)tWI));
                linkReplies += linkReply;
            }

            if (!thumbnail.isEmpty())
            {
                auto imageReply = net.start(thumbnail);
                imageReply->setProperty("tWI", QVariant::fromValue((void *)tWI));
                imageReplies += imageReply;
            }
        }
    }

    if (resultsW->topLevelItemCount() > 0)
    {
        if (!m_apiKey.isEmpty() && !m_clientName.isEmpty() && !m_clientVersion.isEmpty() && !m_continuationToken.isEmpty())
        {
            pageSwitcher->currPageB->setText(QString::number(m_currPage));
            pageSwitcher->setEnabled(true);
            pageSwitcher->show();
        }

        progressB->setMaximum(linkReplies.count() + imageReplies.count());
        progressB->setValue(0);
    }
}

QStringList YouTube::getYouTubeVideo(const QString &param, const QString &url, IOController<YouTubeDL> &youTubeDL)
{
    if (!youTubeDL.assign(new YouTubeDL))
        return {};

    const auto rawOutputs = youTubeDL->exec(url, {"--flat-playlist", "--write-sub", "-J"}, nullptr, true);
    if (rawOutputs.count() != 2)
        return {};

    const auto rawOutput = rawOutputs[0].toUtf8();
    if (rawOutput.isEmpty())
        return {};

    const auto &rawErrOutput = rawOutputs[1];

    const auto o = QJsonDocument::fromJson(rawOutput).object();
    if (o.isEmpty())
        return {};

    const auto formats = o["formats"].toArray();
    if (formats.isEmpty())
        return {};

    const bool hasTitle = !rawErrOutput.contains("Unable to extract video title", Qt::CaseInsensitive);
    const auto title = hasTitle ? o["title"].toString() : QString();

    const bool audioOnly = (param.compare("audio", Qt::CaseInsensitive) == 0);
    const bool isLive = o["is_live"].toBool();

    QStringList urls;
    QStringList exts;

    m_itagsMutex.lock();
    const auto videoItags = m_videoItags;
    const auto audioItags = m_audioItags;
    const auto hlsItags = m_hlsItags;
    const auto singleUrlItags = m_singleUrlItags;
    m_itagsMutex.unlock();

    QHash<int, QPair<QString, QString>> itagsData;

    for (auto &&formatVal : formats)
    {
        const auto format = formatVal.toObject();
        if (format.isEmpty())
            continue;

        const auto protocol = format["protocol"].toString();
        if (protocol.contains("dash", Qt::CaseInsensitive))
        {
            // Skip MP4 DASH, because it doesn't work properly (old comment, is it still valid?)
            continue;
        }

        const auto itag = format["format_id"].toString().toInt();
        const auto url = format["url"].toString();
        const auto ext = format["ext"].toString();
        if (itag != 0 && !url.isEmpty() && !ext.isEmpty())
            itagsData[itag] = {url, "." + ext};
    }

    auto appendUrl = [&](const QList<int> &itags) {
        for (auto &&itag : itags)
        {
            auto it = itagsData.constFind(itag);
            if (it != itagsData.cend())
            {
                urls += it->first;
                exts += it->second;
                break;
            }
        }
    };

    if (!isLive)
    {
        if (!audioOnly)
            appendUrl(videoItags);
        appendUrl(audioItags);
    }
    if (urls.count() != 1 + (audioOnly ? 0 : 1))
    {
        if (!urls.isEmpty())
            urls.clear();
        appendUrl(hlsItags);
        if (urls.isEmpty())
            appendUrl(singleUrlItags);
    }

    if (urls.isEmpty())
    {
        qCritical() << "YouTube :: Can't find desired format, available:" << itagsData.keys();
        return {};
    }

    const auto subtitles = o["subtitles"].toObject();
    QString lang = QMPlay2Core.getSettings().getString("SubtitlesLanguage");
    if (lang.isEmpty()) // Default language
        lang = QLocale::languageToString(QLocale::system().language());
    if (!audioOnly && m_allowSubtitles && !subtitles.isEmpty() && !lang.isEmpty())
    {
        // Try to convert full language name into short language code
        for (int i = QLocale::C + 1; i <= QLocale::LastLanguage; ++i)
        {
            const QLocale::Language ll = (QLocale::Language)i;
            if (lang == QLocale::languageToString(ll))
            {
                lang = QLocale(ll).name();
                const int idx = lang.indexOf('_');
                if (idx > -1)
                    lang.remove(idx, lang.length() - idx);
                break;
            }
        }

        auto subtitlesForLang = subtitles[lang].toArray();
        if (subtitlesForLang.isEmpty())
            subtitlesForLang = subtitles[QMPlay2Core.getLanguage()].toArray();

        for (auto &&subtitlesFmtVal : qAsConst(subtitlesForLang))
        {
            const auto subtitlesFmt = subtitlesFmtVal.toObject();
            if (subtitlesFmt.isEmpty())
                continue;

            const auto ext = subtitlesFmt["ext"].toString();
            if (ext != "vtt")
                continue;

            const auto url = subtitlesFmt["url"].toString();
            if (url.isEmpty())
                continue;

            urls += url;
            exts += ".vtt";
            break;
        }
    }

    Q_ASSERT(!urls.isEmpty());
    Q_ASSERT(urls.count() == exts.count());

    QStringList result;
    if (urls.count() == 1)
    {
        result += urls.at(0);
        result += exts.at(0);
    }
    else
    {
        QString url = "FFmpeg://{";
        for (auto &&urlPart : qAsConst(urls))
            url += "[" + urlPart + "]";
        url += "}";

        QString ext;
        for (auto &&extPart : qAsConst(exts))
            ext += "[" + extPart + "]";

        result += url;
        result += ext;
    }
    result += title;

    return result;
}

void YouTube::preparePlaylist(const QByteArray &data, QTreeWidgetItem *tWI)
{
    QStringList playlist;

    const auto contents = getYtInitialData(data)
        ["contents"]
        ["twoColumnBrowseResultsRenderer"]
        ["tabs"].toArray().at(0)
        ["tabRenderer"]
        ["content"]
        ["sectionListRenderer"]
        ["contents"].toArray().at(0)
        ["itemSectionRenderer"]
        ["contents"].toArray().at(0)
        ["playlistVideoListRenderer"]
        ["contents"].toArray()
    ;

    for (auto &&obj : contents)
    {
        const auto title = obj["playlistVideoRenderer"]["title"]["runs"].toArray().at(0)["text"].toString();
        const auto videoId = obj["playlistVideoRenderer"]["videoId"].toString();
        if (title.isEmpty() || videoId.isEmpty())
            continue;

        playlist += {
            videoId,
            title,
        };
    }

    if (!playlist.isEmpty())
    {
        tWI->setData(0, Qt::UserRole + 1, playlist);
        tWI->setDisabled(false);
    }
}

QJsonDocument YouTube::getYtInitialData(const QByteArray &data)
{
    int idx = data.indexOf("ytInitialData");
    if (idx < 0)
        return QJsonDocument();

    idx = data.indexOf("{", idx);
    if (idx < 0)
        return QJsonDocument();

    QJsonParseError e = {};
    auto jsonDoc = QJsonDocument::fromJson(data.mid(idx), &e);

    if (Q_UNLIKELY(e.error == QJsonParseError::NoError))
        return jsonDoc;

    if (e.error == QJsonParseError::GarbageAtEnd && e.offset > 0)
        return QJsonDocument::fromJson(data.mid(idx, e.offset));

    return QJsonDocument();
}
