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

#include <MediaBrowser.hpp>

#include <MediaBrowserJS.hpp>
#include <Functions.hpp>
#include <LineEdit.hpp>
#include <Playlist.hpp>
#include <CPU.hpp>

#include <QStringListModel>
#include <QDesktopServices>
#include <QLoggingCategory>
#include <QApplication>
#include <QProgressBar>
#include <QGridLayout>
#include <QHeaderView>
#include <QToolButton>
#include <QCompleter>
#include <QClipboard>
#include <QTextFrame>
#include <QTextEdit>
#include <QComboBox>
#include <QMimeData>
#include <QSpinBox>
#include <QFile>
#include <QDir>
#include <QUrl>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

#include <algorithm>

Q_LOGGING_CATEGORY(mb, "MediaBrowser")

constexpr const char *g_mediaBrowserBaseUrl = "https://raw.githubusercontent.com/zaps166/QMPlay2OnlineContents/master/";
constexpr int g_mediaBrowserApiVersion = 1;

static inline QString getScriptsPath()
{
    return QMPlay2Core.getSettingsDir() + MediaBrowserName;
}

static inline QString getStringFromItem(QTreeWidgetItem *tWI)
{
    return tWI->data(0, Qt::UserRole).toString();
}

/**/

MediaBrowserResults::MediaBrowserResults(MediaBrowserJS *&mediaBrowser) :
    m_mediaBrowser(mediaBrowser)
{
    headerItem()->setHidden(true);

    connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(playEntry(QTreeWidgetItem *)));
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenu(const QPoint &)));
    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionMode(ExtendedSelection);
}
MediaBrowserResults::~MediaBrowserResults()
{}

void MediaBrowserResults::setCurrentName(const QString &name, const QString &pageName)
{
    m_currentName = name;
    if (!m_currentName.isEmpty())
    {
        if (m_currentName.at(0).isLower())
            m_currentName[0] = m_currentName.at(0).toUpper();
        m_currentName.replace('/', '_');
        if (!pageName.isEmpty())
            m_currentName.prepend(QString(pageName).replace('/', '_') + "/");
    }
}

void MediaBrowserResults::enqueueSelected()
{
    QMPlay2Action("enqueue", getItems(true));
}
void MediaBrowserResults::playSelected()
{
    QMPlay2Action("open", getItems(true));
}
void MediaBrowserResults::playAll()
{
    QMPlay2Action("open", getItems(false));
}
void MediaBrowserResults::openPage()
{
    if (m_mediaBrowser && m_mediaBrowser->hasWebpage())
    {
        if (QTreeWidgetItem *tWI = currentItem())
            QDesktopServices::openUrl(m_mediaBrowser->getWebpageUrl(getStringFromItem(tWI)));
    }
}
void MediaBrowserResults::copyPageURL()
{
    if (m_mediaBrowser && m_mediaBrowser->hasWebpage())
    {
        if (QTreeWidgetItem *tWI = currentItem())
        {
            QMimeData *mimeData = new QMimeData;
            mimeData->setText(m_mediaBrowser->getWebpageUrl(getStringFromItem(tWI)));
            QApplication::clipboard()->setMimeData(mimeData);
        }
    }
}

void MediaBrowserResults::playEntry(QTreeWidgetItem *tWI)
{
    QMPlay2Action("open", {tWI});
}

void MediaBrowserResults::contextMenu(const QPoint &point)
{
    m_menu.clear();
    if (!m_mediaBrowser)
        return;
    if (QTreeWidgetItem *tWI = currentItem())
    {
        m_menu.addAction(tr("Enqueue"), this, SLOT(enqueueSelected()));
        m_menu.addAction(tr("Play"), this, SLOT(playSelected()));
        m_menu.addSeparator();
        if (m_mediaBrowser->hasWebpage())
        {
            m_menu.addAction(tr("Open the page in the browser"), this, SLOT(openPage()));
            m_menu.addAction(tr("Copy page address"), this, SLOT(copyPageURL()));
            m_menu.addSeparator();
        }
        QString name = tWI->data(0, Qt::UserRole + 1).toString();
        if (name.isEmpty())
            name = tWI->text(0);
        for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
        {
            QString addressPrefixName, url, param;
            if (Functions::splitPrefixAndUrlIfHasPluginPrefix(m_mediaBrowser->getQMPlay2Url(getStringFromItem(tWI)), &addressPrefixName, &url, &param))
            {
                const bool self = dynamic_cast<MediaBrowser *>(QMPlay2Ext);
                for (QAction *act : QMPlay2Ext->getActions(name, -2, url, addressPrefixName, param))
                {
                    if (!self || act->property("ptr").value<quintptr>() != (quintptr)m_mediaBrowser)
                    {
                        act->setParent(&m_menu);
                        m_menu.addAction(act);
                    }
                }
            }
        }
        m_menu.popup(viewport()->mapToGlobal(point));
    }
}

