/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2025  Błażej Szczygieł

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

#include <QPointer>
#include <QIcon>
#include <QMap>

namespace Ui {
    class Radio;
}

class RadioBrowserModel;
class QListWidgetItem;
class NetworkAccess;
class NetworkReply;
class QTimer;

class Radio final : public QWidget, public QMPlay2Extensions
{
    Q_OBJECT

public:
    Radio(Module &);
    ~Radio();

    DockWidget *getDockWidget() override;

    QMenu *getTrayMenu() override;
    void ensureTrayMenu() override;

private slots:
    void visibilityChanged(const bool v);

    void searchData();
    void searchFinished();

    void loadIcons();

    void replyFinished(NetworkReply *reply);

    void on_addMyRadioStationButton_clicked();
    void on_editMyRadioStationButton_clicked();
    void on_removeMyRadioStationButton_clicked();
    void on_loadMyRadioStationButton_clicked();
    void on_saveMyRadioStationButton_clicked();

    void on_myRadioListWidget_itemDoubleClicked(QListWidgetItem *item);

    void on_searchByComboBox_activated(int idx);
    void on_addRadioBrowserStationButton_clicked();
    void on_radioView_doubleClicked(const QModelIndex &index);
    void on_radioView_customContextMenuRequested(const QPoint &pos);

    void radioBrowserPlay();
    void radioBrowserAdd();
    void radioBrowserEnqueue();
    void radioBrowserOpenHomePage();

private:
    QString getFileFilters(bool all) const;

    void radioBrowserPlayOrEnqueue(const QModelIndex &index, const QString &param);

    bool addMyRadioStation(QString name, const QString &address, const QPixmap &icon, QListWidgetItem *item = nullptr);

    void setSearchInfo(const QStringList &list);

    void restoreSettings();

    QStringList getMyRadios() const;
    void loadMyRadios(const QStringList &radios);

    void trayActionTriggered(bool checked);

    void play(const QString &url, const QString &name);

    void connectionError();

private:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    const QString m_newStationTxt;
    const QIcon m_radioIcon;

    Ui::Radio *ui;

    bool m_loaded = false;
    bool m_loadCurrentCountry = false;
    bool m_storeMyRadios = false;
    bool m_recreateTrayMenu = true;
    DockWidget *m_dw;
    QMenu *m_menu = nullptr;

    QMetaObject::Connection m_sectionResizedConn;

    QMap<int, QPair<QStringList, QPointer<NetworkReply>>> m_searchInfo;
    RadioBrowserModel *m_radioBrowserModel;
    QMenu *m_radioBrowserMenu;
    QTimer *m_loadIconsTimer;
    QStringList m_nameItems;
    NetworkAccess *m_net;
    int m_searchByComboBoxIdx = -1;
};

#define RadioName "Radio Browser"
