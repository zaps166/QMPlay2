// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <IOController.hpp>
#include <Functions.hpp>
#include <Playlist.hpp>

#include <QTreeWidget>
#include <QAtomicInt>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QTimer>
#include <QUrl>

class QTreeWidgetItem;
class PlaylistWidget;
class Demuxer;

class UpdateEntryThr final : public QThread
{
    Q_OBJECT

public:
    UpdateEntryThr(PlaylistWidget &pLW);

    void updateEntry(QTreeWidgetItem *item, const QString &name = QString(), double length = -2.0);

    void stop();

    inline bool hasPendingUpdates() const
    {
        return pendingUpdates > 0;
    }

private:
    void run() override;

    QAtomicInt pendingUpdates;
    IOController<> ioCtrl;
    PlaylistWidget &pLW;
    bool timeChanged;
    QMutex mutex;

    struct ItemToUpdate
    {
        QTreeWidgetItem *item;
        QString url;
        double oldLength;

        QString name;
        double length;
    };
    QQueue<ItemToUpdate> itemsToUpdate;

    struct ItemUpdated
    {
        QTreeWidgetItem *item;
        QString name;
        QIcon icon;

        bool updateLength, updateIcon;
        double length;
    };
private slots:
    void updateItem(ItemUpdated iu);
    void finished();
};

class AddThr final : public QThread
{
    Q_OBJECT
public:
    enum SYNC {NO_SYNC = 0, DIR_SYNC = 1, FILE_SYNC = 2};

    AddThr(PlaylistWidget &pLW);

    void setData(const QStringList &_urls, const QStringList &_existingEntries, QTreeWidgetItem *_par, bool _loadList, SYNC = NO_SYNC);
    void setDataForSync(const QString &, QTreeWidgetItem *, bool);

    void stop();
private slots:
    void changeItemText0(QTreeWidgetItem *tWI, QString name);
    void modifyItemFlags(QTreeWidgetItem *tWI, int flags);
    void deleteTreeWidgetItem(QTreeWidgetItem *tWI);
private:
    void run() override;

    bool add(const QStringList &urls, QTreeWidgetItem *parent, const Functions::DemuxersInfo &demuxersInfo, QStringList *existingEntries = nullptr, bool loadList = false);
    QTreeWidgetItem *insertPlaylistEntries(const Playlist::Entries &entries, QTreeWidgetItem *parent, const Functions::DemuxersInfo &demuxersInfo, int insertChildAt, QStringList *existingEntries);

    PlaylistWidget &pLW;
    QStringList urls, existingEntries;
    QTreeWidgetItem *par;
    bool loadList;
    SYNC sync;
    IOController<> ioCtrl;
    QTreeWidgetItem *firstItem, *lastItem;
    bool inProgress;
public:
    bool running = false;
private slots:
    void finished();
signals:
    void status(bool s);
};

class PlaylistWidget final : public QTreeWidget
{
    friend class AddThr;
    friend class UpdateEntryThr;
    Q_OBJECT
public:
    enum CHILDREN {ONLY_GROUPS = 0x10, ONLY_NON_GROUPS = 0x100, ALL_CHILDREN = ONLY_GROUPS | ONLY_NON_GROUPS};
    enum REFRESH  {REFRESH_QUEUE = 0x10, REFRESH_GROUPS_TIME = 0x100, REFRESH_CURRPLAYING = 0x1000, REFRESH_ALL = REFRESH_QUEUE | REFRESH_GROUPS_TIME | REFRESH_CURRPLAYING};

    PlaylistWidget();

    QString getUrl(QTreeWidgetItem *tWI = nullptr) const;

    void setItemsResizeToContents(bool);

    void sortCurrentGroup(int column, Qt::SortOrder sortOrder);

    bool add(const QStringList &, QTreeWidgetItem *par, const QStringList &existingEntries = {}, bool loadList = false, bool forceEnqueue = false);
    bool add(const QStringList &, bool atEndOfList = false);
    void sync(const QString &pth, QTreeWidgetItem *par, bool notDir);
    void quickSync(const QString &pth, QTreeWidgetItem *par, bool recursive, QTreeWidgetItem *&itemToNull);

    void setCurrentPlaying(QTreeWidgetItem *tWI);

    void clear();
    void clearCurrentPlaying(bool url = true);

    void stopLoading();

    QList<QTreeWidgetItem *> topLevelNonGroupsItems() const;
    QList<QUrl> getUrls() const;

    QTreeWidgetItem *newGroup();

    QList<QTreeWidgetItem *> getChildren(CHILDREN children = ALL_CHILDREN, const QTreeWidgetItem *parent = nullptr) const;

    bool canModify(bool all = true) const;

    void enqueue();
    void refresh(REFRESH Refresh = REFRESH_ALL);

    void processItems(QList<QTreeWidgetItem *> *itemsToShow = nullptr, bool hideGroups = false);

    QString currentPlayingUrl;
    QTreeWidgetItem *currentPlaying;
    QVariant currentPlayingItemIcon;

    QList<QTreeWidgetItem *> queue;

    AddThr addThr;
    UpdateEntryThr updateEntryThr;

    bool dontUpdateAfterAdd;

    static inline bool isGroup(QTreeWidgetItem *tWI)
    {
        return tWI ? (bool)(tWI->flags() & Qt::ItemIsDropEnabled) : false;
    }
    static inline int getFlags(QTreeWidgetItem *tWI)
    {
        return tWI ? tWI->data(0, Qt::UserRole + 1).toInt() : 0;
    }

    static bool isAlwaysSynced(QTreeWidgetItem *tWI, bool parentOnly = false);

    static void setEntryFont(QTreeWidgetItem *tWI, const int flags);
private:
    QTreeWidgetItem *newGroup(const QString &name, const QString &url, QTreeWidgetItem *parent, int insertChildAt, QStringList *existingEntries);
    QTreeWidgetItem *newEntry(const Playlist::Entry &entry, QTreeWidgetItem *parent, const Functions::DemuxersInfo &demuxersInfo, int insertChildAt, QStringList *existingEntries);

    void setEntryIcon(const QIcon &icon, QTreeWidgetItem *);

    void quickSyncScanDirs(const QString &pth, QTreeWidgetItem *par, bool &mustRefresh, bool recursive, QTreeWidgetItem *&itemToNull);

    void createExtensionsMenu();

    void mouseMoveEvent(QMouseEvent *) override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dragMoveEvent(QDragMoveEvent *) override;
    void dropEvent(QDropEvent *) override;
    void paintEvent(QPaintEvent *) override;
    void scrollContentsBy(int dx, int dy) override;

    QRect getArcRect(int size);

    bool internalDrag, selectAfterAdd, hasHiddenItems;
    QString currPthToSave;
    struct AddData
    {
        QStringList urls;
        QStringList existingEntries;
        QTreeWidgetItem *par;
        bool loadList;
    };
    QQueue<AddData> enqueuedAddData;
    QTimer animationTimer, addTimer;
    bool repaintAll;
    int rotation;
private slots:
    void insertItem(QTreeWidgetItem *, QTreeWidgetItem *, int insertChildAt);
    void popupContextMenu(const QPoint &);
    void animationUpdate();
    void addTimerElapsed();
public slots:
    void modifyMenu();
signals:
    void returnItem(QTreeWidgetItem *);
    void visibleItemsCount(int);
    void addStatus(bool s);
};
