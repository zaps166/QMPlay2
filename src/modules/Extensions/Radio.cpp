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

#include <Radio.hpp>
#include <ui_Radio.h>

#include <Radio/RadioBrowserModel.hpp>
#include <NetworkAccess.hpp>
#include <Functions.hpp>
#include <Playlist.hpp>

#include <QDesktopServices>
#include <QJsonDocument>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonObject>
#include <QScrollBar>
#include <QJsonArray>
#include <qevent.h>
#include <QBuffer>
#include <QTimer>
#include <QMenu>
#include <QUrl>

enum SearchByIndexes
{
    Name,
    Tag,
    Country,
    Language,
    State,
};

Radio::Radio(Module &module) :
    m_newStationTxt(tr("Adding a new radio station")),
    m_radioIcon(":/radio.svgz"),
    ui(new Ui::Radio),
    m_dw(new DockWidget),
    m_radioBrowserModel(new RadioBrowserModel(this)),
    m_radioBrowserMenu(new QMenu(this)),
    m_loadIconsTimer(new QTimer(this)),
    m_net(new NetworkAccess(this))
{
    SetModule(module);

    m_dw->setWindowTitle(tr("Internet radios"));
    m_dw->setObjectName(RadioName);
    m_dw->setWidget(this);

    m_loadIconsTimer->setInterval(10);

    m_net->setRetries(g_nRetries, g_retryInterval);

    ui->setupUi(this);

    ui->addMyRadioStationButton->setIcon(QMPlay2Core.getIconFromTheme("list-add"));
    ui->editMyRadioStationButton->setIcon(QMPlay2Core.getIconFromTheme("document-properties"));
    ui->removeMyRadioStationButton->setIcon(QMPlay2Core.getIconFromTheme("list-remove"));
    ui->loadMyRadioStationButton->setIcon(QMPlay2Core.getIconFromTheme("folder-open"));
    ui->saveMyRadioStationButton->setIcon(QMPlay2Core.getIconFromTheme("document-save"));

    ui->addRadioBrowserStationButton->setIcon(ui->addMyRadioStationButton->icon());

    ui->myRadioListWidget->installEventFilter(this);

    ui->searchByComboBox->addItem(tr("Name"), QStringList{"Name", ""});
    ui->searchByComboBox->addItem(tr("Tag"), QStringList{"Tag", "tags"});
    ui->searchByComboBox->addItem(tr("Country"), QStringList{"Country", "countries"});
    ui->searchByComboBox->addItem(tr("Language"), QStringList{"Language", "languages"});
    ui->searchByComboBox->addItem(tr("State"), QStringList{"State", "states"});

    ui->radioView->sortByColumn(4, Qt::AscendingOrder); // Click count
    ui->radioView->setModel(m_radioBrowserModel);
    ui->radioView->setIconSize({m_radioBrowserModel->elementHeight(), m_radioBrowserModel->elementHeight()});

    connect(m_radioBrowserMenu->addAction(tr("Play")), SIGNAL(triggered(bool)), this, SLOT(radioBrowserPlay()));
    connect(m_radioBrowserMenu->addAction(tr("Enqueue")), SIGNAL(triggered(bool)), this, SLOT(radioBrowserEnqueue()));
    m_radioBrowserMenu->addSeparator();
    connect(m_radioBrowserMenu->addAction(tr("Add to my radio stations")), SIGNAL(triggered(bool)), this, SLOT(radioBrowserAdd()));
    m_radioBrowserMenu->addSeparator();
    connect(m_radioBrowserMenu->addAction(tr("Open radio website")), SIGNAL(triggered(bool)), this, SLOT(radioBrowserOpenHomePage()));

    connect(m_dw, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));
    connect(m_radioBrowserModel, SIGNAL(radiosAdded()), m_loadIconsTimer, SLOT(start()));
    connect(m_radioBrowserModel, SIGNAL(searchFinished()), this, SLOT(searchFinished()));
    connect(m_radioBrowserModel, &RadioBrowserModel::connectionError, this, &Radio::connectionError);
    connect(ui->radioView->verticalScrollBar(), SIGNAL(valueChanged(int)), m_loadIconsTimer, SLOT(start()));
    connect(m_loadIconsTimer, SIGNAL(timeout()), this, SLOT(loadIcons()));
    connect(ui->filterEdit, SIGNAL(textEdited(QString)), m_radioBrowserModel, SLOT(setFiltrText(QString)));
    connect(ui->filterEdit, SIGNAL(clearButtonClicked()), m_radioBrowserModel, SLOT(setFiltrText()));
    connect(ui->searchComboBox->lineEdit(), SIGNAL(returnPressed()), this, SLOT(searchData()));
    connect(ui->searchComboBox, SIGNAL(activated(int)), this, SLOT(searchData()));
    connect(m_net, SIGNAL(finished(NetworkReply *)), this, SLOT(replyFinished(NetworkReply *)));
}
Radio::~Radio()
{
    if (m_loaded)
    {
        if (m_storeMyRadios)
        {
            Settings sets("Radio");
            const auto radios = getMyRadios();
            if (radios.isEmpty())
                sets.remove("Radia");
            else
                sets.set("Radia", radios);
        }

        sets().set("Radio/RadioBrowserSplitter", ui->radioBrowserSplitter->saveState().toBase64());

        {
            QByteArray columnSizes;
            QDataStream stream(&columnSizes, QIODevice::WriteOnly);
            const int columnCount = m_radioBrowserModel->columnCount(QModelIndex());
            for (int i = 0; i < columnCount; ++i)
                stream << ui->radioView->columnWidth(i);
            sets().set("Radio/ColumnSizes", columnSizes.toBase64());
        }

        sets().set("Radio/SearchByIndex", ui->searchByComboBox->currentIndex());
    }
    delete ui;
}

