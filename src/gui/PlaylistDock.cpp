/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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

#include <PlaylistDock.hpp>
#include <PlaylistWidget.hpp>

#include <EntryProperties.hpp>
#include <LineEdit.hpp>
#include <Main.hpp>

#include <QFileInfo>
#include <QLabel>
#include <QBuffer>
#include <QLineEdit>
#include <QGridLayout>
#include <QClipboard>
#include <QApplication>
#include <QMimeData>
#include <QPainter>
#include <QAction>
#include <QMessageBox>

PlaylistDock::PlaylistDock() :
	repeatMode(RepeatNormal),
	lastPlaying(nullptr)
{
	setWindowTitle(tr("Playlist"));
	setWidget(&mainW);

	list = new PlaylistWidget;
	findE = new LineEdit;
	findE->setToolTip(tr("Filter entries"));
	statusL = new QLabel;

	QGridLayout *layout = new QGridLayout(&mainW);
	layout->addWidget(list);
	layout->addWidget(findE);
	layout->addWidget(statusL);

	playAfterAdd = false;

	connect(list, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(itemDoubleClicked(QTreeWidgetItem *)));
	connect(list, SIGNAL(returnItem(QTreeWidgetItem *)), this, SLOT(addAndPlay(QTreeWidgetItem *)));
	connect(list, &PlaylistWidget::itemExpanded, this, &PlaylistDock::maybeDoQuickSync);
	connect(list, SIGNAL(visibleItemsCount(int)), this, SLOT(visibleItemsCount(int)));
	connect(list, SIGNAL(addStatus(bool)), findE, SLOT(setDisabled(bool)));
	connect(findE, SIGNAL(textChanged(const QString &)), this, SLOT(findItems(const QString &)));
	connect(findE, SIGNAL(returnPressed()), this, SLOT(findNext()));

	QAction *act = new QAction(this);
	act->setShortcuts(QList<QKeySequence>() << QKeySequence("Return") << QKeySequence("Enter"));
	connect(act, SIGNAL(triggered()), this, SLOT(start()));
	act->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	addAction(act);
}

void PlaylistDock::stopThreads()
{
	list->updateEntryThr.stop();
	list->addThr.stop();
}

QString PlaylistDock::getUrl(QTreeWidgetItem *tWI) const
{
	return list->getUrl(tWI);
}
QString PlaylistDock::getCurrentItemName() const
{
	if (!list->currentItem())
		return QString();
	return list->currentItem()->text(0);
}

void PlaylistDock::load(const QString &url)
{
	if (!url.isEmpty())
		list->add({url}, nullptr, {}, true);
}
bool PlaylistDock::save(const QString &_url, bool saveCurrentGroup)
{
	const QString url = Functions::Url(_url);
	QList<QTreeWidgetItem *> parents;
	Playlist::Entries entries;
	for (QTreeWidgetItem *tWI : list->getChildren(PlaylistWidget::ALL_CHILDREN, saveCurrentGroup ? list->currentItem() : nullptr))
	{
		Playlist::Entry entry;
		if (PlaylistWidget::isGroup(tWI))
		{
			parents += tWI;
			entry.GID = parents.size();
		}
		else
		{
			entry.length = tWI->data(2, Qt::UserRole).isNull() ? -1.0 : tWI->data(2, Qt::UserRole).toDouble(); //FIXME: Is it possible to have a null variant for non-groups entries?
			entry.queue = tWI->text(1).toInt();
		}
		entry.url = tWI->data(0, Qt::UserRole).toString();
		entry.name = tWI->text(0);
		if (tWI->parent())
			entry.parent = parents.indexOf(tWI->parent()) + 1;
		if (tWI == list->currentItem())
			entry.flags |= Playlist::Entry::Selected;
		entry.flags |= PlaylistWidget::getFlags(tWI); //Additional flags
		entries += entry;
	}
	return Playlist::write(entries, url);
}