QList<QTreeWidgetItem *> MediaBrowserResults::getItems(bool selected) const
{
    QList<QTreeWidgetItem *> items = selected ? selectedItems() : findItems(QString(), Qt::MatchContains);
    if (items.count() < 2)
        return {currentItem()};
    std::sort(items.begin(), items.end(), [](QTreeWidgetItem *a, QTreeWidgetItem *b) {
        return (a->text(0) < b->text(0));
    });
    return items;
}

void MediaBrowserResults::QMPlay2Action(const QString &action, const QList<QTreeWidgetItem *> &items)
{
    if (m_mediaBrowser && items.value(0))
    {
        if (items.count() == 1)
            emit QMPlay2Core.processParam(action, m_mediaBrowser->getQMPlay2Url(getStringFromItem(items[0])));
        else
        {
            QMPlay2CoreClass::GroupEntries entries;
            for (QTreeWidgetItem *tWI : items)
                entries += {tWI->text(0), m_mediaBrowser->getQMPlay2Url(getStringFromItem(tWI))};
            if (!entries.isEmpty())
            {
                const bool enqueue = (action == "enqueue");
                QMPlay2Core.loadPlaylistGroup(m_mediaBrowser->name() + "/" + m_currentName, entries, enqueue);
            }
        }
    }
}

/**/

MediaBrowserPages::MediaBrowserPages() :
    m_page(0)
{
    m_prevPage = new QToolButton;
    connect(m_prevPage, SIGNAL(clicked()), this, SLOT(prevPage()));
    m_prevPage->setArrowType(Qt::LeftArrow);
    m_prevPage->setAutoRaise(true);
    m_prevPage->hide();

    m_currentPage = new QLineEdit;
    connect(m_currentPage, SIGNAL(editingFinished()), this, SLOT(maybeSwitchPage()));
    m_currentPage->setFixedWidth(QFontMetrics(m_currentPage->font()).boundingRect('0').width() * 35 / 10);
    m_currentPage->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_currentPage->setValidator(new QIntValidator(1, 99, m_currentPage));
    m_currentPage->setContextMenuPolicy(Qt::NoContextMenu);
    m_currentPage->setMaxLength(2);
    m_currentPage->hide();

    m_nextPage = new QToolButton;
    connect(m_nextPage, SIGNAL(clicked()), this, SLOT(nextPage()));
    m_nextPage->setArrowType(Qt::RightArrow);
    m_nextPage->setAutoRaise(true);
    m_nextPage->hide();

    m_list = new QComboBox;
    connect(m_list, SIGNAL(activated(int)), this, SLOT(maybeSwitchPage()));
    m_list->hide();

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(m_prevPage, 0, 0, 1, 1);
    layout->addWidget(m_currentPage, 0, 1, 1, 1);
    layout->addWidget(m_nextPage, 0, 2, 1, 1);
    layout->setSpacing(3);
    layout->setMargin(0);
}
MediaBrowserPages::~MediaBrowserPages()
{}

inline QComboBox *MediaBrowserPages::getPagesList() const
{
    return m_list;
}

void MediaBrowserPages::setPage(const int page, bool gui)
{
    if (gui)
        setPageInGui(page);
    m_page = page;
}
void MediaBrowserPages::setPages(const QStringList &pages)
{
    m_list->blockSignals(true);
    m_list->clear();
    if (!pages.isEmpty())
    {
        m_list->addItems(pages);
        m_list->setCurrentIndex(m_page - 1);
    }
    m_list->blockSignals(false);

    m_prevPage->setVisible(pages.isEmpty());
    m_currentPage->setVisible(pages.isEmpty());
    m_nextPage->setVisible(pages.isEmpty());
    m_list->setVisible(!pages.isEmpty());
}