DockWidget *Radio::getDockWidget()
{
    return m_dw;
}

QMenu *Radio::getTrayMenu()
{
    bool hasEntries = false;

    if (m_loaded)
        hasEntries = (ui->myRadioListWidget->count() > 0);
    else
        hasEntries = Settings("Radio").contains("Radia");

    if (!hasEntries)
    {
        delete m_menu;
        m_menu = nullptr;
    }
    else if (!m_menu)
    {
        m_menu = new QMenu(m_dw->windowTitle(), this);
    }

    return m_menu;
}
void Radio::ensureTrayMenu()
{
    if (!m_recreateTrayMenu || !m_menu)
        return;

    if (!m_loaded)
    {
        restoreSettings();
    }

    m_menu->clear();
    for (auto item : ui->myRadioListWidget->findItems(QString(), Qt::MatchContains))
    {
        auto act = m_menu->addAction(item->text());
        act->setData(item->data(Qt::UserRole));
        connect(act, &QAction::triggered,
                this, &Radio::trayActionTriggered);
    }

    m_recreateTrayMenu = false;
}

void Radio::visibilityChanged(const bool v)
{
    if (v && !m_loaded)
    {
        restoreSettings();
    }
}

void Radio::searchData()
{
    const QString text = ui->searchComboBox->lineEdit()->text();
    m_radioBrowserModel->searchRadios(text, ui->searchByComboBox->itemData(ui->searchByComboBox->currentIndex()).toStringList().at(0));
    ui->radioView->setEnabled(false);
    ui->filterEdit->clear();
}
void Radio::searchFinished()
{
    QHeaderView *header = ui->radioView->header();
    int s = 0;
    for (int i = 0; i < 5; ++i)
        s += header->sectionSize(i);
    if (s < header->width()) // It's inaccurate comparison
    {
        header->setSectionResizeMode(0, QHeaderView::Stretch);
        if (!m_sectionResizedConn)
        {
            m_sectionResizedConn = connect(header, &QHeaderView::sectionResized,
                                           header, [this, header](int logicalIndex, int oldSize, int newSize) {
                Q_UNUSED(oldSize)
                Q_UNUSED(newSize)

                if (logicalIndex != 0)
                    return;

                disconnect(m_sectionResizedConn);
                header->setSectionResizeMode(0, QHeaderView::Interactive);
            }, Qt::QueuedConnection);
        }
    }
    ui->radioView->setEnabled(true);
}

void Radio::loadIcons()
{
    const QRect viewportRect = ui->radioView->viewport()->rect();
    const QModelIndex first = ui->radioView->indexAt(viewportRect.topLeft());
    if (first.isValid())
    {
        QModelIndex last = first;
        for (;;)
        {
            const QModelIndex index = ui->radioView->indexBelow(last);
            if (!index.isValid())
                break;
            const QRect indexRect = ui->radioView->visualRect(index);
            if (!viewportRect.contains(indexRect.topLeft()))
                break;
            last = index;
        }
        m_radioBrowserModel->loadIcons(first.row(), last.row());
    }
}

