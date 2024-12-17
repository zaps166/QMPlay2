/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

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

#include <OpenSubtitles.hpp>

#include <NetworkAccess.hpp>
#include <QMPlay2Core.hpp>
#include <LineEdit.hpp>

#include <QDesktopServices>
#include <QXmlStreamReader>
#include <QStringListModel>
#include <QJsonDocument>
#include <QGridLayout>
#include <QTreeWidget>
#include <QHeaderView>
#include <QScrollBar>
#include <QJsonArray>
#include <QCompleter>
#include <QComboBox>
#include <QTimer>
#include <QDebug>
#include <QFile>

enum
{
    SubItemUrlRole = Qt::UserRole + 0,
    SubItemUrlRoleBak = Qt::UserRole + 1,
    DownloadUrlRole = Qt::UserRole + 2,
    ToUrlRole = Qt::UserRole + 3,
};

static inline QString getBaseUrl()
{
    return QStringLiteral("https://www.opensubtitles.org");
}
static inline QString getSuggestUrl(const QString &text, const QString &lang)
{
    return getBaseUrl() + QStringLiteral("/libs/suggest.php?format=json3&MovieName=%1&SubLanguageID=%2").arg(text.toUtf8().toPercentEncoding(), lang);
}
static inline QString getSearchUrl(const QString &text, const QString &lang)
{
    return getBaseUrl() + QStringLiteral("/search2/moviename-%1/sublanguageid-%2/xml").arg(text.toUtf8().toPercentEncoding(), lang);
}

struct OpenSubtitles::Subtitle
{
    QString name;
    QString releaseName;
    QString episodeName;
    QString format;
    QString downloadsCount;
    QString subDate;
    QString movieYear;
    QString seriesSeason;
    QString seriesSubtitles;
    QString episodeUrl;
    QString movieUrl;
    QString thumbUrl;
    QString downloadUrl;
};

OpenSubtitles::OpenSubtitles(Module &module, const QIcon &icon)
    : m_icon(icon)
    , m_dw(new DockWidget)
    , m_net(new NetworkAccess(this))
    , mSearchEdit(new LineEdit(this))
    , m_completerTimer(new QTimer(this))
    , mLanguages(new QComboBox(this))
    , m_results(new QTreeWidget(this))
{
    SetModule(module);

    m_dw->setWindowTitle(tr("Subtitles browser"));
    m_dw->setObjectName(OpenSubtitlesName);
    m_dw->setWidget(this);

    auto completer = new QCompleter(mSearchEdit);
    completer->setModel(new QStringListModel(completer));
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionMode(QCompleter::PopupCompletion);

    m_completerTimer->setSingleShot(true);
    m_completerTimer->setInterval(250);
    connect(m_completerTimer, &QTimer::timeout, this, [this] {
        complete();
    });

    mSearchEdit->setPlaceholderText(tr("Type the movie name and press enter"));
    mSearchEdit->setCompleter(completer);
    connect(mSearchEdit, &LineEdit::clearButtonClicked, this, [this] {
        clearCompleter();
        search();
    });
    connect(mSearchEdit, &QLineEdit::textEdited, this, [this](const QString &text) {
        Q_UNUSED(text);
        m_dontComplete = false;
        m_completerTimer->start();
    });
    connect(mSearchEdit, &QLineEdit::returnPressed, this, [this] {
        m_dontComplete = true;
        search();
    });

    const auto lang = sets().getString(QStringLiteral("Language"), QStringLiteral("eng"));
    mLanguages->setMaximumSize(100, mLanguages->maximumHeight());
    for (auto &&[s, l] : QMPlay2Core.getLanguagesMap().asKeyValueRange())
    {
        if (!l.contains(",") && !l.contains(";") && !l.endsWith("languages"))
        {
            if (int i = l.indexOf("("); i > -1)
            {
                l.remove(i, l.size() - i);
            }
            mLanguages->addItem(l, s);
            if (s == lang)
            {
                mLanguages->setCurrentIndex(mLanguages->count() - 1);
            }
        }
    }
    connect(mLanguages, &QComboBox::currentIndexChanged, this, [this](int idx) {
        Q_UNUSED(idx)
        sets().set(QStringLiteral("Language"), getLanguage());
        search();
    });

    m_results->setIconSize(QSize(47, 47));
    m_results->setSortingEnabled(true);
    m_results->setColumnCount(3);
    m_results->setAnimated(true);
    connect(m_results, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem *twi) {
        if (Q_UNLIKELY(!twi))
            return;

        if (twi->childCount() == 1)
        {
            const auto subItem = twi->child(0);
            const auto url = twi->data(0, SubItemUrlRole).toString();
            if (!url.isEmpty())
            {
                subItem->setText(0, tr("Loading..."));
                twi->setData(0, SubItemUrlRole, QVariant());
                twi->setData(0, SubItemUrlRoleBak, url);
                loadSubItem(url, m_results->indexFromItem(twi), false);
            }
        }
    });
    connect(m_results, &QTreeWidget::itemDoubleClicked, this, [](QTreeWidgetItem *twi, int column) {
        Q_UNUSED(column);

        if (Q_UNLIKELY(!twi))
            return;

        if (!twi->parent() || Q_UNLIKELY(twi->childCount() != 0))
            return;

        const auto url = twi->data(0, DownloadUrlRole).toString();
        if (url.isEmpty())
            return;

        QDesktopServices::openUrl(url);
    });

    connect(m_results->verticalScrollBar(), &QScrollBar::rangeChanged, this, [this](int min, int max) {
        Q_UNUSED(min)
        Q_UNUSED(max)
        scanForToUrl();
    });
    connect(m_results->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        Q_UNUSED(value)
        scanForToUrl();
    });

    const auto headerItem = m_results->headerItem();
    headerItem->setText(0, tr("Movie name"));
    headerItem->setText(1, tr("Uploaded"));
    headerItem->setText(2, tr("Downloaded"));

    const auto header = m_results->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSortIndicator(2, Qt::DescendingOrder);

    auto layout = new QGridLayout(this);
    layout->addWidget(mSearchEdit, 0, 0, 1, 1);
    layout->addWidget(mLanguages, 0, 1, 1, 1);
    layout->addWidget(m_results, 1, 0, 1, 2);
}
OpenSubtitles::~OpenSubtitles()
{
}