inline int MediaBrowserPages::getCurrentPage() const
{
    return m_page;
}
QString MediaBrowserPages::getCurrentPageName() const
{
    return m_list->currentText();
}

void MediaBrowserPages::maybeSwitchPage()
{
    const int page = getPageFromUi();
    maybeSetCurrentPage(page);
    if (page != m_page)
    {
        m_page = page;
        emit pageSwitched();
    }
}
void MediaBrowserPages::prevPage()
{
    setPageInGui(getPageFromUi() - 1);
    maybeSwitchPage();
}
void MediaBrowserPages::nextPage()
{
    setPageInGui(getPageFromUi() + 1);
    maybeSwitchPage();
}

void MediaBrowserPages::setPageInGui(const int page)
{
    if (m_list->count() == 0)
        maybeSetCurrentPage(page);
    else
    {
        m_list->blockSignals(true);
        m_list->setCurrentIndex(page - 1);
        m_list->blockSignals(false);
    }
}

void MediaBrowserPages::maybeSetCurrentPage(const int page)
{
    if (m_list->count() == 0)
    {
        const QIntValidator *validator = (const QIntValidator *)m_currentPage->validator();
        m_currentPage->setText(QString::number(qBound(validator->bottom(), page, validator->top())));
    }
}

int MediaBrowserPages::getPageFromUi() const
{
    if (m_list->count() == 0)
        return m_currentPage->text().toInt();
    return m_list->currentIndex() + 1;
}

/**/

MediaBrowser::MediaBrowser(Module &module) :
    m_resultsW(new MediaBrowserResults(m_mediaBrowser)),
    m_completerModel(new QStringListModel(this)),
    m_completer(new QCompleter(m_completerModel, this)),
    m_net(this)
{
    m_dW = new DockWidget;
    connect(m_dW, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));
    m_dW->setWindowTitle(MediaBrowserName);
    m_dW->setObjectName(MediaBrowserName);
    m_dW->setWidget(this);

    m_completer->setCaseSensitivity(Qt::CaseInsensitive);

    m_searchE = new LineEdit;
    connect(m_searchE, SIGNAL(textEdited(const QString &)), this, SLOT(searchTextEdited(const QString &)));
    connect(m_searchE, SIGNAL(clearButtonClicked()), this, SLOT(search()));
    connect(m_searchE, SIGNAL(returnPressed()), this, SLOT(search()));
    m_searchE->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_searchE->setCompleter(m_completer);

    m_searchCB = new QComboBox;
    connect(m_searchCB, SIGNAL(editTextChanged(const QString &)), this, SLOT(searchTextEdited(const QString &)));
    connect(m_searchCB, SIGNAL(activated(int)), this, SLOT(search()));
    m_searchCB->setSizePolicy(m_searchE->sizePolicy());
    m_searchCB->setInsertPolicy(QComboBox::NoInsert);
    m_searchCB->setEditable(true);

    m_providersB = new QComboBox;
    m_providersB->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(m_providersB, SIGNAL(currentIndexChanged(int)), this, SLOT(providerChanged(int)));

    m_searchB = new QToolButton;
    connect(m_searchB, SIGNAL(clicked()), this, SLOT(search()));
    m_searchB->setIcon(QMPlay2Core.getIconFromTheme("edit-find"));
    m_searchB->setFocusPolicy(Qt::StrongFocus);
    m_searchB->setToolTip(tr("Search"));
    m_searchB->setAutoRaise(true);

    m_pages = new MediaBrowserPages;
    connect(m_pages, SIGNAL(pageSwitched()), this, SLOT(search()));
    m_pages->hide();

    m_loadAllB = new QToolButton;
    m_loadAllB->setIcon(QMPlay2Core.getIconFromTheme("media-playback-start"));
    m_loadAllB->setFocusPolicy(Qt::StrongFocus);
    m_loadAllB->setToolTip(tr("Play all"));
    m_loadAllB->setAutoRaise(true);
    m_loadAllB->hide();

    m_progressB = new QProgressBar;
    m_progressB->setRange(0, 0);
    m_progressB->hide();

    m_descr = new QTextEdit;
    m_descr->setSizePolicy({QSizePolicy::Preferred, QSizePolicy::Fixed});
    m_descr->setReadOnly(true);
    m_descr->hide();

    connect(m_loadAllB, SIGNAL(clicked()), m_resultsW, SLOT(playAll()));

    connect(&m_net, SIGNAL(finished(NetworkReply *)), this, SLOT(netFinished(NetworkReply *)));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(m_providersB, 0, 0, 1, 1);
    layout->addWidget(m_searchE, 0, 1, 1, 1);
    layout->addWidget(m_searchCB, 0, 1, 1, 1);
    layout->addWidget(m_pages, 0, 2, 1, 1);
    layout->addWidget(m_searchB, 0, 3, 1, 1);
    layout->addWidget(m_loadAllB, 0, 4, 1, 1);
    layout->addWidget(m_pages->getPagesList(), 1, 0, 1, 5);
    layout->addWidget(m_resultsW, 2, 0, 1, 5);
    layout->addWidget(m_descr, 3, 0, 1, 5);
    layout->addWidget(m_progressB, 4, 0, 1, 5);
    setLayout(layout);

    SetModule(module);
}
MediaBrowser::~MediaBrowser()
{}