void Radio::replyFinished(NetworkReply *reply)
{
    const int idx = m_searchInfo.key({{}, reply}, -1);
    if (idx > -1)
    {
        if (reply->hasError())
        {
            if (!m_errorDisplayed)
            {
                connectionError();
                m_errorDisplayed = true;
            }
        }
        else
        {
            const QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
            if (json.isArray())
            {
                QStringList list;
                for (const QJsonValue &data : json.array())
                {
                    if (!data.isObject())
                        continue;

                    const auto name = data["name"].toString();
                    if (name.trimmed().isEmpty())
                        continue;

                    list += name;
                }
                list.removeDuplicates();
                m_searchInfo[idx].first = list;
                if (ui->searchByComboBox->currentIndex() == idx)
                    setSearchInfo(list);
            }
        }
    }
    reply->deleteLater();
}

void Radio::on_addMyRadioStationButton_clicked()
{
    QString name;
    QString address("http://");
    for (;;)
    {
        bool ok = false;
        name = QInputDialog::getText(this, m_newStationTxt, tr("Name"), QLineEdit::Normal, name, &ok);
        if (ok && !name.isEmpty())
        {
            address = QInputDialog::getText(this, m_newStationTxt, tr("Address"), QLineEdit::Normal, address, &ok).simplified();
            if (ok && !address.isEmpty())
            {
                if (!addMyRadioStation(name, address, QPixmap()))
                    continue;
            }
        }
        break;
    }
}
void Radio::on_editMyRadioStationButton_clicked()
{
    if (QListWidgetItem *item = ui->myRadioListWidget->currentItem())
    {
        const QString newStationTxt = tr("Editing selected radio station");
        QString name = item->text();
        QString address = item->data(Qt::UserRole).toString();
        for (;;)
        {
            bool ok = false;
            name = QInputDialog::getText(this, newStationTxt, tr("Name"), QLineEdit::Normal, name, &ok);
            if (ok && !name.isEmpty())
            {
                address = QInputDialog::getText(this, newStationTxt, tr("Address"), QLineEdit::Normal, address, &ok).simplified();
                if (ok && !address.isEmpty())
                {
                    if (!addMyRadioStation(name, address, QPixmap(), item))
                        continue;
                }
            }
            break;
        }
    }
}
void Radio::on_removeMyRadioStationButton_clicked()
{
    delete ui->myRadioListWidget->currentItem();
    m_storeMyRadios = true;
    m_recreateTrayMenu = true;
}
void Radio::on_loadMyRadioStationButton_clicked()
{
    const QString filePath = QFileDialog::getOpenFileName(this, tr("Load radio station list"), QString(), getFileFilters(false));
    if (!filePath.isEmpty())
    {
        loadMyRadios(QSettings(filePath, QSettings::IniFormat).value("Radia").toStringList());
        m_storeMyRadios = true;
        m_recreateTrayMenu = true;
    }
}
void Radio::on_saveMyRadioStationButton_clicked()
{
    if (ui->myRadioListWidget->count() == 0)
        return;

    QString filter;
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save radio station list"), QString(), getFileFilters(true), &filter);
    if (filePath.isEmpty())
        return;

    int idx = filter.indexOf("(*.");
    if (idx < 0)
        return;

    const auto suffix = QStringView(filter).mid(idx + 2).chopped(1)MAYBE_TO_STRING;
    if (!filePath.endsWith(suffix, Qt::CaseInsensitive))
        filePath += suffix;

    if (suffix == QStringLiteral(".qmplay2radio"))
    {
        QSettings(filePath, QSettings::IniFormat).setValue("Radia", getMyRadios());
    }
    else
    {
        Playlist::Entries plist;
        for (auto item : ui->myRadioListWidget->findItems(QString(), Qt::MatchContains))
        {
            plist.push_back({item->text(), item->data(Qt::UserRole).toString()});
        }
        Playlist::write(plist, Functions::Url(filePath));
    }
}

void Radio::on_myRadioListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    if (item)
    {
        play(item->data(Qt::UserRole).toString(), item->text());
    }
}

