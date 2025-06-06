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

#include <DockWidget.hpp>
#include <RepeatMode.hpp>

class QTreeWidgetItem;
class PlaylistWidget;
class LineEdit;
class QLabel;

class PlaylistDock : public DockWidget
{
    Q_OBJECT
public:
    PlaylistDock();

    void stopThreads();

    QString getUrl(QTreeWidgetItem *tWI = nullptr) const;
    QString getCurrentItemName() const;

    void load(const QString &);
    bool save(const QString &, bool saveCurrentGroup = false);

    void add(const QStringList &);
    void addAndPlay(const QStringList &);
    void add(const QString &);
    void addAndPlay(const QString &);

    void remove(const QString &url);
    void remove(const QStringList &urls);

    void scrollToCurrectItem();

    inline QWidget *findEdit() const
    {
        return (QWidget *)findE;
    }

    void setMode();

private:
    void showEvent(QShowEvent *e) override;

private:
    void expandTree(QTreeWidgetItem *);

    void toggleEntryFlag(const int flag);

    inline bool isRandomPlayback() const;

    void doGroupSync(bool quick, QTreeWidgetItem *tWI, bool quickRecursive = true);

    bool maybeDeleteTreeWidgetItem(QTreeWidgetItem *tWI);

    QTreeWidgetItem *initializeItemsDelete();
    void finalizeItemsDelete(QTreeWidgetItem *par, bool deleted);

    QWidget mainW;
    PlaylistWidget *list;
    QLabel *statusL;
    LineEdit *findE;

    RepeatMode repeatMode;

    bool playAfterAdd;
    QTreeWidgetItem *lastPlaying;
    QList<QTreeWidgetItem *> randomPlayedItems;
private slots:
    void itemDoubleClicked(QTreeWidgetItem *);
    void addAndPlay(QTreeWidgetItem *);
    void maybeDoQuickSync(QTreeWidgetItem *item);
public slots:
    void stopLoading();
    void next(bool playingError = false);
    void prev();
    void skip();
    void stopAfter();
    void toggleLock();
    void alwaysSyncTriggered(bool checked);
    void start();
    void clearCurrentPlaying();
    void setCurrentPlaying();
    void newGroup();
    void delEntries(bool fromDisk);
    void delNonGroupEntries(bool force = false);
    void clear();
    void copy();
public:
    void paste(bool play);
public slots:
    void renameGroup();
    void entryProperties();
    void timeSort1();
    void timeSort2();
    void titleSort1();
    void titleSort2();
    void collapseAll();
    void expandAll();
    void goToPlayback();
    void queue();
    void findItems(const QString &);
    void findNext();
    void visibleItemsCount(int);
    void syncCurrentFolder();
    void quickSyncCurrentFolder();
    void repeat();
    void updateCurrentEntry(const QString &, double);
signals:
    void play(const QString &);
    void repeatEntry(bool b);
    void stop();
    void addAndPlayRestoreWindow();
};