void PlaylistDock::add(const QStringList &urls)
{
	list->add(urls);
}
void PlaylistDock::addAndPlay(const QStringList &urls)
{
	playAfterAdd = true;
	list->dontUpdateAfterAdd = urls.size() == 1;
	list->add(urls, true);
}
void PlaylistDock::add(const QString &url)
{
	if (!url.isEmpty())
		add(QStringList{url});
}
void PlaylistDock::addAndPlay(const QString &_url)
{
	if (_url.isEmpty())
		return;
	/* If the entry exists, find and play it */
	const QList<QTreeWidgetItem *> items = list->getChildren(PlaylistWidget::ALL_CHILDREN);
	const QString url = Functions::Url(_url);
	for (QTreeWidgetItem *item : items)
	{
		QString itemUrl = item->data(0, Qt::UserRole).toString();
		bool urlMatches = (itemUrl == url);
		if (!urlMatches && Functions::splitPrefixAndUrlIfHasPluginPrefix(itemUrl, nullptr, &itemUrl, nullptr))
			urlMatches = (itemUrl == url); //E.g. for chiptunes without group
		if (urlMatches)
		{
			playAfterAdd = true;
			if (list->isGroup(item))
			{
				list->setCurrentItem(item);
				syncCurrentFolder();
				return;
			}
			if (!item->parent())
				list->addTopLevelItem(list->takeTopLevelItem(list->indexOfTopLevelItem(item)));
			addAndPlay(item);
			return;
		}
	}
	/**/
	addAndPlay(QStringList{_url});
}

void PlaylistDock::scrollToCurrectItem()
{
	list->scrollToItem(list->currentItem());
}

void PlaylistDock::expandTree(QTreeWidgetItem *i)
{
	while (i)
	{
		if (!i->isExpanded())
			i->setExpanded(true);
		i = i->parent();
	}
}

void PlaylistDock::toggleEntryFlag(const int flag)
{
	for (QTreeWidgetItem *tWI : list->selectedItems())
	{
		if (!list->isGroup(tWI))
		{
			const int entryFlags = PlaylistWidget::getFlags(tWI) ^ flag; //Toggle flag
			PlaylistWidget::setEntryFont(tWI, entryFlags);
			tWI->setData(0, Qt::UserRole + 1, entryFlags);
		}
	}
}

inline bool PlaylistDock::isRandomPlayback() const
{
	return (repeatMode == RandomMode || repeatMode == RandomGroupMode || repeatMode == RepeatRandom || repeatMode == RepeatRandomGroup);
}

void PlaylistDock::doGroupSync(bool quick, QTreeWidgetItem *tWI, bool quickRecursive)
{
	if (!tWI || !PlaylistWidget::isGroup(tWI))
		return;
	QString pth = tWI->data(0, Qt::UserRole).toString();
	if (pth.isEmpty() || (pth.startsWith("QMPlay2://") && !QMPlay2Core.hasResource(pth)))
		return;
	if (pth.startsWith("file://"))
		pth.remove(0, 7);
	const QFileInfo pthInfo(pth);
	const bool possibleModuleScheme = pth.contains("://");
	if (!possibleModuleScheme && !pthInfo.isDir() && !pthInfo.isFile())
	{
		tWI->setData(0, Qt::UserRole, QString());
		QMPlay2GUI.setTreeWidgetItemIcon(tWI, *QMPlay2GUI.groupIcon, 0, list);
		return;
	}
	if (pthInfo.isDir() && !pth.endsWith("/"))
		pth += "/";
	if (!quick)
	{
		findE->clear();
		list->sync(pth, tWI, !pthInfo.isDir() && (possibleModuleScheme || pthInfo.isFile()));
	}
	else if (pthInfo.isDir())
	{
		findE->clear();
		list->quickSync(pth, tWI, quickRecursive);
	}
}

void PlaylistDock::deleteTreeWidgetItem(QTreeWidgetItem *tWI)
{
	randomPlayedItems.removeOne(tWI);
	if (lastPlaying == tWI)
		lastPlaying = nullptr;
	delete tWI;
}

