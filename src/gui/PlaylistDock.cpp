/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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
	lastPlaying(NULL)
{
	setWindowTitle(tr("Playlist"));
	setWidget(&mainW);

	list = new PlaylistWidget;
	findE = new LineEdit;
	findE->setToolTip(tr("Filter entries"));
	statusL = new QLabel;

	layout = new QGridLayout(&mainW);
	layout->addWidget(list);
	layout->addWidget(findE);
	layout->addWidget(statusL);

	playAfterAdd = false;

	connect(list, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(itemDoubleClicked(QTreeWidgetItem *)));
	connect(list, SIGNAL(returnItem(QTreeWidgetItem *)), this, SLOT(addAndPlay(QTreeWidgetItem *)));
	connect(list, SIGNAL(visibleItemsCount(int)), this, SLOT(visibleItemsCount(int)));
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
		list->add(QStringList(url), NULL, true);
}
bool PlaylistDock::save(const QString &_url, bool saveCurrentGroup)
{
	const QString url = Functions::Url(_url);
	QList<QTreeWidgetItem *> parents;
	Playlist::Entries entries;
	foreach (QTreeWidgetItem *tWI, list->getChildren(PlaylistWidget::ALL_CHILDREN, saveCurrentGroup ? list->currentItem() : NULL))
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
		entry.selected = tWI == list->currentItem();
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
		add(QStringList(url));
}
void PlaylistDock::addAndPlay(const QString &_url)
{
	if (_url.isEmpty())
		return;
	/* Jeżeli wpis istnieje, znajdzie go i zacznie odtwarzać */
	const QList<QTreeWidgetItem *> items = list->getChildren(PlaylistWidget::ALL_CHILDREN);
	const QString url = Functions::Url(_url);
	foreach (QTreeWidgetItem *item, items)
	{
		QString itemUrl = item->data(0, Qt::UserRole).toString();
		bool urlMatches = itemUrl == url;
		if (!urlMatches && Functions::splitPrefixAndUrlIfHasPluginPrefix(itemUrl, NULL, &itemUrl, NULL))
			urlMatches = itemUrl == url;
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
	addAndPlay(QStringList(_url));
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

inline bool PlaylistDock::isRandomPlayback() const
{
	return (repeatMode == RandomMode || repeatMode == RandomGroupMode || repeatMode == RepeatRandom || repeatMode == RepeatRandomGroup);
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

	//Jeżeli jest w kolejce, usuń z kolejki
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
	start();
}

void PlaylistDock::stopLoading()
{
	list->addThr.stop();
}
void PlaylistDock::next(bool playingError)
{
	QList<QTreeWidgetItem *> l = list->getChildren(PlaylistWidget::ONLY_NON_GROUPS);
	if (!l.contains(lastPlaying))
		lastPlaying = NULL;
	QTreeWidgetItem *tWI = NULL;
	if (!l.isEmpty())
	{
		if (isRandomPlayback())
		{
			if (repeatMode == RandomGroupMode || repeatMode == RepeatRandomGroup) //Random in group
			{
				QTreeWidgetItem *P = list->currentPlaying ? list->currentPlaying->parent() : (list->currentItem() ? list->currentItem()->parent() : NULL);
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
				int r;
				if (l.count() == 1)
					r = 0;
				else
				{
					do
						r = qrand() % l.count();
					while (lastPlaying == l.at(r) || randomPlayedItems.contains(l.at(r)));
				}
				tWI = l.at(r);
			}
		}
		else
		{
			const bool canRepeat = sender() ? !qstrcmp(sender()->metaObject()->className(), "PlayClass") : false;
			if (canRepeat && repeatMode == RepeatEntry) //zapętlenie utworu
				tWI = lastPlaying ? lastPlaying : list->currentItem();
			else
			{
				QTreeWidgetItem *P = NULL;
				if (!list->queue.size())
				{
					tWI = list->currentPlaying ? list->currentPlaying : list->currentItem();
					P = tWI ? tWI->parent() : NULL;
					expandTree(P);
					for (;;)
					{
						tWI = list->itemBelow(tWI);
						if (canRepeat && repeatMode == RepeatGroup && P && (!tWI || tWI->parent() != P)) //zapętlenie grupy
						{
							const QList<QTreeWidgetItem *> l2 = list->getChildren(PlaylistWidget::ONLY_NON_GROUPS, P);
							if (!l2.isEmpty())
								tWI = l2[0];
							break;
						}
						if (PlaylistWidget::isGroup(tWI)) //grupa
						{
							if (!tWI->isExpanded())
								tWI->setExpanded(true);
							continue;
						}
						break;
					}
				}
				else
					tWI = list->queue.first();
				if (canRepeat && (repeatMode == RepeatList || repeatMode == RepeatGroup) && !tWI && !l.isEmpty()) //zapętlenie listy
					tWI = l.at(0);
			}
		}
	}
	if (playingError && tWI == list->currentItem()) //nie ponawiaj odtwarzania tego samego utworu, jeżeli wystąpił błąd przy jego odtwarzaniu
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
	QTreeWidgetItem *tWI = NULL;
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
void PlaylistDock::start()
{
	itemDoubleClicked(list->currentItem());
}
void PlaylistDock::clearCurrentPlaying()
{
	if (list->currentPlaying)
		list->currentPlaying->setIcon(0, list->currentPlayingItemIcon);
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
		QTreeWidgetItem *par = list->currentItem() ? list->currentItem()->parent() : NULL;
		list->setItemsResizeToContents(false);
		foreach (QTreeWidgetItem *tWI, selectedItems)
		{
			randomPlayedItems.removeOne(tWI);
			delete tWI;
		}
		list->refresh();
		if (list->currentItem())
			par = list->currentItem();
		else if (!list->getChildren().contains(par))
			par = NULL;
		if (!par)
			par = list->topLevelItem(0);
		list->setCurrentItem(par);
		list->setItemsResizeToContents(true);
		list->processItems();
	}
}
void PlaylistDock::delNonGroupEntries()
{
	if (!list->canModify()) //jeżeli jest np. drag and drop to nie wolno usuwać
		return;
	if (QMessageBox::question(this, tr("Playlist"), tr("Are you sure you want to delete not grouped entries?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		list->setItemsResizeToContents(false);
		foreach (QTreeWidgetItem *tWI, list->topLevelNonGroupsItems())
		{
			randomPlayedItems.removeOne(tWI);
			delete tWI;
		}
		list->refresh();
		list->setItemsResizeToContents(true);
		list->processItems();
	}
}
void PlaylistDock::clear()
{
	if (QMessageBox::question(this, tr("Playlist"), tr("Are you sure you want to clear the list?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
		list->clear();
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
		list->add(Functions::getUrlsFromMimeData(mimeData), list->selectedItems().count() ? list->currentItem() : NULL);
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
	list->sortItems(2, Qt::AscendingOrder);
	scrollToCurrectItem();
}
void PlaylistDock::timeSort2()
{
	list->sortItems(2, Qt::DescendingOrder);
	scrollToCurrectItem();
}
void PlaylistDock::titleSort1()
{
	list->sortItems(0, Qt::AscendingOrder);
	scrollToCurrectItem();
}
void PlaylistDock::titleSort2()
{
	list->sortItems(0, Qt::DescendingOrder);
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
	QTreeWidgetItem *firstItem = NULL;
	foreach (QTreeWidgetItem *tWI, list->getChildren(PlaylistWidget::ALL_CHILDREN))
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
	QTreeWidgetItem *tWI = list->currentItem();
	if (!(tWI && PlaylistWidget::isGroup(tWI)))
		return;
	QString pth = tWI->data(0, Qt::UserRole).toString();
	if (pth.isEmpty())
		return;
	if (pth.startsWith("file://"))
		pth.remove(0, 7);
	const QFileInfo pthInfo(pth);
	const bool possibleModuleScheme = pth.contains("://");
	if (!possibleModuleScheme && !pthInfo.isDir() && !pthInfo.isFile())
	{
		tWI->setData(0, Qt::UserRole, QString());
		tWI->setIcon(0, *QMPlay2GUI.groupIcon);
		return;
	}
	if (pthInfo.isDir() && pth.right(1) != "/")
		pth += "/";
	findE->clear();
	list->sync(pth, tWI, !pthInfo.isDir() && (possibleModuleScheme || pthInfo.isFile()));
}
void PlaylistDock::repeat()
{
	if (QAction *act = qobject_cast<QAction *>(sender()))
	{
		const RepeatMode lastRepeatMode = repeatMode;
		repeatMode = (RepeatMode)act->property("enumValue").toInt();
		if (lastRepeatMode != repeatMode)
		{
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