void Radio::on_searchByComboBox_activated(int idx)
{
    const QString toDownload = ui->searchByComboBox->itemData(idx).toStringList().at(1);

    QString placeholderText;
    if (idx == Name)
        placeholderText = tr("Type the station name and press Enter");
    else
        placeholderText = tr("Select a \"%1\" from the drop-down list").arg(ui->searchByComboBox->itemText(idx));
    ui->searchComboBox->lineEdit()->setPlaceholderText(placeholderText);

    if (idx != Name)
    {
        Q_ASSERT(!toDownload.isEmpty());
        if (m_searchByComboBoxIdx == Name && m_nameItems.isEmpty())
        {
            for (int i = 0; i < ui->searchComboBox->count(); ++i)
                m_nameItems += ui->searchComboBox->itemText(i);
            ui->searchComboBox->clear();
        }
        ui->searchComboBox->setInsertPolicy(QComboBox::NoInsert);

        auto &value = m_searchInfo[idx];
        if (value.first.isEmpty())
        {
            if (!value.second)
                value.second = m_net->start(QString("%1/%2").arg(g_radioBrowserBaseApiUrl, toDownload));
        }
        else
        {
            setSearchInfo(value.first);
        }
    }
    else
    {
        ui->searchComboBox->clear();
        if (!m_nameItems.isEmpty())
        {
            ui->searchComboBox->addItems(m_nameItems);
            ui->searchComboBox->lineEdit()->clear();
            m_nameItems.clear();
        }
        ui->searchComboBox->setInsertPolicy(QComboBox::InsertAtBottom);
    }

    m_radioBrowserModel->clear();

    m_searchByComboBoxIdx = idx;
}
void Radio::on_addRadioBrowserStationButton_clicked()
{
    QDesktopServices::openUrl(QUrl("http://www.radio-browser.info/add"));
}
void Radio::on_radioView_doubleClicked(const QModelIndex &index)
{
    radioBrowserPlayOrEnqueue(index, "open");
}
void Radio::on_radioView_customContextMenuRequested(const QPoint &pos)
{
    if (ui->radioView->currentIndex().isValid())
        m_radioBrowserMenu->popup(ui->radioView->viewport()->mapToGlobal(pos));
}

void Radio::radioBrowserPlay()
{
    const QModelIndex index = ui->radioView->currentIndex();
    if (index.isValid())
        radioBrowserPlayOrEnqueue(index, "open");
}
void Radio::radioBrowserAdd()
{
    const QModelIndex index = ui->radioView->currentIndex();
    if (index.isValid())
    {
        const QString title = m_radioBrowserModel->getName(index);
        const QString url = m_radioBrowserModel->getUrl(index).toString();
        const QPixmap icon = m_radioBrowserModel->getIcon(index);
        addMyRadioStation(title, url, icon);
    }
}
void Radio::radioBrowserEnqueue()
{
    const QModelIndex index = ui->radioView->currentIndex();
    if (index.isValid())
        radioBrowserPlayOrEnqueue(index, "enqueue");
}
void Radio::radioBrowserOpenHomePage()
{
    const QModelIndex index = ui->radioView->currentIndex();
    if (index.isValid())
        QDesktopServices::openUrl(m_radioBrowserModel->getHomePageUrl(index));
}

QString Radio::getFileFilters(bool all) const
{
    QString filter = "QMPlay2 radio station list (*.qmplay2radio)";
    if (all)
    {
        for (const QString &e : Playlist::extensions())
            filter += ";;" + e.toUpper() + " (*." + e + ")";
    }
    return filter;
}

void Radio::radioBrowserPlayOrEnqueue(const QModelIndex &index, const QString &param)
{
    const QString title = m_radioBrowserModel->getName(index);
    const QString url = m_radioBrowserModel->getUrl(index).toString();
    const QString uuid = m_radioBrowserModel->getUUID(index);

    // Increase clickcount
    m_net->start(QString("%1/url/%2").arg(g_radioBrowserBaseApiUrl, uuid));

    QMPlay2Core.addNameForUrl(url, title);
    emit QMPlay2Core.processParam(param, url);
}

bool Radio::addMyRadioStation(QString name, const QString &address, const QPixmap &icon, QListWidgetItem *item)
{
    name = name.simplified();
    const auto items = ui->myRadioListWidget->findItems(name, Qt::MatchExactly | Qt::MatchCaseSensitive);
    if (!items.empty() && (!item || !items.contains(item)))
    {
        QMessageBox::information(this, m_newStationTxt, tr("Radio station with given name already exists!"));
        return false;
    }
    if (!item)
    {
        item = new QListWidgetItem(ui->myRadioListWidget);
        item->setIcon(icon.isNull() ? m_radioIcon : icon);
        item->setData(Qt::UserRole + 1, !icon.isNull());
        ui->myRadioListWidget->setCurrentItem(item);
    }
    item->setText(name);
    item->setData(Qt::UserRole, address);
    if (m_loaded)
    {
        m_storeMyRadios = true;
        m_recreateTrayMenu = true;
    }
    return true;
}