void PlaylistDock::itemDoubleClicked(QTreeWidgetItem *tWI)
{
	if (!tWI || PlaylistWidget::isGroup(tWI))
	{
		if (!tWI)
			emit stop();
		return;
	}

	if (!list->currentPlaying || list->currentItem() == list->currentPlaying)
		list->setCurrentItem(tWI);

	if (isRandomPlayback() && !randomPlayedItems.contains(tWI))
		randomPlayedItems.append(tWI);

	lastPlaying = tWI;

	//If in queue, remove from queue
	int idx = list->queue.indexOf(tWI);
	if (idx >= 0)
	{
		tWI->setText(1, QString());
		list->queue.removeOne(tWI);
		list->refresh(PlaylistWidget::REFRESH_QUEUE);
	}

	emit play(tWI->data(0, Qt::UserRole).toString());
}
void PlaylistDock::addAndPlay(QTreeWidgetItem *tWI)
{
	if (!tWI || !playAfterAdd)
		return;
	playAfterAdd = false;
	list->setCurrentItem(tWI);
	emit addAndPlayRestoreWindow();
	start();
}
void PlaylistDock::maybeDoQuickSync(QTreeWidgetItem *item)
{
	QTimer::singleShot(0, this, [=] { // Go through event queue
		if (list->canModify() && PlaylistWidget::isAlwaysSynced(item) && list->getChildren(PlaylistWidget::ONLY_GROUPS).contains(item))
			doGroupSync(true, item, false);
	});
}

