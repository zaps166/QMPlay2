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

#include <PlaylistWidget.hpp>
#include <PlaylistDock.hpp>

#include <EntryProperties.hpp>
#include <LineEdit.hpp>
#include <Settings.hpp>
#include <Main.hpp>

#include <QApplication>
#include <QInputDialog>
#include <QGridLayout>
#include <QMessageBox>
#include <QToolButton>
#include <QClipboard>
#include <QTabWidget>
#include <QFileInfo>
#include <QLineEdit>
#include <QMimeData>
#include <QPainter>
#include <QAction>
#include <QBuffer>
#include <QLabel>
#include <QDir>

PlaylistDock::PlaylistDock() :
	m_currPlaylist(nullptr),
	repeatMode(RepeatNormal),
	lastPlaying(nullptr)
{
	setWindowTitle(tr("Playlist"));
	setWidget(&mainW);

	findE = new LineEdit;
	findE->setToolTip(tr("Filter entries"));
	statusL = new QLabel;

	m_playlistsW = new QTabWidget;
	m_playlistsW->setTabsClosable(true);
	m_playlistsW->setMovable(true);

	QToolButton *newTab = new QToolButton;
	newTab->setIcon(QMPlay2Core.getIconFromTheme("list-add"));
	connect(newTab, SIGNAL(clicked(bool)), this, SLOT(newPlaylist()));
	m_playlistsW->setCornerWidget(newTab, Qt::TopLeftCorner);

	QGridLayout *layout = new QGridLayout(&mainW);
	layout->addWidget(m_playlistsW);
	layout->addWidget(findE);
	layout->addWidget(statusL);

	playAfterAdd = false;

	connect(findE, SIGNAL(textChanged(const QString &)), this, SLOT(findItems(const QString &)));
	connect(findE, SIGNAL(returnPressed()), this, SLOT(findNext()));
	connect(m_playlistsW, SIGNAL(tabCloseRequested(int)), this, SLOT(playlistsCloseTab(int)));
	connect(m_playlistsW, SIGNAL(currentChanged(int)), this, SLOT(playlistsTabsCurrentChanged(int)));
	connect(m_playlistsW, SIGNAL(tabBarDoubleClicked(int)), this, SLOT(playlistsTabDoubleClicked(int)));

	QAction *act = new QAction(this);
	act->setShortcuts(QList<QKeySequence>() << QKeySequence("Return") << QKeySequence("Enter"));
	connect(act, SIGNAL(triggered()), this, SLOT(start()));
	act->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	addAction(act);
}

void PlaylistDock::stopThreads()
{
	for (int i = 0, end = m_playlistsW->count(); i < end; ++i)
	{
		PlaylistWidget *list = (PlaylistWidget *)m_playlistsW->widget(i);
		list->updateEntryThr.stop();
		list->addThr.stop();
	}
}

QString PlaylistDock::getUrl(QTreeWidgetItem *tWI) const
{
	return m_currPlaylist->getUrl(tWI);
}
QString PlaylistDock::getCurrentItemName() const
{
	if (!m_currPlaylist->currentItem())
		return QString();
	return m_currPlaylist->currentItem()->text(0);
}