DockWidget *OpenSubtitles::getDockWidget()
{
    return m_dw;
}

QVector<QAction *> OpenSubtitles::getActions(const QString &name, double, const QString &url, const QString &, const QString &)
{
    if (name != url)
    {
        auto act = new QAction(tr("Search on OpenSubtitles"), nullptr);
        act->connect(act, &QAction::triggered, this, [this, name](bool checked) {
            Q_UNUSED(checked)
            clearCompleter();
            mSearchEdit->setText(name);
            search();
            m_dw->raise();
        });
        act->setIcon(m_icon);
        return {act};
    }
    return {};
}

void OpenSubtitles::parseCompleterJson(const QByteArray &data)
{
    QStringList titles;

    const auto array = QJsonDocument::fromJson(data).array();
    titles.reserve(array.size());
    for (auto &&element : array)
    {
        const auto title = element[QStringLiteral("name")].toString();
        if (!title.isEmpty())
        {
            titles.push_back(title);
        }
    }

    const auto completer = mSearchEdit->completer();
    static_cast<QStringListModel *>(completer->model())->setStringList(titles);
    if (titles.isEmpty())
    {
        completer->popup()->close();
    }
    else if (!m_dontComplete && mSearchEdit->hasFocus() && !completer->popup()->isVisible())
    {
        completer->complete();
    }
}
void OpenSubtitles::parseXml(const QByteArray &data, QTreeWidgetItem *parentTwi)
{
    QXmlStreamReader xml(data);

    QVector<Subtitle> subtitles;
    Subtitle *subtitle = nullptr;

    bool inSubtitle = false;
    bool inResultsFound = false;

    QString tmpToUrl;
    auto &toUrl = parentTwi
        ? tmpToUrl
        : m_toUrl
    ;
    toUrl.clear();

    int downloadCountFieldWidth = 0;

    while (!xml.atEnd())
    {
        const auto tokenType = xml.readNext();
        const auto name = xml.name();
        const auto attrs = xml.attributes();

        const bool isResultsFound = (name.compare(QStringLiteral("results_found"), Qt::CaseInsensitive) == 0);
        const bool isSubtitle = (name.compare(QStringLiteral("subtitle"), Qt::CaseInsensitive) == 0);

        if (tokenType != QXmlStreamReader::StartElement)
        {
            if (tokenType == QXmlStreamReader::EndElement)
            {
                if (inResultsFound && isResultsFound)
                {
                    inResultsFound = false;
                }
                else if (inSubtitle && isSubtitle)
                {
                    subtitle = nullptr;
                    inSubtitle = false;
                }
            }
            continue;
        }

        if (inResultsFound)
        {
            if (name == QStringLiteral("to"))
            {
                if (toUrl.isEmpty())
                {
                    toUrl = attrs.value(QStringLiteral("Linker")).toString();
                    if (!toUrl.isEmpty())
                        toUrl.prepend(getBaseUrl());
                }
            }
        }
        else if (inSubtitle)
        {
            Q_ASSERT(subtitle);
            if (name == QStringLiteral("IDSubtitle"))
            {
                if (subtitle->downloadUrl.isEmpty())
                    subtitle->downloadUrl = attrs.value(QStringLiteral("LinkDownload")).toString();
            }
            else if (name == QStringLiteral("MovieName"))
            {
                if (subtitle->name.isEmpty())
                    subtitle->name = xml.readElementText();
            }
            else if (name == QStringLiteral("EpisodeName"))
            {
                if (subtitle->episodeName.isEmpty())
                    subtitle->episodeName = xml.readElementText();
                if (subtitle->episodeUrl.isEmpty())
                    subtitle->episodeUrl = getBaseUrl() + attrs.value(QStringLiteral("Link")).toString() + "/xml";
            }
            else if (name == QStringLiteral("MovieThumb"))
            {
                if (subtitle->thumbUrl.isEmpty())
                    subtitle->thumbUrl = xml.readElementText();
            }
            else if (name == QStringLiteral("MovieReleaseName"))
            {
                if (subtitle->releaseName.isEmpty())
                    subtitle->releaseName = xml.readElementText();
                if (subtitle->downloadUrl.isEmpty())
                    subtitle->downloadUrl = getBaseUrl() + attrs.value(QStringLiteral("Link")).toString();
            }
            else if (name == QStringLiteral("SubFormat"))
            {
                if (subtitle->format.isEmpty())
                    subtitle->format = xml.readElementText();
            }
            else if (name == QStringLiteral("SubDownloadsCnt"))
            {
                if (subtitle->downloadsCount.isEmpty())
                   subtitle->downloadsCount = xml.readElementText();
            }
            else if (name == QStringLiteral("SubDate"))
            {
                if (subtitle->subDate.isEmpty())
                    subtitle->subDate = xml.readElementText();
            }
            else if (name == QStringLiteral("MovieYear"))
            {
                if (subtitle->movieYear.isEmpty())
                    subtitle->movieYear = xml.readElementText();
            }
            else if (name == QStringLiteral("MovieID"))
            {
                if (subtitle->movieUrl.isEmpty())
                    subtitle->movieUrl = getBaseUrl() + attrs.value(QStringLiteral("Link")).toString();
            }
            else if (name == QStringLiteral("SeriesSeason"))
            {
                if (subtitle->seriesSeason.isEmpty())
                    subtitle->seriesSeason = xml.readElementText();
            }
            else if (name == QStringLiteral("SeriesSubtitles"))
            {
                if (subtitle->seriesSubtitles.isEmpty())
                    subtitle->seriesSubtitles = xml.readElementText();
            }
#if 0
            else if (name == QStringLiteral("SeriesDownloadsCnt"))
            {
                if (subtitle->downloadUrl.isEmpty())
                    subtitle->downloadUrl = attrs.value(QStringLiteral("LinkDownload")).toString();
                if (subtitle->downloadsCount.isEmpty())
                    subtitle->downloadsCount = xml.readElementText();
            }
            else if (name == QStringLiteral("Newest"))
            {
                if (subtitle->subDate.isEmpty())
                    subtitle->subDate = xml.readElementText();
            }
#endif
            else if (name == QStringLiteral("Subtitle"))
            {
                if (subtitle->downloadUrl.isEmpty())
                    subtitle->downloadUrl = attrs.value(QStringLiteral("LinkDownload")).toString();
                if (subtitle->subDate.isEmpty())
                    subtitle->subDate = attrs.value(QStringLiteral("SubAddDate")).toString();
            }
            downloadCountFieldWidth = qMax(downloadCountFieldWidth, subtitle->downloadsCount.size());
        }
        else if (isResultsFound)
        {
            inResultsFound = true;
        }
        else if (isSubtitle)
        {
            inSubtitle = true;
            subtitle = &subtitles.emplace_back();
        }
    }

    auto createTwi = [&] {
        if (parentTwi)
            return new QTreeWidgetItem(parentTwi);
        return new QTreeWidgetItem(m_results);
    };
    for (auto &&subtitle : std::as_const(subtitles))
    {
        if (!subtitle.episodeUrl.isEmpty())
        {
            if (subtitle.episodeName.isEmpty() || subtitle.seriesSubtitles.toInt() < 1)
            {
                continue;
            }

            auto twi = createTwi();
            if (subtitle.seriesSeason.toInt() > 0)
                twi->setText(0, tr("%1 (Season %2)").arg(subtitle.episodeName, subtitle.seriesSeason));
            else
                twi->setText(0, QStringLiteral("%1").arg(subtitle.episodeName));

            new QTreeWidgetItem(twi);
            twi->setData(0, SubItemUrlRole, subtitle.episodeUrl);
        }
        else if (!subtitle.downloadUrl.isEmpty())
        {
            auto name = subtitle.releaseName;
            if (name.isEmpty() || name == subtitle.movieYear)
            {
                name = subtitle.name;
            }
            if (name.isEmpty())
            {
                continue;
            }

            auto twi = createTwi();
            if (!subtitle.movieYear.isEmpty())
                twi->setText(0, QStringLiteral("%1 (%2)").arg(name, subtitle.movieYear));
            else
                twi->setText(0, QStringLiteral("%1").arg(name));
            twi->setText(1, subtitle.subDate);
            twi->setText(2, QStringLiteral("%1x (%2)").arg(subtitle.downloadsCount, downloadCountFieldWidth).arg(subtitle.format));

            twi->setData(0, DownloadUrlRole, subtitle.downloadUrl);

            twi->setTextAlignment(1, Qt::AlignCenter);
            twi->setTextAlignment(2, Qt::AlignCenter);

            if (parentTwi && !toUrl.isEmpty() && !parentTwi->parent())
            {
                parentTwi->setData(0, ToUrlRole, toUrl);
            }
        }
        else if (!subtitle.movieUrl.isEmpty())
        {
            if (subtitle.name.isEmpty())
            {
                continue;
            }

            auto twi = createTwi();
            twi->setText(0, subtitle.name + " (" + subtitle.movieYear + ")");

            if (subtitle.thumbUrl.startsWith(QStringLiteral("http")))
            {
                auto reply = m_net->start(subtitle.thumbUrl);
                m_loadThumbnailReplies.push_back(reply);
                connect(reply, &NetworkReply::finished, this, [this, reply, twi] {
                    if (!reply->hasError() && m_results->indexOfTopLevelItem(twi) > -1)
                    {
                        twi->setIcon(0, QIcon(QPixmap::fromImage(QImage::fromData(reply->readAll()))));
                    }
                    m_loadThumbnailReplies.removeOne(reply);
                    reply->deleteLater();
                });
            }

            new QTreeWidgetItem(twi);
            twi->setData(0, SubItemUrlRole, subtitle.movieUrl);
        }
    }
}