void PlaylistDock::stopLoading()
{
	list->addThr.stop();
}
void PlaylistDock::next(bool playingError)
{
	QList<QTreeWidgetItem *> l = list->getChildren(PlaylistWidget::ONLY_NON_GROUPS);
	if (lastPlaying && !l.contains(lastPlaying))
		lastPlaying = nullptr;
	if (repeatMode == RepeatStopAfter)
	{
		emit stop();
		return;
	}
	QTreeWidgetItem *tWI = nullptr;
	if (!l.isEmpty())
	{
		if (isRandomPlayback())
		{
			if (repeatMode == RandomGroupMode || repeatMode == RepeatRandomGroup) //Random in group
			{
				QTreeWidgetItem *P = list->currentPlaying ? list->currentPlaying->parent() : (list->currentItem() ? list->currentItem()->parent() : nullptr);
				expandTree(P);
				l = P ? list->getChildren(PlaylistWidget::ONLY_NON_GROUPS, P) : list->topLevelNonGroupsItems();
				if (l.isEmpty() || (!randomPlayedItems.isEmpty() && randomPlayedItems.at(0)->parent() != P))
					randomPlayedItems.clear();
			}
			const bool playedEverything = (randomPlayedItems.count() == l.count());
			if (playedEverything)
				randomPlayedItems.clear();
			if (!playedEverything || (repeatMode == RepeatRandom || (repeatMode == RepeatRandomGroup && !l.isEmpty())))
			{
				if (l.count() == 1)
					tWI = l.at(0);
				else do
				{
					tWI = l.at(qrand() % l.count());
					if (PlaylistWidget::getFlags(tWI) & Playlist::Entry::Skip)
					{
						//Don't play skipped item.
						if (!randomPlayedItems.contains(tWI))
							randomPlayedItems.append(tWI);
						if (randomPlayedItems.count() == l.count())
						{
							//Break and stop playback if played everything ignoring skipped entries
							tWI = nullptr;
							break;
						}
					}
				} while (lastPlaying == tWI || randomPlayedItems.contains(tWI));
			}
		}
		else
		{
			int entryFlags = PlaylistWidget::getFlags(lastPlaying);
			if (entryFlags & Playlist::Entry::StopAfter)
			{
				//Remove "StopAfter" flag and stop playback.
				entryFlags &= ~Playlist::Entry::StopAfter;
				PlaylistWidget::setEntryFont(lastPlaying, entryFlags);
				lastPlaying->setData(0, Qt::UserRole + 1, entryFlags);
				//"tWI" is nullptr here, so playback will stop.
			}
			else
			{
				const bool canRepeat = sender() ? !qstrcmp(sender()->metaObject()->className(), "PlayClass") : false;
				if (canRepeat && repeatMode == RepeatEntry) //loop track
					tWI = lastPlaying ? lastPlaying : list->currentItem();
				else
				{
					QTreeWidgetItem *P = nullptr;
					if (!list->queue.size())
					{
						tWI = list->currentPlaying ? list->currentPlaying : list->currentItem();
						P = tWI ? tWI->parent() : nullptr;
						expandTree(P);
						for (;;)
						{
							tWI = list->itemBelow(tWI);
							if (canRepeat && repeatMode == RepeatGroup && P && (!tWI || tWI->parent() != P)) //loop group
							{
								const QList<QTreeWidgetItem *> l2 = list->getChildren(PlaylistWidget::ONLY_NON_GROUPS, P);
								if (!l2.isEmpty())
									tWI = l2[0];
								break;
							}
							if (PlaylistWidget::isGroup(tWI)) //group
							{
								if (!tWI->isExpanded())
									tWI->setExpanded(true);
								continue;
							}
							if (PlaylistWidget::getFlags(tWI) & Playlist::Entry::Skip) //skip item
								continue;
							break;
						}
					}
					else
						tWI = list->queue.first();
					if (canRepeat && (repeatMode == RepeatList || repeatMode == RepeatGroup) && !tWI && !l.isEmpty()) //loop list
						tWI = l.at(0);
				}
			}
		}
	}
	if (playingError && tWI == list->currentItem()) //don't play the same song if playback error occurred
	{
		if (isRandomPlayback())
			randomPlayedItems.append(tWI);
		emit stop();
	}
	else
		itemDoubleClicked(tWI);
}
void PlaylistDock::prev()
{
	QTreeWidgetItem *tWI = nullptr;
//Dla "wstecz" nie uwzględniam kolejkowania utworów
	tWI = list->currentPlaying ? list->currentPlaying : list->currentItem();
	if (tWI)
		expandTree(tWI->parent());
	for (;;)
	{
		QTreeWidgetItem *tmpI = list->itemAbove(tWI);
		if (PlaylistWidget::isGroup(tmpI) && !tmpI->isExpanded())
		{
			tmpI->setExpanded(true);
			continue;
		}
		tWI = list->itemAbove(tWI);
		if (!PlaylistWidget::isGroup(tWI))
			break;
	}
	itemDoubleClicked(tWI);
}
void PlaylistDock::skip()
{
	toggleEntryFlag(Playlist::Entry::Skip);
}
void PlaylistDock::stopAfter()
{
	toggleEntryFlag(Playlist::Entry::StopAfter);
}
void PlaylistDock::toggleLock()
{
	for (QTreeWidgetItem *tWI : list->selectedItems())
	{
		const int entryFlags = PlaylistWidget::getFlags(tWI) ^ Playlist::Entry::Locked; //Toggle "Locked" flag
		PlaylistWidget::setEntryFont(tWI, entryFlags);
		tWI->setData(0, Qt::UserRole + 1, entryFlags);
	}
	list->modifyMenu();
}
void PlaylistDock::alwaysSyncTriggered(bool checked)
{
	bool mustUpdateMenu = false;
	for (QTreeWidgetItem *tWI : list->selectedItems())
	{
		int entryFlags = PlaylistWidget::getFlags(tWI);
		if (PlaylistWidget::isAlwaysSynced(tWI, true))
		{
			mustUpdateMenu = (list->currentItem() == tWI);
			continue;
		}
		if (checked)
			entryFlags |= Playlist::Entry::AlwaysSync;
		else
			entryFlags &= ~Playlist::Entry::AlwaysSync;
		tWI->setData(0, Qt::UserRole + 1, entryFlags);
	}
	if (mustUpdateMenu)
		list->modifyMenu();
}
void PlaylistDock::start()
{
	itemDoubleClicked(list->currentItem());
}
void PlaylistDock::clearCurrentPlaying()
{
	if (list->currentPlaying)
	{
		if (list->currentPlayingItemIcon.type() == QVariant::Icon)
			QMPlay2GUI.setTreeWidgetItemIcon(list->currentPlaying, list->currentPlayingItemIcon.value<QIcon>(), 0, list);
		else
			list->currentPlaying->setData(0, Qt::DecorationRole, list->currentPlayingItemIcon);
	}
	list->clearCurrentPlaying();
}
void PlaylistDock::setCurrentPlaying()
{
	if (!lastPlaying || PlaylistWidget::isGroup(lastPlaying))
		return;
	list->currentPlayingUrl = getUrl(lastPlaying);
	list->setCurrentPlaying(lastPlaying);
}
void PlaylistDock::newGroup()
{
	list->setCurrentItem(list->newGroup());
	entryProperties();
}