DockWidget *MediaBrowser::getDockWidget()
{
    return m_dW;
}

bool MediaBrowser::canConvertAddress() const
{
    return true;
}

QList<QMPlay2Extensions::AddressPrefix> MediaBrowser::addressPrefixList(bool img) const
{
    QList<AddressPrefix> ret;
    const_cast<MediaBrowser *>(this)->initScripts();
    for (const auto &m : m_mediaBrowsers)
        ret.append(m->addressPrefix(img));
    return ret;
}
void MediaBrowser::convertAddress(const QString &prefix, const QString &url, const QString &param, QString *streamUrl, QString *name, QIcon *icon, QString *extension, IOController<> *ioCtrl)
{
    if (streamUrl || icon)
    {
        initScripts();
        for (const auto &m : m_mediaBrowsers)
        {
            if (m->convertAddress(prefix, url, param, streamUrl, name, icon, extension, ioCtrl))
                break;
        }
    }
}

QVector<QAction *> MediaBrowser::getActions(const QString &name, double, const QString &url, const QString &, const QString &)
{
    QVector<QAction *> actions;
    if (name != url)
    {
        initScripts();
        for (quint32 i = 0; i < m_mediaBrowsers.size(); ++i)
        {
            const auto m = m_mediaBrowsers[i];
            if (!m->hasAction())
                continue;

            auto act = new QAction(tr("Search on %1").arg(m->name()), nullptr);
            act->setIcon(m->icon());
            act->setProperty("ptr", (quintptr)m);
            act->setProperty("idx", i);
            act->setProperty("name", name);
            connect(act, &QAction::triggered,
                    this, &MediaBrowser::searchMenu);
            actions.append(act);
        }
    }
    return actions;
}

void MediaBrowser::initScripts()
{
    if (m_loadScripts)
    {
        m_loadScripts = false;
        if (scanScripts())
            m_canUpdateScripts = true;
    }
}

inline void MediaBrowser::setCompleterListCallback()
{
    if (m_mediaBrowser)
        m_mediaBrowser->setCompleterListCallback(std::bind(&MediaBrowser::completionsReady, this));
}
void MediaBrowser::completionsReady()
{
    if (m_mediaBrowser)
    {
        const QString text = m_searchCB->currentText();
        m_searchCB->blockSignals(true);
        m_searchCB->clear();
        m_searchCB->addItems(m_mediaBrowser->getCompletions());
        m_searchCB->setCurrentIndex(-1);
        m_searchCB->setEditText(text);
        m_searchCB->blockSignals(false);
    }
}