void PlaylistDock::load(const QString &url, const QString &name)
{
	if (!url.isEmpty())
	{
		PlaylistWidget *list = new PlaylistWidget;
		list->add({url}, nullptr, {}, true);

		connect(list, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(itemDoubleClicked(QTreeWidgetItem *)));
		connect(list, SIGNAL(returnItem(QTreeWidgetItem *)), this, SLOT(addAndPlay(QTreeWidgetItem *)));
		connect(list, &PlaylistWidget::itemExpanded, this, &PlaylistDock::maybeDoQuickSync, Qt::QueuedConnection); // Must be queued to not crash at startup in some cases
		connect(list, SIGNAL(visibleItemsCount(int)), this, SLOT(visibleItemsCount(int)));
		connect(list, SIGNAL(addStatus(bool)), findE, SLOT(setDisabled(bool)));

		m_playlistsW->addTab(list, (name.isEmpty() ? Functions::fileName(url) : name));
	}
}
bool PlaylistDock::save(const QString &_url, bool saveCurrentGroup, PlaylistWidget *_list)
{
	const QString url = Functions::Url(_url);
	QList<QTreeWidgetItem *> parents;
	Playlist::Entries entries;
	PlaylistWidget *list = (_list ? _list : m_currPlaylist);
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
		if (tWI == m_currPlaylist->currentItem())
			entry.flags |= Playlist::Entry::Selected;
		entry.flags |= PlaylistWidget::getFlags(tWI); //Additional flags
		entries += entry;
	}
	return Playlist::write(entries, url);
}
void PlaylistDock::loadAll()
{
	for (const QString &fname : QDir(QMPlay2Core.getSettingsDir() + "/Playlists").entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::DirsFirst))
	{
		const QString fileName = Functions::fileName(fname);
		const int end = fileName.lastIndexOf('.');
		const int start = fileName.indexOf('_') + 1;
		load(QMPlay2Core.getSettingsDir() + "/Playlists/" + fname, fileName.mid(start, end == -1 ? -1 : end - start));
	}
	if (m_playlistsW->count() == 0)
	{
		m_playlistsW->addTab(m_currPlaylist = new PlaylistWidget, tr("Playlist"));
	}
	m_playlistsW->setCurrentIndex(qBound(0, QMPlay2Core.getSettings().getInt("PlaylistDock/SelectedPlaylist"), m_playlistsW->count()));
}
void PlaylistDock::saveAll()
{
	const QString dir = QMPlay2Core.getSettingsDir() + "/Playlists/";
	for (const QString &fName : QDir(dir).entryList(QDir::Files | QDir::NoDotAndDotDot))
		QFile::remove(dir + fName);
	int length = QString::number(m_playlistsW->count()).length();
	for (int i = 0; i < m_playlistsW->count(); ++i)
		save(QString("%0/%2_%1.pls").arg(dir, m_playlistsW->tabText(i)).arg(i, length, 10, QChar('0')), false, (PlaylistWidget *)m_playlistsW->widget(i));
	QMPlay2Core.getSettings().set("PlaylistDock/SelectedPlaylist", m_playlistsW->currentIndex());
}

void PlaylistDock::add(const QStringList &urls)
{
	m_currPlaylist->add(urls);
}
void PlaylistDock::addAndPlay(const QStringList &urls)
{
	playAfterAdd = true;
	m_currPlaylist->dontUpdateAfterAdd = urls.size() == 1;
	m_currPlaylist->add(urls, true);
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
	const QList<QTreeWidgetItem *> items = m_currPlaylist->getChildren(PlaylistWidget::ALL_CHILDREN);
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
			if (m_currPlaylist->isGroup(item))
			{
				m_currPlaylist->setCurrentItem(item);
				syncCurrentFolder();
				return;
			}
			if (!item->parent())
				m_currPlaylist->addTopLevelItem(m_currPlaylist->takeTopLevelItem(m_currPlaylist->indexOfTopLevelItem(item)));
			addAndPlay(item);
			return;
		}
	}
	/**/
	addAndPlay(QStringList{_url});
}