void OpenSubtitles::complete()
{
    if (m_completerReply)
    {
        m_completerReply->abort();
        m_completerReply.clear();
    }

    if (const auto text = mSearchEdit->text().simplified(); !text.isEmpty())
    {
        m_completerReply = m_net->start(getSuggestUrl(text, getLanguage()));
        connect(m_completerReply, &NetworkReply::finished, this, [this, completerReply = m_completerReply] {
            if (!completerReply->hasError())
            {
                parseCompleterJson(completerReply->readAll());
            }
            completerReply->deleteLater();
        });
    }
    else
    {
        parseCompleterJson({});
    }
}

void OpenSubtitles::scanForToUrl()
{
    const auto viewportRect = m_results->viewport()->rect();
    const int count = m_results->topLevelItemCount();
    for (int i = 0; i < count; ++i)
    {
        const auto twi = m_results->topLevelItem(i);
        const int childCount = twi->childCount();
        if (childCount > 0)
        {
            const auto twiChild = twi->child(childCount - 1);
            if (m_results->visualItemRect(twiChild).intersects(viewportRect))
            {
                const auto toUrl = twi->data(0, ToUrlRole).toString();
                if (!toUrl.isEmpty())
                {
                    twi->setData(0, ToUrlRole, QVariant());
                    loadSubItem(toUrl, m_results->indexFromItem(twi), true);
                }
            }
        }
    }

    const auto vsb = m_results->verticalScrollBar();
    if (!m_toUrl.isEmpty() && vsb->value() == vsb->maximum())
    {
        searchNext();
    }
}