bool MediaBrowser::scanScripts()
{
    const auto lastName = m_providersB->currentText();

    m_providersB->clear();
    m_mediaBrowser = nullptr;
    for (const auto &m : m_mediaBrowsers)
        delete m;
    m_mediaBrowsers.clear();

    QDir scriptsDir(getScriptsPath());
    if (!scriptsDir.mkpath("."))
    {
        qCCritical(mb) << "Unable to create directory" << scriptsDir.path();
        return false;
    }

    const auto files = scriptsDir.entryInfoList({"*.js"}, QDir::Files, QDir::Name);
    if (files.isEmpty())
        return true;

    const auto jsCommonCode = []()->QString {
        QFile jsCommon(":/MediaBrowserJSCommon.js");
        if (jsCommon.open(QFile::ReadOnly))
            return jsCommon.readAll();
        return QString();
    }();
    if (jsCommonCode.isEmpty())
        return false;

    const int lineNumber = 1 - jsCommonCode.split('\n').indexOf("%1");
    QSet<QString> names;

    for (auto &&file : files)
    {
        const auto js = new MediaBrowserJS(jsCommonCode, lineNumber, file.filePath(), m_net, m_resultsW, this);
        if (js->version() > 0)
        {
            const auto name = js->name();
            if (!names.contains(name))
            {
                m_mediaBrowsers.push_back(js);
                names.insert(name);
                continue;
            }
        }
        delete js;
    }

    m_providersB->blockSignals(true);
    for (const auto &m : m_mediaBrowsers)
        m_providersB->addItem(m->icon(), m->name());
    m_providersB->setCurrentIndex(-1);
    m_providersB->blockSignals(false);

    if (!lastName.isEmpty())
    {
        const int idx = m_providersB->findText(lastName, Qt::MatchExactly);
        if (idx > -1)
            m_providersB->setCurrentIndex(idx);
    }
    if (m_providersB->currentIndex() < 0)
    {
        const int idx = m_providersB->findText(sets().getString("MediaBrowser/Provider"), Qt::MatchExactly);
        m_providersB->setCurrentIndex(idx < 0 ? 0 : idx);
    }

    return true;
}
void MediaBrowser::downloadScripts(const QByteArray &jsonData)
{
    const auto jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isArray())
        return;

    const auto jsonArr = jsonDoc.array();
    if (jsonArr.isEmpty())
        return;

    const auto currentScripts = [this] {
        QHash<QString, MediaBrowserJS *> currentScripts;
        for (const auto &m : m_mediaBrowsers)
            currentScripts[m->name()] = m;
        return currentScripts;
    }();

    bool removed = false, downloading = false;
    for (auto &&jsonVal : jsonArr)
    {
        const auto name = jsonVal["Name"].toString();
        const auto path = jsonVal["Path"].toString();
        const auto version = jsonVal["Version"].toInt();
        const auto apiVersion = jsonVal["ApiVersion"].toInt();
        if (name.isEmpty() || version <= 0 || apiVersion != g_mediaBrowserApiVersion)
            continue;

        const auto m = currentScripts[name];
        const auto scriptVersion = m ? m->version() : 0;
        auto scriptPath = m ? currentScripts[name]->scriptPath() : QString();

        if (path.endsWith(".js") && scriptVersion < version)
        {
            if (scriptPath.isEmpty())
                scriptPath = getScriptsPath() + "/" + Functions::fileName(path);
            auto reply = m_net.start(g_mediaBrowserBaseUrl + path);
            reply->setProperty("scriptPath", scriptPath);
            m_scriptReplies.insert(reply);
            downloading = true;
            qCInfo(mb) << "Downloading script" << Functions::fileName(scriptPath);
        }
        else if (path.isEmpty() && !scriptPath.isEmpty() && scriptVersion <= version)
        {
            const auto scriptName = Functions::fileName(scriptPath);
            if (QFile::remove(scriptPath))
            {
                qCInfo(mb) << "Removed script" << scriptName;
                removed = true;
            }
            else
            {
                qCInfo(mb) << "Unable to remove script" << scriptName;
            }
        }
    }
    if (removed && !downloading)
        scanScripts();
}
void MediaBrowser::saveScript(const QByteArray &data, const QString &scriptPath)
{
    QFile f(scriptPath);
    if (f.open(QFile::WriteOnly))
    {
        if (f.write(data) == data.size())
        {
            return;
        }
    }
    qCCritical(mb) << "Unable to write file" << scriptPath;
}

