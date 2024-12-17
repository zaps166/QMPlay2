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
    QString format;
    QString downloadsCount;
    QString subDate;
    QString movieYear;
    QString movieUrl;
    QString thumbUrl;
    QString downloadUrl;
};

OpenSubtitles::OpenSubtitles(Module &module)
    : m_dw(new DockWidget)
    , m_net(new NetworkAccess(this))
    , mSearchEdit(new LineEdit(this))
    , m_completerTimer(new QTimer(this))
    , mLanguages(new QComboBox(this))
    , mResults(new QTreeWidget(this))
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
    m_completerTimer->setInterval(500);
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
        mDontComplete = false;
        m_completerTimer->start();
    });
    connect(mSearchEdit, &QLineEdit::returnPressed, this, [this] {
        mDontComplete = true;
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

    mResults->setUniformRowHeights(true);
    mResults->setIconSize(QSize(47, 47));
    mResults->setColumnCount(3);
    mResults->setAnimated(true);
    connect(mResults, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem *twi) {
        if (Q_UNLIKELY(!twi))
            return;

        if (twi->childCount() == 1)
        {
            const auto subItem = twi->child(0);
            const auto url = subItem->data(0, SubItemUrlRole).toString();
            if (!url.isEmpty())
            {
                subItem->setText(0, tr("Loading..."));
                subItem->setData(0, SubItemUrlRole, QVariant());
                subItem->setData(0, SubItemUrlRoleBak, url);
                loadSubItem(url, twi);
            }
        }
    });
    connect(mResults, &QTreeWidget::itemDoubleClicked, this, [](QTreeWidgetItem *twi, int column) {
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

    connect(mResults->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        if (value >= mResults->verticalScrollBar()->maximum() - 1)
        {
            searchNext();
        }
    });

    const auto headerItem = mResults->headerItem();
    headerItem->setText(0, tr("Movie name"));
    headerItem->setText(1, tr("Uploaded"));
    headerItem->setText(2, tr("Downloaded"));

    const auto header = mResults->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    auto layout = new QGridLayout(this);
    layout->addWidget(mSearchEdit, 0, 0, 1, 1);
    layout->addWidget(mLanguages, 0, 1, 1, 1);
    layout->addWidget(mResults, 1, 0, 1, 2);
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
        });
        // act->setIcon();
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
    else if (!mDontComplete && mSearchEdit->hasFocus() && !completer->popup()->isVisible())
    {
        completer->complete();
    }
}
void OpenSubtitles::parseXml(const QByteArray &data, QTreeWidgetItem *twi)
{
    QXmlStreamReader xml(data);

    QVector<Subtitle> subtitles;
    Subtitle *subtitle = nullptr;

    bool inSubtitle = false;
    bool inResultsFound = false;

    mToUrl.clear();

    while (!xml.atEnd())
    {
        const auto tokenType = xml.readNext();
        const auto name = xml.name();
        const auto attrs = xml.attributes();

        const bool isResultsFound = (name == QStringLiteral("results_found"));
        const bool isSubtitle = (name == QStringLiteral("subtitle"));

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
                    if (subtitle->name.isEmpty())
                    {
                        subtitles.removeLast();
                    }
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
                if (mToUrl.isEmpty())
                {
                    mToUrl = attrs.value(QStringLiteral("Linker")).toString();
                    if (!mToUrl.isEmpty())
                        mToUrl.prepend(getBaseUrl());
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
            else if (name == QStringLiteral("MovieThumb"))
            {
                if (subtitle->thumbUrl.isEmpty())
                    subtitle->thumbUrl = xml.readElementText();
            }
            else if (name == QStringLiteral("MovieReleaseName"))
            {
                if (subtitle->releaseName.isEmpty())
                    subtitle->releaseName = xml.readElementText();
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
        }
        else if (name == QStringLiteral("results_found"))
        {
            inResultsFound = true;
        }
        else if (name == QStringLiteral("subtitle"))
        {
            inSubtitle = true;
            subtitle = &subtitles.emplace_back();
        }
    }

    for (auto &&subtitle : std::as_const(subtitles))
    {
        if (twi)
        {
            auto name = subtitle.releaseName;
            if (name.isEmpty() || name == subtitle.movieYear)
            {
                name = subtitle.name;
            }
            if (name.isEmpty() || subtitle.downloadUrl.isEmpty())
            {
                continue;
            }

            auto subItem = new QTreeWidgetItem(twi);
            subItem->setText(0, name + " (" + subtitle.movieYear + ")");
            subItem->setText(1, subtitle.subDate);
            subItem->setText(2, subtitle.downloadsCount + "x\n" + subtitle.format);

            subItem->setData(0, DownloadUrlRole, subtitle.downloadUrl);

            subItem->setTextAlignment(1, Qt::AlignCenter);
            subItem->setTextAlignment(2, Qt::AlignCenter);
        }
        else
        {
            if (subtitle.name.isEmpty() || subtitle.movieUrl.isEmpty())
            {
                continue;
            }

            auto twi = new QTreeWidgetItem(mResults);
            twi->setText(0, subtitle.name + " (" + subtitle.movieYear + ")");

            if (subtitle.thumbUrl.startsWith(QStringLiteral("http")))
            {
                auto reply = m_net->start(subtitle.thumbUrl);
                m_loadThumbnailReplies.push_back(reply);
                connect(reply, &NetworkReply::finished, this, [this, reply, twi] {
                    if (!reply->hasError() && mResults->indexOfTopLevelItem(twi) > -1)
                    {
                        twi->setIcon(0, QIcon(QPixmap::fromImage(QImage::fromData(reply->readAll()))));
                    }
                    m_loadThumbnailReplies.removeOne(reply);
                    reply->deleteLater();
                });
            }

            auto subItem = new QTreeWidgetItem(twi);
            subItem->setData(0, SubItemUrlRole, subtitle.movieUrl);
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
        mResults->setEnabled(false);
        connect(m_searchReply, &NetworkReply::finished, this, [this, searchReply = m_searchReply] {
            if (!searchReply->hasError())
            {
                mResults->clear();
                parseXml(searchReply->readAll());
                mResults->scrollToTop();
            }
            setCursor(QCursor());
            mResults->setEnabled(true);
            searchReply->deleteLater();
        });
    }
    else
    {
        mResults->clear();
        mResults->setEnabled(true);
    }
}
void OpenSubtitles::searchNext()
{
    if (mToUrl.isEmpty())
        return;

    if (m_searchReply)
    {
        m_searchReply->abort();
        m_searchReply.clear();
    }

    m_searchReply = m_net->start(mToUrl);
    setCursor(Qt::BusyCursor);
    connect(m_searchReply, &NetworkReply::finished, this, [this, searchReply = m_searchReply] {
        if (!searchReply->hasError())
        {
            parseXml(searchReply->readAll());
        }
        setCursor(QCursor());
        searchReply->deleteLater();
    });

    mToUrl.clear();
}
void OpenSubtitles::loadSubItem(const QString &url, QTreeWidgetItem *twi)
{
    auto reply = m_net->start(url);
    setCursor(Qt::BusyCursor);
    m_loadSubItemReplies.push_back(reply);
    connect(reply, &NetworkReply::finished, this, [this, reply, twi] {
        if (mResults->indexOfTopLevelItem(twi) > -1 && Q_LIKELY(twi->childCount() == 1))
        {
            auto loadingTwi = twi->child(0);
            if (!reply->hasError())
            {
                parseXml(reply->readAll(), twi);
                delete loadingTwi;
                loadingTwi = nullptr;
            }
            else
            {
                loadingTwi->setText(0, tr("Error"));
                loadingTwi->setData(0, SubItemUrlRole, loadingTwi->data(0, SubItemUrlRoleBak));
                loadingTwi->setData(0, SubItemUrlRoleBak, QVariant());
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
