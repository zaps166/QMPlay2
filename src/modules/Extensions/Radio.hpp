// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Extensions.hpp>

class LineEdit;

class RadioBrowserModel;
class NetworkAccess;
class NetworkReply;

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

    void radioBrowserPlay();
    void radioBrowserAdd();
    void radioBrowserEnqueue();
    void radioBrowserOpenHomePage();

private:
    void setupUi();

    void addMyRadioStationButtonClicked();
    void editMyRadioStationButtonClicked();
    void removeMyRadioStationButtonClicked();
    void loadMyRadioStationButtonClicked();
    void saveMyRadioStationButtonClicked();

    void myRadioListWidgetItemDoubleClicked(QListWidgetItem *item);

    void searchByComboBoxActivated(int idx);
    void addRadioBrowserStationButtonClicked();
    void radioViewDoubleClicked(const QModelIndex &index);
    void radioViewCustomContextMenuRequested(const QPoint &pos);

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
    QSplitter *m_radioBrowserSplitter;
    QLabel *m_label;
    QListWidget *m_myRadioListWidget;
    QToolButton *m_loadMyRadioStationButton;
    QToolButton *m_saveMyRadioStationButton;
    QToolButton *m_addMyRadioStationButton;
    QToolButton *m_editMyRadioStationButton;
    QToolButton *m_removeMyRadioStationButton;
    QComboBox *m_searchByComboBox;
    QComboBox *m_searchComboBox;
    QToolButton *m_addRadioBrowserStationButton;
    QTreeView *m_radioView;
    LineEdit *m_filterEdit;

    const QString m_newStationTxt;
    const QIcon m_radioIcon;

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