void PlaylistDock::scrollToCurrectItem()
{
	m_currPlaylist->scrollToItem(m_currPlaylist->currentItem());
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
	for (QTreeWidgetItem *tWI : m_currPlaylist->selectedItems())
	{
		if (!m_currPlaylist->isGroup(tWI))
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
		QMPlay2GUI.setTreeWidgetItemIcon(tWI, *QMPlay2GUI.groupIcon, 0, m_currPlaylist);
		return;
	}
	if (pthInfo.isDir() && !pth.endsWith("/"))
		pth += "/";
	if (!quick)
	{
		findE->clear();
		m_currPlaylist->sync(pth, tWI, !pthInfo.isDir() && (possibleModuleScheme || pthInfo.isFile()));
	}
	else if (pthInfo.isDir())
	{
		findE->clear();
		m_currPlaylist->quickSync(pth, tWI, quickRecursive, lastPlaying);
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

	if (!m_currPlaylist->currentPlaying || m_currPlaylist->currentItem() == m_currPlaylist->currentPlaying)
		m_currPlaylist->setCurrentItem(tWI);

	if (isRandomPlayback() && !randomPlayedItems.contains(tWI))
		randomPlayedItems.append(tWI);

	lastPlaying = tWI;

	//If in queue, remove from queue
	int idx = m_currPlaylist->queue.indexOf(tWI);
	if (idx >= 0)
	{
		tWI->setText(1, QString());
		m_currPlaylist->queue.removeOne(tWI);
		m_currPlaylist->refresh(PlaylistWidget::REFRESH_QUEUE);
	}

	emit play(tWI->data(0, Qt::UserRole).toString());
}
void PlaylistDock::addAndPlay(QTreeWidgetItem *tWI)
{
	if (!tWI || !playAfterAdd)
		return;
	playAfterAdd = false;
	m_currPlaylist->setCurrentItem(tWI);
	emit addAndPlayRestoreWindow();
	start();
}
void PlaylistDock::maybeDoQuickSync(QTreeWidgetItem *item)
{
	if (m_currPlaylist->canModify() && PlaylistWidget::isAlwaysSynced(item) && m_currPlaylist->getChildren(PlaylistWidget::ONLY_GROUPS).contains(item))
		doGroupSync(true, item, false);
}

void PlaylistDock::playlistsTabsCurrentChanged(int index)
{
	m_currPlaylist = (PlaylistWidget *)m_playlistsW->widget(index);
}

void PlaylistDock::stopLoading()
{
	m_currPlaylist->addThr.stop();
}
void PlaylistDock::next(bool playingError)
{
	QList<QTreeWidgetItem *> l = m_currPlaylist->getChildren(PlaylistWidget::ONLY_NON_GROUPS);
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
				QTreeWidgetItem *P = m_currPlaylist->currentPlaying ? m_currPlaylist->currentPlaying->parent() : (m_currPlaylist->currentItem() ? m_currPlaylist->currentItem()->parent() : nullptr);
				expandTree(P);
				l = P ? m_currPlaylist->getChildren(PlaylistWidget::ONLY_NON_GROUPS, P) : m_currPlaylist->topLevelNonGroupsItems();
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
					tWI = lastPlaying ? lastPlaying : m_currPlaylist->currentItem();
				else
				{
					QTreeWidgetItem *P = nullptr;
					if (!m_currPlaylist->queue.size())
					{
						tWI = m_currPlaylist->currentPlaying ? m_currPlaylist->currentPlaying : m_currPlaylist->currentItem();
						P = tWI ? tWI->parent() : nullptr;
						expandTree(P);
						for (;;)
						{
							tWI = m_currPlaylist->itemBelow(tWI);
							if (canRepeat && repeatMode == RepeatGroup && P && (!tWI || tWI->parent() != P)) //loop group
							{
								const QList<QTreeWidgetItem *> l2 = m_currPlaylist->getChildren(PlaylistWidget::ONLY_NON_GROUPS, P);
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
						tWI = m_currPlaylist->queue.first();
					if (canRepeat && (repeatMode == RepeatList || repeatMode == RepeatGroup) && !tWI && !l.isEmpty()) //loop list
						tWI = l.at(0);
				}
			}
		}
	}
	if (playingError && tWI == m_currPlaylist->currentItem()) //don't play the same song if playback error occurred
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
	tWI = m_currPlaylist->currentPlaying ? m_currPlaylist->currentPlaying : m_currPlaylist->currentItem();
	if (tWI)
		expandTree(tWI->parent());
	for (;;)
	{
		QTreeWidgetItem *tmpI = m_currPlaylist->itemAbove(tWI);
		if (PlaylistWidget::isGroup(tmpI) && !tmpI->isExpanded())
		{
			tmpI->setExpanded(true);
			continue;
		}
		tWI = m_currPlaylist->itemAbove(tWI);
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
	for (QTreeWidgetItem *tWI : m_currPlaylist->selectedItems())
	{
		const int entryFlags = PlaylistWidget::getFlags(tWI) ^ Playlist::Entry::Locked; //Toggle "Locked" flag
		PlaylistWidget::setEntryFont(tWI, entryFlags);
		tWI->setData(0, Qt::UserRole + 1, entryFlags);
	}
	m_currPlaylist->modifyMenu();
}
void PlaylistDock::alwaysSyncTriggered(bool checked)
{
	bool mustUpdateMenu = false;
	for (QTreeWidgetItem *tWI : m_currPlaylist->selectedItems())
	{
		int entryFlags = PlaylistWidget::getFlags(tWI);
		if (PlaylistWidget::isAlwaysSynced(tWI, true))
		{
			mustUpdateMenu = (m_currPlaylist->currentItem() == tWI);
			continue;
		}
		if (checked)
			entryFlags |= Playlist::Entry::AlwaysSync;
		else
			entryFlags &= ~Playlist::Entry::AlwaysSync;
		tWI->setData(0, Qt::UserRole + 1, entryFlags);
	}
	if (mustUpdateMenu)
		m_currPlaylist->modifyMenu();
}
void PlaylistDock::start()
{
	itemDoubleClicked(m_currPlaylist->currentItem());
}
void PlaylistDock::clearCurrentPlaying()
{
	if (m_currPlaylist->currentPlaying)
	{
		if (m_currPlaylist->currentPlayingItemIcon.type() == QVariant::Icon)
			QMPlay2GUI.setTreeWidgetItemIcon(m_currPlaylist->currentPlaying, m_currPlaylist->currentPlayingItemIcon.value<QIcon>(), 0, m_currPlaylist);
		else
			m_currPlaylist->currentPlaying->setData(0, Qt::DecorationRole, m_currPlaylist->currentPlayingItemIcon);
	}
	m_currPlaylist->clearCurrentPlaying();
}
void PlaylistDock::setCurrentPlaying()
{
	if (!lastPlaying || PlaylistWidget::isGroup(lastPlaying))
		return;
	m_currPlaylist->currentPlayingUrl = getUrl(lastPlaying);
	m_currPlaylist->setCurrentPlaying(lastPlaying);
}
void PlaylistDock::newGroup()
{
	m_currPlaylist->setCurrentItem(m_currPlaylist->newGroup());
	entryProperties();
}

void PlaylistDock::delEntries()
{
	if (!isVisible() || !m_currPlaylist->canModify()) //jeżeli jest np. drag and drop to nie wolno usuwać
		return;
	const QList<QTreeWidgetItem *> selectedItems = m_currPlaylist->selectedItems();
	if (!selectedItems.isEmpty())
	{
		QTreeWidgetItem *par = m_currPlaylist->currentItem() ? m_currPlaylist->currentItem()->parent() : nullptr;
		m_currPlaylist->setItemsResizeToContents(false);
		for (QTreeWidgetItem *tWI : selectedItems)
		{
			if (!(PlaylistWidget::getFlags(tWI) & Playlist::Entry::Locked))
				deleteTreeWidgetItem(tWI);
		}
		m_currPlaylist->refresh();
		if (m_currPlaylist->currentItem())
			par = m_currPlaylist->currentItem();
		else if (!m_currPlaylist->getChildren().contains(par))
			par = nullptr;
		if (!par)
			par = m_currPlaylist->topLevelItem(0);
		m_currPlaylist->setCurrentItem(par);
		m_currPlaylist->setItemsResizeToContents(true);
		m_currPlaylist->processItems();
	}
}
void PlaylistDock::delNonGroupEntries(bool force)
{
	if (!m_currPlaylist->canModify()) //jeżeli jest np. drag and drop to nie wolno usuwać
		return;
	if (force || QMessageBox::question(this, tr("Playlist"), tr("Are you sure you want to delete ungrouped entries?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		m_currPlaylist->setItemsResizeToContents(false);
		for (QTreeWidgetItem *tWI : m_currPlaylist->topLevelNonGroupsItems())
		{
			if (!(PlaylistWidget::getFlags(tWI) & Playlist::Entry::Locked))
				deleteTreeWidgetItem(tWI);
		}
		m_currPlaylist->refresh();
		m_currPlaylist->setItemsResizeToContents(true);
		m_currPlaylist->processItems();
	}
}
void PlaylistDock::clear()
{
	if (QMessageBox::question(this, tr("Playlist"), tr("Are you sure you want to clear the list?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		lastPlaying = nullptr;
		m_currPlaylist->clear();
	}
}
void PlaylistDock::copy()
{
	QMimeData *mimeData = new QMimeData;
	mimeData->setUrls(m_currPlaylist->getUrls());
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
		m_currPlaylist->add(Functions::getUrlsFromMimeData(mimeData), m_currPlaylist->selectedItems().count() ? m_currPlaylist->currentItem() : nullptr);
}
void PlaylistDock::renameGroup()
{
	QTreeWidgetItem *tWI = m_currPlaylist->currentItem();
	if (!PlaylistWidget::isGroup(tWI))
		return;
	m_currPlaylist->editItem(tWI);
}
void PlaylistDock::entryProperties()
{
	bool sync, accepted;
	EntryProperties eP(this, m_currPlaylist->currentItem(), sync, accepted);
	if (accepted)
	{
		if (sync)
			syncCurrentFolder();
		m_currPlaylist->modifyMenu();
		m_currPlaylist->updateEntryThr.updateEntry(m_currPlaylist->currentItem());
	}
}
void PlaylistDock::timeSort1()
{
	m_currPlaylist->sortCurrentGroup(2, Qt::AscendingOrder);
	scrollToCurrectItem();
}
void PlaylistDock::timeSort2()
{
	m_currPlaylist->sortCurrentGroup(2, Qt::DescendingOrder);
	scrollToCurrectItem();
}
void PlaylistDock::titleSort1()
{
	m_currPlaylist->sortCurrentGroup(0, Qt::AscendingOrder);
	scrollToCurrectItem();
}
void PlaylistDock::titleSort2()
{
	m_currPlaylist->sortCurrentGroup(0, Qt::DescendingOrder);
	scrollToCurrectItem();
}
void PlaylistDock::collapseAll()
{
	m_currPlaylist->collapseAll();
}
void PlaylistDock::expandAll()
{
	m_currPlaylist->expandAll();
}
void PlaylistDock::goToPlayback()
{
	if (m_currPlaylist->currentPlaying)
	{
		m_currPlaylist->clearSelection();
		m_currPlaylist->setCurrentItem(m_currPlaylist->currentPlaying);
		m_currPlaylist->scrollToItem(m_currPlaylist->currentPlaying);
	}
}
void PlaylistDock::queue()
{
	m_currPlaylist->enqueue();
}
void PlaylistDock::findItems(const QString &txt)
{
	QList<QTreeWidgetItem *> itemsToShow = m_currPlaylist->findItems(txt, Qt::MatchContains | Qt::MatchRecursive);
	m_currPlaylist->processItems(&itemsToShow, !txt.isEmpty());
	if (txt.isEmpty())
	{
		const QList<QTreeWidgetItem *> selectedItems = m_currPlaylist->selectedItems();
		if (!selectedItems.isEmpty())
			m_currPlaylist->scrollToItem(selectedItems[0]);
		else if (m_currPlaylist->currentPlaying)
			m_currPlaylist->scrollToItem(m_currPlaylist->currentPlaying);
	}
}
void PlaylistDock::findNext()
{
	bool belowSelection = false;
	QTreeWidgetItem *currentItem = m_currPlaylist->currentItem();
	QTreeWidgetItem *firstItem = nullptr;
	for (QTreeWidgetItem *tWI : m_currPlaylist->getChildren(PlaylistWidget::ALL_CHILDREN))
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
		m_currPlaylist->setCurrentItem(tWI);
		return;
	}
	m_currPlaylist->setCurrentItem(firstItem);
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
	doGroupSync(false, m_currPlaylist->currentItem());
}
void PlaylistDock::quickSyncCurrentFolder()
{
	doGroupSync(true, m_currPlaylist->currentItem());
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
			if (m_currPlaylist->currentPlaying && isRandomPlayback())
			{
				Q_ASSERT(randomPlayedItems.isEmpty());
				randomPlayedItems.append(m_currPlaylist->currentPlaying);
			}
			emit QMPlay2Core.statusBarMessage(act->text().remove('&'), 1500);
		}
	}
}
void PlaylistDock::updateCurrentEntry(const QString &name, double length)
{
	m_currPlaylist->updateEntryThr.updateEntry(m_currPlaylist->currentPlaying, name, length);
}

void PlaylistDock::newPlaylist()
{
	m_playlistsW->addTab(new PlaylistWidget, tr("Playlist"));
}
void PlaylistDock::playlistsCloseTab(int index)
{
	if (index == -1)
		index = m_playlistsW->currentIndex();
	PlaylistWidget *list = (PlaylistWidget *)m_playlistsW->widget(index);
	m_playlistsW->removeTab(index);

	if (m_playlistsW->count() == 0)
		m_playlistsW->addTab(m_currPlaylist = new PlaylistWidget, tr("Playlist"));
	else
		m_currPlaylist = (PlaylistWidget *)m_playlistsW->currentWidget();

	delete list;
}
void PlaylistDock::playlistsTabDoubleClicked(int index)
{
	if (index == -1)
		index = m_playlistsW->currentIndex();
	bool ok;
	const QString prevName = m_playlistsW->tabText(index);
	const QString newName = QInputDialog::getText(this, tr("Rename Playlist Name"), tr("Rename \"%1\" to:").arg(prevName), QLineEdit::Normal, prevName, &ok);
	if (ok && !newName.isEmpty() && newName != prevName)
		m_playlistsW->setTabText(index, newName);
}