void PlaylistDock::delEntries()
{
	if (!isVisible() || !list->canModify()) //jeżeli jest np. drag and drop to nie wolno usuwać
		return;
	const QList<QTreeWidgetItem *> selectedItems = list->selectedItems();
	if (!selectedItems.isEmpty())
	{
		QTreeWidgetItem *par = list->currentItem() ? list->currentItem()->parent() : nullptr;
		list->setItemsResizeToContents(false);
		for (QTreeWidgetItem *tWI : selectedItems)
		{
			if (!(PlaylistWidget::getFlags(tWI) & Playlist::Entry::Locked))
				deleteTreeWidgetItem(tWI);
		}
		list->refresh();
		if (list->currentItem())
			par = list->currentItem();
		else if (!list->getChildren().contains(par))
			par = nullptr;
		if (!par)
			par = list->topLevelItem(0);
		list->setCurrentItem(par);
		list->setItemsResizeToContents(true);
		list->processItems();
	}
}
void PlaylistDock::delNonGroupEntries(bool force)
{
	if (!list->canModify()) //jeżeli jest np. drag and drop to nie wolno usuwać
		return;
	if (force || QMessageBox::question(this, tr("Playlist"), tr("Are you sure you want to delete ungrouped entries?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		list->setItemsResizeToContents(false);
		for (QTreeWidgetItem *tWI : list->topLevelNonGroupsItems())
		{
			if (!(PlaylistWidget::getFlags(tWI) & Playlist::Entry::Locked))
				deleteTreeWidgetItem(tWI);
		}
		list->refresh();
		list->setItemsResizeToContents(true);
		list->processItems();
	}
}
void PlaylistDock::clear()
{
	if (QMessageBox::question(this, tr("Playlist"), tr("Are you sure you want to clear the list?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		lastPlaying = nullptr;
		list->clear();
	}
}
void PlaylistDock::copy()
{
	QMimeData *mimeData = new QMimeData;
	mimeData->setUrls(list->getUrls());
	if (mimeData->urls().isEmpty())
	{
		QApplication::clipboard()->clear();
		delete mimeData;
		return;
	}
	QApplication::clipboard()->setMimeData(mimeData);
}
void PlaylistDock::paste()
{
	const QMimeData *mimeData = QApplication::clipboard()->mimeData();
	if (Functions::chkMimeData(mimeData))
		list->add(Functions::getUrlsFromMimeData(mimeData), list->selectedItems().count() ? list->currentItem() : nullptr);
}
void PlaylistDock::renameGroup()
{
	QTreeWidgetItem *tWI = list->currentItem();
	if (!PlaylistWidget::isGroup(tWI))
		return;
	list->editItem(tWI);
}
void PlaylistDock::entryProperties()
{
	bool sync, accepted;
	EntryProperties eP(this, list->currentItem(), sync, accepted);
	if (accepted)
	{
		if (sync)
			syncCurrentFolder();
		list->modifyMenu();
		list->updateEntryThr.updateEntry(list->currentItem());
	}
}
void PlaylistDock::timeSort1()
{
	list->sortCurrentGroup(2, Qt::AscendingOrder);
	scrollToCurrectItem();
}
void PlaylistDock::timeSort2()
{
	list->sortCurrentGroup(2, Qt::DescendingOrder);
	scrollToCurrectItem();
}
void PlaylistDock::titleSort1()
{
	list->sortCurrentGroup(0, Qt::AscendingOrder);
	scrollToCurrectItem();
}
void PlaylistDock::titleSort2()
{
	list->sortCurrentGroup(0, Qt::DescendingOrder);
	scrollToCurrectItem();
}
void PlaylistDock::collapseAll()
{
	list->collapseAll();
}
void PlaylistDock::expandAll()
{
	list->expandAll();
}
void PlaylistDock::goToPlayback()
{
	if (list->currentPlaying)
	{
		list->clearSelection();
		list->setCurrentItem(list->currentPlaying);
		list->scrollToItem(list->currentPlaying);
	}
}
void PlaylistDock::queue()
{
	list->enqueue();
}
void PlaylistDock::findItems(const QString &txt)
{
	QList<QTreeWidgetItem *> itemsToShow = list->findItems(txt, Qt::MatchContains | Qt::MatchRecursive);
	list->processItems(&itemsToShow, !txt.isEmpty());
	if (txt.isEmpty())
	{
		const QList<QTreeWidgetItem *> selectedItems = list->selectedItems();
		if (!selectedItems.isEmpty())
			list->scrollToItem(selectedItems[0]);
		else if (list->currentPlaying)
			list->scrollToItem(list->currentPlaying);
	}
}
void PlaylistDock::findNext()
{
	bool belowSelection = false;
	QTreeWidgetItem *currentItem = list->currentItem();
	QTreeWidgetItem *firstItem = nullptr;
	for (QTreeWidgetItem *tWI : list->getChildren(PlaylistWidget::ALL_CHILDREN))
	{
		if (tWI->isHidden())
			continue;
		if (!firstItem && !PlaylistWidget::isGroup(tWI))
			firstItem = tWI;
		if (!belowSelection)
		{
			if (tWI == currentItem || !currentItem)
				belowSelection = true;
			if (currentItem)
				continue;
		}
		if (PlaylistWidget::isGroup(tWI))
			continue;
		list->setCurrentItem(tWI);
		return;
	}
	list->setCurrentItem(firstItem);
}
void PlaylistDock::visibleItemsCount(int count)
{
	if (count < 0)
		statusL->setText(tr("Playlist is loading now..."));
	else
		statusL->setText(tr("Visible entries") + ": " + QString::number(count));
}
void PlaylistDock::syncCurrentFolder()
{
	doGroupSync(false, list->currentItem());
}
void PlaylistDock::quickSyncCurrentFolder()
{
	doGroupSync(true, list->currentItem());
}
void PlaylistDock::repeat()
{
	if (QAction *act = qobject_cast<QAction *>(sender()))
	{
		const RepeatMode lastRepeatMode = repeatMode;
		repeatMode = (RepeatMode)act->property("enumValue").toInt();
		if (lastRepeatMode != repeatMode)
		{
			emit repeatEntry(repeatMode == RepeatEntry);
			switch (lastRepeatMode)
			{
				case RandomMode:
				case RandomGroupMode:
				case RepeatRandom:
				case RepeatRandomGroup:
					randomPlayedItems.clear();
					break;
				default:
					break;
			}
			if (list->currentPlaying && isRandomPlayback())
			{
				Q_ASSERT(randomPlayedItems.isEmpty());
				randomPlayedItems.append(list->currentPlaying);
			}
			emit QMPlay2Core.statusBarMessage(act->text().remove('&'), 1500);
		}
	}
}
void PlaylistDock::updateCurrentEntry(const QString &name, double length)
{
	list->updateEntryThr.updateEntry(list->currentPlaying, name, length);
}