void MediaBrowser::visibilityChanged(bool v)
{
    setEnabled(v);
    if (v)
    {
        initScripts();
        if (m_canUpdateScripts && m_updateScripts)
        {
            m_updateScripts = false;
            m_jsonReply = m_net.start(g_mediaBrowserBaseUrl + QString("MediaBrowser.json"));
        }
    }
}

void MediaBrowser::providerChanged(int idx)
{
    if (idx < 0)
        return;

    if (m_mediaBrowser)
    {
        m_mediaBrowser->setCompleterListCallback(nullptr);
        m_mediaBrowser->finalize();
    }

    m_searchCB->blockSignals(true);
    m_searchCB->clear();
    m_searchCB->blockSignals(false);
    m_searchE->blockSignals(true);
    m_searchE->clearText();
    m_searchE->blockSignals(false);

    // Clear list and cancel all network actions.
    m_mediaBrowser = nullptr;
    search();

    m_mediaBrowser = m_mediaBrowsers[idx];
    switch (m_mediaBrowser->completerMode())
    {
        case MediaBrowserJS::CompleterMode::None:
        case MediaBrowserJS::CompleterMode::Continuous:
            m_searchE->setVisible(true);
            m_searchCB->setVisible(false);
            break;
        case MediaBrowserJS::CompleterMode::All:
            m_searchE->setVisible(false);
            m_searchCB->setVisible(true);
            setCompleterListCallback();
            break;
    }
    m_mediaBrowser->prepareWidget();

    sets().set("MediaBrowser/Provider", m_providersB->currentText());
}

void MediaBrowser::searchTextEdited(const QString &text)
{
    if (sender() == m_searchE)
    {
#ifndef Q_OS_ANDROID
        if (m_autocompleteReply)
            m_autocompleteReply->deleteLater();
        if (text.isEmpty())
            m_completerModel->setStringList({});
        else if (m_mediaBrowser && m_mediaBrowser->completerMode() == MediaBrowserJS::CompleterMode::Continuous)
            m_autocompleteReply = m_mediaBrowser->getCompleterReply(text);
#endif
    }
    else if (sender() == m_searchCB && m_searchCB->count() == 0)
    {
        setCompleterListCallback();
    }
}

void MediaBrowser::search()
{
    QWidget *searchW = nullptr;
    QString name;

    if (m_mediaBrowser)
    {
        switch (m_mediaBrowser->completerMode())
        {
            case MediaBrowserJS::CompleterMode::None:
            case MediaBrowserJS::CompleterMode::Continuous:
                searchW = m_searchE;
                name = m_searchE->text();
                break;
            case MediaBrowserJS::CompleterMode::All:
                searchW = m_searchCB;
                name = m_searchCB->currentText();
                break;
            default:
                break;
        }
        name = name.simplified();
    }

    if (m_autocompleteReply)
        m_autocompleteReply->deleteLater();
    if (m_searchReply)
        m_searchReply->deleteLater();
    if (m_imageReply)
        m_imageReply->deleteLater();
    if (m_mediaBrowser)
        m_mediaBrowser->finalize();
    m_resultsW->clear();
    if (!name.isEmpty())
    {
        if (m_lastName != name || sender() == searchW || sender() == m_searchB)
            m_pages->setPage(1, m_mediaBrowser && m_mediaBrowser->pagesMode() == MediaBrowserJS::PagesMode::Multi);
        if (m_mediaBrowser)
            m_searchReply = m_mediaBrowser->getSearchReply(name, m_pages->getCurrentPage());
        if (m_searchReply)
        {
            m_descr->clear();
            m_descr->hide();
            m_progressB->show();
        }
        else
        {
            loadSearchResults();
        }
    }
    else
    {
        m_descr->clear();
        m_descr->hide();
        m_completerModel->setStringList({});
        m_pages->hide();
        m_pages->setPages({});
        m_loadAllB->hide();
        m_progressB->hide();
        m_resultsW->setCurrentName(QString(), QString());
    }
    m_lastName = name;
}