void OpenSubtitles::search()
{
    if (m_searchReply)
    {
        m_searchReply->abort();
        m_searchReply.clear();
    }

    for (auto &&reply : std::as_const(m_loadSubItemReplies))
    {
        reply->abort();
    }
    m_loadSubItemReplies.clear();

    for (auto &&reply : std::as_const(m_loadThumbnailReplies))
    {
        reply->abort();
    }
    m_loadThumbnailReplies.clear();

    if (const auto text = mSearchEdit->text().simplified(); !text.isEmpty())
    {
        m_searchReply = m_net->start(getSearchUrl(text, getLanguage()));
        setCursor(Qt::BusyCursor);
        m_results->setEnabled(false);
        connect(m_searchReply, &NetworkReply::finished, this, [this, searchReply = m_searchReply] {
            if (!searchReply->hasError())
            {
                m_results->clear();
                parseXml(searchReply->readAll());
                m_results->scrollToTop();
            }
            setCursor(QCursor());
            m_results->setEnabled(true);
            searchReply->deleteLater();
        });
    }
    else
    {
        m_results->clear();
        m_results->setEnabled(true);
    }
}
void OpenSubtitles::searchNext()
{
    Q_ASSERT(!m_toUrl.isEmpty());

    if (m_searchReply)
    {
        m_searchReply->abort();
        m_searchReply.clear();
    }

    m_searchReply = m_net->start(m_toUrl);
    setCursor(Qt::BusyCursor);
    connect(m_searchReply, &NetworkReply::finished, this, [this, searchReply = m_searchReply] {
        if (!searchReply->hasError())
        {
            parseXml(searchReply->readAll());
        }
        setCursor(QCursor());
        searchReply->deleteLater();
    });

    m_toUrl.clear();
}
void OpenSubtitles::loadSubItem(const QString &url, const QPersistentModelIndex &index, bool next)
{
    auto reply = m_net->start(url);
    setCursor(Qt::BusyCursor);
    m_loadSubItemReplies.push_back(reply);
    connect(reply, &NetworkReply::finished, this, [this, reply, index, next] {
        const auto twi = m_results->itemFromIndex(index);
        if (Q_LIKELY(twi) && (next || Q_LIKELY(twi->childCount() == 1)))
        {
            auto loadingTwi = next
                ? nullptr
                : twi->child(0)
            ;
            if (!reply->hasError())
            {
                parseXml(reply->readAll(), twi);
                if (loadingTwi)
                {
                    twi->setData(0, SubItemUrlRoleBak, QVariant());
                    delete loadingTwi;
                    loadingTwi = nullptr;
                }
            }
            else if (loadingTwi)
            {
                loadingTwi->setText(0, tr("Error"));
                twi->setData(0, SubItemUrlRole, twi->data(0, SubItemUrlRoleBak));
                twi->setData(0, SubItemUrlRoleBak, QVariant());
            }
        }
        setCursor(QCursor());
        m_loadSubItemReplies.removeOne(reply);
        reply->deleteLater();
    });
}

inline void OpenSubtitles::clearCompleter()
{
    static_cast<QStringListModel *>(mSearchEdit->completer()->model())->setStringList({});
}

inline QString OpenSubtitles::getLanguage() const
{
    return mLanguages->currentData().toString();
}