void Radio::setSearchInfo(const QStringList &list)
{
    ui->searchComboBox->clear();
    ui->searchComboBox->addItems(list);
    ui->searchComboBox->lineEdit()->clear();
    if (m_loadCurrentCountry)
    {
        m_loadCurrentCountry = false;
        if (ui->searchByComboBox->currentIndex() == Country)
        {
            const auto country = QLocale::countryToString(QLocale::system().country());
            auto it = std::find_if(list.crbegin(), list.crend(), [&](const QString &str) {
                return str.contains(country, Qt::CaseInsensitive);
            });
            const int idx = list.size() - std::distance(list.crbegin(), it) - 1;
            if (!country.isEmpty() && idx >= 0 && idx < list.size())
            {
                ui->searchComboBox->setCurrentIndex(idx);
                searchData();
            }
        }
    }
}

void Radio::restoreSettings()
{
    loadMyRadios(Settings("Radio").getStringList("Radia"));

    {
        QDataStream stream(QByteArray::fromBase64(sets().getByteArray("Radio/ColumnSizes")));
        int c = 0;
        while (!stream.atEnd())
        {
            int w;
            stream >> w;
            ui->radioView->setColumnWidth(c++, w);
        }
    }

    if (!ui->radioBrowserSplitter->restoreState(QByteArray::fromBase64(sets().getByteArray("Radio/RadioBrowserSplitter"))))
        ui->radioBrowserSplitter->setSizes({width() * 1 / 4, width() * 3 / 4});

    const int searchByIdx = qBound(0, sets().getInt("Radio/SearchByIndex", Country), ui->searchByComboBox->count() - 1);
    if (searchByIdx > 0)
    {
        ui->searchByComboBox->setCurrentIndex(searchByIdx);
    }
    m_loadCurrentCountry = (searchByIdx == Country);
    on_searchByComboBox_activated(searchByIdx);

    m_loaded = true;
}

QStringList Radio::getMyRadios() const
{
    QStringList myRadios;
    for (QListWidgetItem *item : ui->myRadioListWidget->findItems(QString(), Qt::MatchContains))
    {
        QString radio = item->text() + "\n" + item->data(Qt::UserRole).toString();
        if (item->data(Qt::UserRole + 1).toBool())
        {
            const auto icon = item->icon();
            const auto pixmap = icon.pixmap(icon.availableSizes().value(0));
            if (!pixmap.isNull())
            {
                QByteArray data;
                QBuffer buffer(&data);
                if (pixmap.save(&buffer, "PNG", 80))
                {
                    radio += "\n";
                    radio += data.toBase64();
                }
            }
        }
        myRadios += radio;
    }
    return myRadios;
}
void Radio::loadMyRadios(const QStringList &radios)
{
    ui->myRadioListWidget->clear();
    for (const QString &entry : radios)
    {
        const QStringList radioDescr = entry.split('\n');
        QPixmap pixmap;
        if (radioDescr.count() == 3)
            pixmap.loadFromData(QByteArray::fromBase64(radioDescr[2].toLatin1()));
        else if (radioDescr.count() != 2)
            continue;
        addMyRadioStation(radioDescr[0], radioDescr[1], pixmap);
    }
}

void Radio::trayActionTriggered(bool checked)
{
    Q_UNUSED(checked)
    auto act = qobject_cast<QAction *>(sender());
    play(act->data().toString(), act->text());
}

void Radio::play(const QString &url, const QString &name)
{
    QMPlay2Core.addNameForUrl(url, name);
    emit QMPlay2Core.processParam("open", url);
}

void Radio::connectionError()
{
    emit QMPlay2Core.sendMessage(tr("Connection error"), RadioName, 3);
}

bool Radio::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->myRadioListWidget)
    {
        switch (event->type())
        {
            case QEvent::DragEnter:
            {
                QDragEnterEvent *dee = (QDragEnterEvent *)event;
                if (dee->source() == ui->radioView)
                {
                    dee->accept();
                    return true;
                }
                break;
            }
            case QEvent::Drop:
            {
                QDropEvent *de = (QDropEvent *)event;
                if (de->source() == ui->radioView)
                {
                    radioBrowserAdd();
                    de->accept();
                    return true;
                }
                break;
            }
            default:
                break;
        }
    }
    return QWidget::eventFilter(watched, event);
}