void MediaBrowser::netFinished(NetworkReply *reply)
{
    const auto finalizeScriptsReply = [=] {
        m_scriptReplies.remove(reply);
        if (m_scriptReplies.isEmpty())
            scanScripts();
    };

    if (reply->hasError())
    {
        if (reply == m_searchReply)
        {
            m_lastName.clear();
            m_pages->hide();
            m_loadAllB->hide();
            m_progressB->hide();
            if (reply->error() == NetworkReply::Error::Connection404)
                emit QMPlay2Core.sendMessage(tr("Website doesn't exist"), MediaBrowserName, 3);
            else
                emit QMPlay2Core.sendMessage(tr("Connection error"), MediaBrowserName, 3);
        }
        else if (reply == m_jsonReply)
        {
            m_updateScripts = true;
        }
        else if (m_scriptReplies.contains(reply))
        {
            const auto fileName = Functions::fileName(reply->url());
            qCWarning(mb) << "Unable to download script" << fileName;
            finalizeScriptsReply();
        }
    }
    else
    {
        const QByteArray replyData = reply->readAll();
        if (reply == m_autocompleteReply)
        {
            const QStringList completions = m_mediaBrowser ? m_mediaBrowser->getCompletions(replyData) : QStringList();
            if (!completions.isEmpty())
            {
                m_completerModel->setStringList(completions);
                if (m_searchE->hasFocus())
                    m_completer->complete();
            }
        }
        else if (reply == m_searchReply)
        {
            if (m_mediaBrowser)
                loadSearchResults(replyData);
        }
        else if (reply == m_imageReply)
        {
            const QImage img = QImage::fromData(replyData);
            if (!img.isNull())
            {
                QTextDocument *doc = m_descr->document();

                const int h = qMin<int>(img.height(), m_descr->height() - doc->documentMargin() * 5 / 2);
                doc->addResource(QTextDocument::ImageResource, QUrl("image"), Functions::getPixmapFromIcon(QPixmap::fromImage(img), QSize(0, h), this));

                QTextImageFormat txtImg;
                txtImg.setName("image");

                QTextCursor cursor = m_descr->textCursor();
                cursor.setPosition(0);
                cursor.insertImage(txtImg, QTextFrameFormat::FloatLeft);
                cursor.insertBlock();
            }
        }
        else if (reply == m_jsonReply)
        {
            downloadScripts(replyData);
        }
        else if (m_scriptReplies.contains(reply))
        {
            const auto scriptPath = reply->property("scriptPath").toString();
            Q_ASSERT(!scriptPath.isEmpty());
            saveScript(replyData, scriptPath);
            finalizeScriptsReply();
        }
    }

    if (reply == m_searchReply)
        m_progressB->hide();

    reply->deleteLater();
}

void MediaBrowser::searchMenu()
{
    const QString name = sender()->property("name").toString();
    if (!name.isEmpty())
    {
        m_providersB->setCurrentIndex(sender()->property("idx").toUInt());
        if (!m_dW->isVisible())
            m_dW->show();
        m_dW->raise();
        m_searchE->setText(name);
        search();
    }
}

void MediaBrowser::loadSearchResults(const QByteArray &replyData)
{
    const MediaBrowserJS::Description descr = m_mediaBrowser->addSearchResults(replyData);
    if (!descr.description.isEmpty())
    {
        m_descr->setHtml(descr.description);
        m_descr->setAlignment(Qt::AlignJustify);
        m_descr->show();
    }
    if (descr.imageReply)
    {
        m_imageReply = descr.imageReply;
        m_descr->show();
    }
    if (descr.nextReply)
        m_searchReply = descr.nextReply;
    else
    {
        if (m_mediaBrowser->pagesMode() == MediaBrowserJS::PagesMode::List)
        {
            const QStringList pages = m_mediaBrowser->getPagesList();
            m_pages->setPages(pages);
            m_pages->setVisible(!pages.isEmpty());
        }
        else
        {
            m_pages->setVisible(m_mediaBrowser->pagesMode() != MediaBrowserJS::PagesMode::Single && m_resultsW->topLevelItemCount());
        }
        m_loadAllB->setVisible(m_mediaBrowser->pagesMode() != MediaBrowserJS::PagesMode::Multi && m_resultsW->topLevelItemCount());
        m_resultsW->setCurrentName(m_lastName, m_pages->getCurrentPageName());
    }
}
