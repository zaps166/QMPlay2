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

#include <Radio.hpp>
#include <ui_Radio.h>

#include <Radio/RadioBrowserModel.hpp>
#include <NetworkAccess.hpp>
#include <Functions.hpp>

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

constexpr const char *g_fileDialogFilter = "QMPlay2 radio station list (*.qmplay2radio)";

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

    ui->radioView->setModel(m_radioBrowserModel);
    ui->radioView->setIconSize({m_radioBrowserModel->elementHeight(), m_radioBrowserModel->elementHeight()});

    QHeaderView *header = ui->radioView->header();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    connect(m_radioBrowserMenu->addAction(tr("Play")), SIGNAL(triggered(bool)), this, SLOT(radioBrowserPlay()));
    connect(m_radioBrowserMenu->addAction(tr("Enqueue")), SIGNAL(triggered(bool)), this, SLOT(radioBrowserEnqueue()));
    m_radioBrowserMenu->addSeparator();
    connect(m_radioBrowserMenu->addAction(tr("Add to my radio stations")), SIGNAL(triggered(bool)), this, SLOT(radioBrowserAdd()));
    m_radioBrowserMenu->addSeparator();
    connect(m_radioBrowserMenu->addAction(tr("Open radio website")), SIGNAL(triggered(bool)), this, SLOT(radioBrowserOpenHomePage()));

    connect(m_dw, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));
    connect(m_radioBrowserModel, SIGNAL(radiosAdded()), m_loadIconsTimer, SLOT(start()));
    connect(m_radioBrowserModel, SIGNAL(searchFinished()), this, SLOT(searchFinished()));
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
    if (m_once)
    {
        if (m_storeMyRadios)
            Settings("Radio").set("Radia", getMyRadios());

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

void Radio::visibilityChanged(const bool v)
{
    if (v && !m_once)
    {
        restoreSettings();
        m_once = true;
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
    if (!reply->hasError())
    {
        const int idx = m_searchInfo.key({{}, reply}, -1);
        if (idx > -1)
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
    bool ok = false;
    const QString name = QInputDialog::getText(this, m_newStationTxt, tr("Name"), QLineEdit::Normal, QString(), &ok);
    if (ok && !name.isEmpty())
    {
        const QString address = QInputDialog::getText(this, m_newStationTxt, tr("Address"), QLineEdit::Normal, "http://", &ok).simplified();
        if (ok && !address.isEmpty())
            addMyRadioStation(name, address, QPixmap());
    }
}
void Radio::on_editMyRadioStationButton_clicked()
{
    if (QListWidgetItem *item = ui->myRadioListWidget->currentItem())
    {
        const QString newStationTxt = tr("Editing selected radio station");
        bool ok = false;
        const QString name = QInputDialog::getText(this, newStationTxt, tr("Name"), QLineEdit::Normal, item->text(), &ok);
        if (ok && !name.isEmpty())
        {
            const QString address = QInputDialog::getText(this, newStationTxt, tr("Address"), QLineEdit::Normal, item->data(Qt::UserRole).toString(), &ok).simplified();
            if (ok && !address.isEmpty())
                addMyRadioStation(name, address, QPixmap(), item);
        }
    }
}
void Radio::on_removeMyRadioStationButton_clicked()
{
    delete ui->myRadioListWidget->currentItem();
    m_storeMyRadios = true;
}
void Radio::on_loadMyRadioStationButton_clicked()
{
    const QString filePath = QFileDialog::getOpenFileName(this, tr("Load radio station list"), QString(), g_fileDialogFilter);
    if (!filePath.isEmpty())
    {
        loadMyRadios(QSettings(filePath, QSettings::IniFormat).value("Radia").toStringList());
        m_storeMyRadios = true;
    }
}
void Radio::on_saveMyRadioStationButton_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save radio station list"), QString(), g_fileDialogFilter);
    if (!filePath.isEmpty())
    {
        if (!filePath.endsWith(".qmplay2radio", Qt::CaseInsensitive))
            filePath += ".qmplay2radio";
        QSettings(filePath, QSettings::IniFormat).setValue("Radia", getMyRadios());
    }
}

void Radio::on_myRadioListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    if (item)
    {
        QMPlay2Core.addNameForUrl(item->data(Qt::UserRole).toString(), item->text());
        emit QMPlay2Core.processParam("open", item->data(Qt::UserRole).toString());
    }
}

void Radio::on_searchByComboBox_activated(int idx)
{
    const QString toDownload = ui->searchByComboBox->itemData(idx).toStringList().at(1);
    if (!toDownload.isEmpty())
    {
        if (m_nameItems.isEmpty())
        {
            m_nameItems += ui->searchComboBox->lineEdit()->text();
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
            const QString text = m_nameItems.takeFirst();
            ui->searchComboBox->addItems(m_nameItems);
            ui->searchComboBox->lineEdit()->setText(text);
            m_nameItems.clear();
        }
        ui->searchComboBox->setInsertPolicy(QComboBox::InsertAtBottom);
    }
}
void Radio::on_addRadioBrowserStationButton_clicked()
{
    QDesktopServices::openUrl(QUrl("http://www.radio-browser.info/#!/add"));
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

void Radio::radioBrowserPlayOrEnqueue(const QModelIndex &index, const QString &param)
{
    const QString title = m_radioBrowserModel->getName(index);
    const QString url = m_radioBrowserModel->getUrl(index).toString();
    QMPlay2Core.addNameForUrl(url, title);
    emit QMPlay2Core.processParam(param, url);
}

void Radio::addMyRadioStation(const QString &name, const QString &address, const QPixmap &icon, QListWidgetItem *item)
{
    if (!item)
    {
        if (!ui->myRadioListWidget->findItems(name, Qt::MatchExactly | Qt::MatchCaseSensitive).isEmpty())
        {
            QMessageBox::information(this, m_newStationTxt, tr("Radio station with given name already exists!"));
            return;
        }
        item = new QListWidgetItem(ui->myRadioListWidget);
        item->setIcon(icon.isNull() ? m_radioIcon : icon);
        item->setData(Qt::UserRole + 1, !icon.isNull());
        ui->myRadioListWidget->setCurrentItem(item);
    }
    item->setText(name);
    item->setData(Qt::UserRole, address);
    if (m_once)
        m_storeMyRadios = true;
}

void Radio::setSearchInfo(const QStringList &list)
{
    ui->searchComboBox->clear();
    ui->searchComboBox->addItems(list);
    ui->searchComboBox->lineEdit()->clear();
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

    const int searchByIdx = qBound(0, sets().getInt("Radio/SearchByIndex"), ui->searchByComboBox->count() - 1);
    if (searchByIdx > 0)
    {
        ui->searchByComboBox->setCurrentIndex(searchByIdx);
        on_searchByComboBox_activated(searchByIdx);
    }
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
