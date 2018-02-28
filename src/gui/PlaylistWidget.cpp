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

#include <QMPlay2Extensions.hpp>
#include <MenuBar.hpp>
#include <Demuxer.hpp>
#include <Reader.hpp>
#include <Main.hpp>

#include <QResizeEvent>
#include <QHeaderView>
#include <QFileInfo>
#include <QMimeData>
#include <QPainter>
#include <QDrag>
#include <QMenu>
#include <QDir>

static inline QStringList getDirEntries(const QString &pth)
{
	return QDir(pth).entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::DirsFirst);
}
static inline void prependPath(QStringList &dirEntries, const QString &pth)
{
	for (int i = 0; i < dirEntries.count(); ++i)
		dirEntries[i].prepend(pth);
}

static void entryCreated(const QString &url, int insertChildAt, QStringList &existingEntries)
{
	const QString fileName = Functions::fileName(url);
	if (insertChildAt < 0 || insertChildAt >= existingEntries.count())
		existingEntries.append(fileName);
	else
		existingEntries.insert(insertChildAt, fileName);
}

static inline MenuBar::Playlist *playlistMenu()
{
	return QMPlay2GUI.menuBar->playlist;
}

/* UpdateEntryThr class */
UpdateEntryThr::UpdateEntryThr(PlaylistWidget &pLW) :
	pendingUpdates(0),
	pLW(pLW),
	timeChanged(false)
{
	qRegisterMetaType<ItemUpdated>("ItemUpdated");
	qRegisterMetaType<QTreeWidgetItem *>("QTreeWidgetItem *");
	connect(this, SIGNAL(finished()), this, SLOT(finished()));
}

void UpdateEntryThr::updateEntry(QTreeWidgetItem *item, const QString &name, double length)
{
	if (!item || !pLW.getChildren(PlaylistWidget::ONLY_NON_GROUPS).contains(item))
		return;
	mutex.lock();
	itemsToUpdate += {item, pLW.getUrl(item), item->data(2, Qt::UserRole).toDouble(), name, length};
	mutex.unlock();
	if (!isRunning())
	{
		ioCtrl.resetAbort();
		start();
	}
}

void UpdateEntryThr::run()
{
	const bool displayOnlyFileName = QMPlay2Core.getSettings().getBool("DisplayOnlyFileName");
	while (!ioCtrl.isAborted())
	{
		mutex.lock();
		if (itemsToUpdate.isEmpty())
		{
			mutex.unlock();
			break;
		}
		ItemToUpdate itu = itemsToUpdate.dequeue();
		mutex.unlock();

		bool updateTitle = true;
		QString url = itu.url;

		ItemUpdated iu;
		iu.item = itu.item;

		if (itu.name.isNull() && itu.length == -2.0)
		{
			Functions::getDataIfHasPluginPrefix(url, &url, &itu.name, &iu.icon, &ioCtrl);
			iu.updateIcon = true;

			IOController<Demuxer> &demuxer = ioCtrl.toRef<Demuxer>();
			if (Demuxer::create(url, demuxer))
			{
				if (!displayOnlyFileName && itu.name.isEmpty())
					itu.name = demuxer->title();
				itu.length = demuxer->length();
				demuxer.reset();
			}
			else
				updateTitle = false;
		}
		else
		{
			iu.updateIcon = false;
		}

		//Don't update title for network streams if title exists and new title doesn't exists
		if (updateTitle && (displayOnlyFileName || url.startsWith("file://") || !itu.name.isEmpty()))
		{
			if (displayOnlyFileName || itu.name.isEmpty())
				iu.name = Functions::fileName(url, false);
			else
				iu.name = itu.name;
		}

		if (qFuzzyCompare(itu.length, itu.oldLength))
			iu.updateLength = false;
		else
		{
			iu.updateLength = true;
			iu.length = itu.length;
			timeChanged = true;
		}

		pendingUpdates.ref();
		QMetaObject::invokeMethod(this, "updateItem", Q_ARG(ItemUpdated, iu));
	}
}
void UpdateEntryThr::stop()
{
	ioCtrl.abort();
	wait(TERMINATE_TIMEOUT);
	if (isRunning())
	{
		terminate();
		wait(1000);
		ioCtrl.reset();
	}
}

void UpdateEntryThr::updateItem(ItemUpdated iu)
{
	if (iu.updateIcon)
		pLW.setEntryIcon(iu.icon, iu.item);
	if (!iu.name.isNull())
		iu.item->setText(0, iu.name);
	if (iu.updateLength)
	{
		iu.item->setText(2, Functions::timeToStr(iu.length));
		iu.item->setData(2, Qt::UserRole, iu.length);
	}
	if (iu.item == pLW.currentPlaying)
		QMetaObject::invokeMethod(QMPlay2GUI.mainW, "updateWindowTitle", Q_ARG(QString, iu.item->text(0)));
	pendingUpdates.deref();
}
void UpdateEntryThr::finished()
{
	if (timeChanged)
	{
		pLW.refresh(PlaylistWidget::REFRESH_GROUPS_TIME);
		timeChanged = false;
	}
	pLW.modifyMenu();
}

/* AddThr class */
AddThr::AddThr(PlaylistWidget &pLW) :
	pLW(pLW),
	inProgress(false)
{
	connect(this, SIGNAL(finished()), this, SLOT(finished()));
}

void AddThr::setData(const QStringList &_urls, const QStringList &_existingEntries, QTreeWidgetItem *_par, bool _loadList, SYNC _sync)
{
	ioCtrl.resetAbort();

	urls = _urls;
	existingEntries = _existingEntries;
	par = _par;
	loadList = _loadList;
	sync = _sync;

	firstItem = nullptr;
	lastItem = pLW.currentItem();

	pLW.setItemsResizeToContents(false);

	emit pLW.visibleItemsCount(-1);

	if (sync)
	{
		while (par->childCount())
			delete par->child(0);
		pLW.refresh(PlaylistWidget::REFRESH_CURRPLAYING);
	}

	if (!inProgress)
	{
		emit status(true);
		emit QMPlay2Core.busyCursor();
	}

	if (!loadList)
	{
		if (!inProgress)
		{
			pLW.rotation = 0;
			pLW.animationTimer.start(50);
			playlistMenu()->stopLoading->setVisible(true);
			inProgress = true;
		}
		start();
	}
	else
	{
		inProgress = true;
		run();
	}
}
void AddThr::setDataForSync(const QString &pth, QTreeWidgetItem *par, bool notDir)
{
	if (notDir)
		setData({pth}, {}, par, false, FILE_SYNC); //File synchronization needs only one file!
	else
	{
		QStringList dirEntries = getDirEntries(pth);
		prependPath(dirEntries, pth);
		setData(dirEntries, {}, par, false, DIR_SYNC);
	}
}

void AddThr::stop()
{
	if (pLW.addTimer.isActive())
	{
		pLW.addTimer.stop();
		pLW.enqueuedAddData.clear();
	}
	ioCtrl.abort();
	wait(TERMINATE_TIMEOUT);
	if (isRunning())
	{
		terminate();
		wait(1000);
		ioCtrl.reset();
	}
}

void AddThr::changeItemText0(QTreeWidgetItem *tWI, QString name)
{
	if (!tWI->data(0, Qt::UserRole + 2).toBool()) //It can be set in "EntryProperties::accept()"
		tWI->setText(0, name);
	tWI->setData(0, Qt::UserRole + 2, QVariant());
}
void AddThr::modifyItemFlags(QTreeWidgetItem *tWI, int flags)
{
	PlaylistWidget::setEntryFont(tWI, flags);
	tWI->setData(0, Qt::UserRole + 1, flags);
}
void AddThr::deleteTreeWidgetItem(QTreeWidgetItem *tWI)
{
	delete tWI;
}

void AddThr::run()
{
	Functions::DemuxersInfo demuxersInfo;
	for (Module *module : QMPlay2Core.getPluginsInstance())
		for (const Module::Info &mod : module->getModulesInfo())
			if (mod.type == Module::DEMUXER)
				demuxersInfo += {mod.name, mod.icon.isNull() ? module->icon() : mod.icon, mod.extensions};
	add(urls, par, demuxersInfo, existingEntries.isEmpty() ? nullptr : &existingEntries, loadList);
	if (currentThread() == pLW.thread()) //jeżeli funkcja działa w głównym wątku
		finished();
}

bool AddThr::add(const QStringList &urls, QTreeWidgetItem *parent, const Functions::DemuxersInfo &demuxersInfo, QStringList *existingEntries, bool loadList)
{
	const bool displayOnlyFileName = QMPlay2Core.getSettings().getBool("DisplayOnlyFileName");
	QTreeWidgetItem *currentItem = parent;
	bool added = false;
	for (int i = 0; i < urls.size(); ++i)
	{
		if (ioCtrl.isAborted())
			break;

		const QString entryName = QMPlay2Core.getNameForUrl(urls.at(i)); // Get the default entry name - it'll be used if doesn't exist in stream

		QString url = Functions::Url(urls.at(i));

		int insertChildAt = -1;
		if (existingEntries)
		{
			//For quick group sync only - find where to place the new item
			const QString newUrl = url.startsWith("file://") ? url.mid(7) : url;
			const QString newFileName = Functions::fileName(newUrl);
			const QString filePath = Functions::filePath(newUrl);
			const bool newIsDir = QFileInfo(newUrl).isDir();
			insertChildAt = existingEntries->count();
			for (int i = 0; i < insertChildAt; ++i)
			{
				const QString fileName = existingEntries->at(i);
				const bool isDir = QFileInfo(filePath + fileName).isDir();
				if ((newIsDir && !isDir) || (newIsDir == isDir && fileName > newFileName))
				{
					insertChildAt = i;
					break;
				}
			}
		}

		QString name;
		const Playlist::Entries entries = Playlist::read(url, &name);
		if (!name.isEmpty()) //Loading playlist
		{
			QTreeWidgetItem *playlistParent = currentItem;
			if (loadList) //Loading QMPlay2 playlist on startup
				pLW.clear(); //This can be executed only from GUI thread!
			else
			{
				const QString groupName = Functions::fileName(url, false);
				if (sync != FILE_SYNC)
					playlistParent = pLW.newGroup(groupName, url, currentItem, insertChildAt, existingEntries); //Adding a new playlist group
				else
				{
					//Reuse current playlist group
					QMetaObject::invokeMethod(this, "changeItemText0", Q_ARG(QTreeWidgetItem *, currentItem), Q_ARG(QString, groupName));
					sync = NO_SYNC;
				}
			}
			QTreeWidgetItem *tmpFirstItem = insertPlaylistEntries(entries, playlistParent, demuxersInfo, insertChildAt, existingEntries);
			if (!firstItem)
				firstItem = tmpFirstItem;
			added = !entries.isEmpty();
			if (loadList)
				break;
		}
		else if (!loadList)
		{
			const QString dUrl = url.startsWith("file://") ? url.mid(7) : QString();
			if (QFileInfo(dUrl).isDir()) //dodawanie podkatalogu
			{
				if (pLW.currPthToSave.isNull())
					pLW.currPthToSave = dUrl;
				QStringList dirEntries = getDirEntries(dUrl);
				if (!dirEntries.isEmpty())
				{
					url = Functions::cleanPath(url);
					QTreeWidgetItem *p = pLW.newGroup(Functions::fileName(url), url, currentItem, insertChildAt, existingEntries);
					for (int j = dirEntries.size() - 1; j >= 0; j--)
					{
						dirEntries[j].prepend(url);
#ifdef Q_OS_WIN
						dirEntries[j].replace("file://", "file:///");
#endif
					}
					if (add(dirEntries, p, demuxersInfo))
						added = true;
					else
					{
						if (existingEntries)
							existingEntries->removeOne(p->text(0));
						QMetaObject::invokeMethod(this, "deleteTreeWidgetItem", Q_ARG(QTreeWidgetItem *, p));
					}
				}
			}
			else
			{
				bool hasOneEntry = pLW.dontUpdateAfterAdd;
				bool tracksAdded = false;

				Playlist::Entry entry;
				entry.url = url;

				if (!pLW.dontUpdateAfterAdd) //Don't try to get the real address from extension plugin in this case (no needed for tracks)
					Functions::getDataIfHasPluginPrefix(url, &url, &entry.name, nullptr, &ioCtrl, demuxersInfo);
				IOController<Demuxer> &demuxer = ioCtrl.toRef<Demuxer>();
				Demuxer::FetchTracks fetchTracks(pLW.dontUpdateAfterAdd);
				if (Demuxer::create(url, demuxer, &fetchTracks))
				{
					if (sync == FILE_SYNC && fetchTracks.tracks.count() <= 1)
						hasOneEntry = false; //Don't allow adding single file when syncing a file group
					else if (fetchTracks.tracks.isEmpty())
					{
						if (!displayOnlyFileName && entry.name.isEmpty())
							entry.name = demuxer->title();
						entry.length = demuxer->length();
						hasOneEntry = true;
					}
					else
					{
						QTreeWidgetItem *tmpFirstItem = insertPlaylistEntries(fetchTracks.tracks, currentItem, demuxersInfo, insertChildAt, existingEntries);
						if (!firstItem)
							firstItem = tmpFirstItem;
						hasOneEntry = false;
						tracksAdded = true;
					}
					demuxer.reset();
				}
				else if (!fetchTracks.isOK)
					hasOneEntry = false; //Don't add entry to list if error occured

				if (sync == FILE_SYNC && (!fetchTracks.isOK || fetchTracks.tracks.count() <= 1))
				{
					 //Change group name to "url" if error or only single file for file sync
					QString groupName = url;
					if (groupName.startsWith("file://"))
						groupName.remove("file://");
					QMetaObject::invokeMethod(this, "changeItemText0", Q_ARG(QTreeWidgetItem *, currentItem), Q_ARG(QString, groupName));
				}

				if (hasOneEntry)
				{
					if (entry.name.isEmpty())
					{
						if (entryName.isEmpty())
							entry.name = Functions::fileName(url, false);
						else
							entry.name = entryName;
					}
					currentItem = pLW.newEntry(entry, currentItem, demuxersInfo, insertChildAt, existingEntries);
					if (!firstItem)
						firstItem = currentItem;
				}

				if (hasOneEntry || tracksAdded)
				{
					if (pLW.currPthToSave.isNull() && QFileInfo(dUrl).isFile())
						pLW.currPthToSave = Functions::filePath(dUrl);
					added = true;
				}

				pLW.dontUpdateAfterAdd = false;
			}
		}
	}
	return added;
}
QTreeWidgetItem *AddThr::insertPlaylistEntries(const Playlist::Entries &entries, QTreeWidgetItem *parent, const Functions::DemuxersInfo &demuxersInfo, int insertChildAt, QStringList *existingEntries)
{
	QList<QTreeWidgetItem *> groupList;
	const int queueSize = pLW.queue.size();
	QTreeWidgetItem *firstItem = nullptr;
	for (const Playlist::Entry &entry : entries)
	{
		QTreeWidgetItem *currentItem = nullptr, *tmpParent = nullptr, *createdItem = nullptr;
		const int idx = entry.parent - 1;
		if (idx >= 0 && groupList.size() > idx)
			tmpParent = groupList.at(idx);
		else
			tmpParent = parent;
		if (entry.GID)
		{
			if (sync != FILE_SYNC)
			{
				createdItem = pLW.newGroup(entry.name, entry.url, tmpParent, insertChildAt, existingEntries);
				groupList += createdItem;
			}
			else
			{
				 //Reuse current file group
				QMetaObject::invokeMethod(this, "changeItemText0", Q_ARG(QTreeWidgetItem *, tmpParent), Q_ARG(QString, entry.name));
				groupList += tmpParent;
			}
		}
		else
		{
			currentItem = createdItem = pLW.newEntry(entry, tmpParent, demuxersInfo, insertChildAt, existingEntries);
			if (entry.queue) //Rebuild queue
			{
				for (int j = pLW.queue.size(); j <= queueSize + entry.queue - 1; ++j)
					pLW.queue += nullptr;
				pLW.queue[queueSize + entry.queue - 1] = currentItem;
			}
			if (!firstItem)
				firstItem = currentItem;
		}

		if (createdItem)
		{
			const int entryAdditionalFlags = (entry.flags &~ Playlist::Entry::Selected);
			if (entryAdditionalFlags)
				QMetaObject::invokeMethod(this, "modifyItemFlags", Q_ARG(QTreeWidgetItem *, createdItem), Q_ARG(int, entryAdditionalFlags));
		}

		if (entry.flags & Playlist::Entry::Selected)
			firstItem = entry.GID ? groupList.last() : currentItem;
	}
	return firstItem;
}

void AddThr::finished()
{
	if (pLW.addTimer.isActive())
		return; //Don't finish, because this thread will be started soon again
	if (!pLW.currPthToSave.isNull())
	{
		QMPlay2GUI.setCurrentPth(pLW.currPthToSave);
		pLW.currPthToSave.clear();
	}
	pLW.animationTimer.stop();
	pLW.viewport()->update();
	pLW.setAnimated(false);
	pLW.refresh();
	pLW.setItemsResizeToContents(true);
	if (firstItem && ((pLW.selectAfterAdd && pLW.currentItem() != firstItem && pLW.currentItem() == lastItem) || !pLW.currentItem()))
		pLW.setCurrentItem(firstItem);
	pLW.selectAfterAdd = false;
	pLW.setAnimated(true);
	pLW.processItems();
	emit pLW.returnItem(firstItem);
	urls.clear();
	existingEntries.clear();
	playlistMenu()->stopLoading->setVisible(false);
	if (!pLW.currentPlaying && !pLW.currentPlayingUrl.isEmpty())
	{
		for (QTreeWidgetItem *tWI : pLW.findItems(QString(), Qt::MatchRecursive | Qt::MatchContains))
			if (pLW.getUrl(tWI) == pLW.currentPlayingUrl)
			{
				pLW.setCurrentPlaying(tWI);
				break;
			}
	}
	emit QMPlay2Core.restoreCursor();
	emit status(false);
	inProgress = false;
}

/*PlaylistWidget class*/
PlaylistWidget::PlaylistWidget() :
	addThr(*this),
	updateEntryThr(*this),
	repaintAll(false)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setDragDropMode(QAbstractItemView::InternalMove);
	setAlternatingRowColors(true);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setIndentation(12);
	setColumnCount(3);
	setAnimated(true);
	header()->setStretchLastSection(false);
	setHeaderHidden(true);
	header()->setSectionResizeMode(0, QHeaderView::Stretch);
	header()->hideSection(1);
	setItemsResizeToContents(true);
	setIconSize({22, 22});

	currentPlaying = nullptr;
	selectAfterAdd = hasHiddenItems = dontUpdateAfterAdd = false;

	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupContextMenu(const QPoint &)));
	connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(modifyMenu()));
	connect(&animationTimer, SIGNAL(timeout()), this, SLOT(animationUpdate()));
	connect(&addTimer, SIGNAL(timeout()), this, SLOT(addTimerElapsed()));
	connect(&addThr, SIGNAL(status(bool)), this, SIGNAL(addStatus(bool)));
	connect(playlistMenu(), &MenuBar::Playlist::aboutToShow, this, &PlaylistWidget::createExtensionsMenu);
}

QString PlaylistWidget::getUrl(QTreeWidgetItem *tWI) const
{
	if (!tWI)
		tWI = currentItem();
	return tWI ? tWI->data(0, Qt::UserRole).toString() : QString();
}

void PlaylistWidget::setItemsResizeToContents(bool b)
{
	const QHeaderView::ResizeMode rm = b ? QHeaderView::ResizeToContents : QHeaderView::Fixed;
	for (int i = 1; i <= 2; ++i)
		header()->setSectionResizeMode(i, rm);
}

void PlaylistWidget::sortCurrentGroup(int column, Qt::SortOrder sortOrder)
{
	QTreeWidgetItem *item = selectedItems().isEmpty() ? nullptr : currentItem();
	while (item && !isGroup(item))
		item = item->parent();
	if (item)
		item->sortChildren(column, sortOrder);
	else
		sortItems(column, sortOrder);
}

bool PlaylistWidget::add(const QStringList &urls, QTreeWidgetItem *par, const QStringList &existingEntries, bool loadList, bool forceEnqueue)
{
	if (urls.isEmpty())
		return false;
	if (!forceEnqueue && enqueuedAddData.isEmpty() && canModify())
		addThr.setData(urls, existingEntries, par, loadList);
	else
	{
		enqueuedAddData += {urls, existingEntries, par, loadList};
		if (!addTimer.isActive())
			addTimer.start(1);
	}
	return true;
}
bool PlaylistWidget::add(const QStringList &urls, bool atEndOfList)
{
	selectAfterAdd = true;
	return add(urls, atEndOfList ? nullptr : (selectedItems().isEmpty() ? nullptr : currentItem()));
}
void PlaylistWidget::sync(const QString &pth, QTreeWidgetItem *par, bool notDir)
{
	if (canModify())
		addThr.setDataForSync(pth, par, notDir);
}
void PlaylistWidget::quickSync(const QString &pth, QTreeWidgetItem *par, bool recursive)
{
	if (canModify())
	{
		bool mustRefresh = false;
		quickSyncScanDirs(pth, par, mustRefresh, recursive);
		if (mustRefresh && canModify())
		{
			refresh();
			processItems();
		}
	}
}

void PlaylistWidget::setCurrentPlaying(QTreeWidgetItem *tWI)
{
	if (!tWI)
		return;
	currentPlaying = tWI;
	/* Ikona */
	currentPlayingItemIcon = currentPlaying->data(0, Qt::DecorationRole);
	QMPlay2GUI.setTreeWidgetItemIcon(currentPlaying, QMPlay2Core.getIconFromTheme("media-playback-start"), 0, this);
}

void PlaylistWidget::clear()
{
	if (!canModify())
		return;
	queue.clear();
	clearCurrentPlaying();
	QTreeWidget::clear();
}
void PlaylistWidget::clearCurrentPlaying(bool url)
{
	currentPlaying = nullptr;
	if (url)
		currentPlayingUrl.clear();
	currentPlayingItemIcon.clear();
	modifyMenu();
}

QList<QTreeWidgetItem *> PlaylistWidget::topLevelNonGroupsItems() const
{
	QList<QTreeWidgetItem *> items;
	int count = topLevelItemCount();
	for (int i = 0; i < count; i++)
	{
		QTreeWidgetItem *tWI = topLevelItem(i);
		if (!tWI->parent() && !isGroup(tWI))
			items += tWI;
	}
	return items;
}
QList<QUrl> PlaylistWidget::getUrls() const
{
	QList<QUrl> urls;

	QStringList protocolsToAvoid;
	for (Module *module : QMPlay2Core.getPluginsInstance())
		for (const Module::Info &mod : module->getModulesInfo())
			if (mod.type == Module::DEMUXER && !mod.name.contains(' '))
				protocolsToAvoid += mod.name;

	for (QTreeWidgetItem *tWI : selectedItems())
	{
		QString url = getUrl(tWI);
		if (!url.isEmpty() && !url.contains("://{"))
		{
			const int idx = url.indexOf(':');
			if (idx > -1 && !protocolsToAvoid.contains(url.left(idx)))
			{
#ifdef Q_OS_WIN
				if (url.mid(4, 5) != ":////")
					url.replace("file://", "file:///");
#endif
				urls += url;
			}
		}
	}

	return urls;
}

QTreeWidgetItem *PlaylistWidget::newGroup()
{
	QTreeWidgetItem *parent = selectedItems().isEmpty() ? nullptr : currentItem();
	return newGroup(QString(), QString(), parent, isGroup(parent) ? 0 : -1, nullptr);
}

QList <QTreeWidgetItem * > PlaylistWidget::getChildren(CHILDREN children, const QTreeWidgetItem *parent) const
{
	QList<QTreeWidgetItem *> list;
	if (!parent)
		parent = invisibleRootItem();
	const int count = parent->childCount();
	for (int i = 0; i < count; i++)
	{
		QTreeWidgetItem *tWI = parent->child(i);
		const bool group = isGroup(tWI);
		if
		(
			((children & ONLY_NON_GROUPS) && !group) ||
			((children & ONLY_GROUPS)     &&  group)
		) list += tWI;
		if (group)
			list += getChildren(children, tWI);
	}
	return list;
}

bool PlaylistWidget::canModify(bool all) const
{
	if (addThr.isInProgress())
		return false;
	if (all)
	{
		if (updateEntryThr.isRunning() || updateEntryThr.hasPendingUpdates())
			return false;
		switch (QAbstractItemView::state())
		{
			case QAbstractItemView::NoState:
			case QAbstractItemView::AnimatingState:
				return true;
			default:
				return false;
		}
	}
	return true;
}

void PlaylistWidget::enqueue()
{
	for (QTreeWidgetItem *tWI : selectedItems())
	{
		if (isGroup(tWI))
			continue;
		if (queue.contains(tWI))
		{
			tWI->setText(1, QString());
			queue.removeOne(tWI);
		}
		else
			queue += tWI;
	}
	refresh(REFRESH_QUEUE);
}
void PlaylistWidget::refresh(REFRESH Refresh)
{
	const QList<QTreeWidgetItem *> items = getChildren(ONLY_NON_GROUPS);
	if (Refresh & REFRESH_QUEUE)
	{
		for (int i = 0; i < queue.size(); i++)
		{
			if (!items.contains(queue.at(i)))
				queue.removeAt(i--);
			else
				queue.at(i)->setText(1, QString::number(i + 1));
		}
		queue.size() ? header()->showSection(1) : header()->hideSection(1);
	}
	if (Refresh & REFRESH_GROUPS_TIME)
	{
		const QList<QTreeWidgetItem *> groups = getChildren(ONLY_GROUPS);
		for (int i = groups.size() - 1; i >= 0; i--)
		{
			double length = 0.0;
			for (QTreeWidgetItem *tWI : getChildren(ALL_CHILDREN, groups.at(i)))
			{
				if (tWI->parent() != groups.at(i))
					continue;
				const double l = tWI->data(2, Qt::UserRole).toDouble();
				if (l > 0.0)
					length += l;
			}
			const bool hasLength = !qFuzzyIsNull(length);
			groups.at(i)->setText(2, hasLength ? Functions::timeToStr(length) : QString());
			groups.at(i)->setData(2, Qt::UserRole, hasLength ? length : QVariant());
		}
	}
	if ((Refresh & REFRESH_CURRPLAYING) && !items.contains(currentPlaying))
		clearCurrentPlaying(false);
}

void PlaylistWidget::processItems(QList<QTreeWidgetItem *> *itemsToShow, bool hideGroups)
{
	int count = 0;
	hasHiddenItems = false;
	for (QTreeWidgetItem *tWI : getChildren(PlaylistWidget::ONLY_NON_GROUPS))
	{
		if (itemsToShow)
		{
			const int idx = itemsToShow->indexOf(tWI);
			tWI->setHidden(idx < 0);
			if (idx > -1)
				itemsToShow->removeAt(idx);
		}
		if (tWI->isHidden())
			hasHiddenItems = true;
		else
			count++;
	}
	emit visibleItemsCount(count);

	if (!itemsToShow)
		return;

	// Hide empty groups which doesn't exist in "itemsToShow" list
	const QList<QTreeWidgetItem *> groups = getChildren(ONLY_GROUPS);
	for (int i = groups.size() - 1; i >= 0; i--)
	{
		QTreeWidgetItem *group = groups.at(i);
		if (hideGroups && !itemsToShow->contains(group))
		{
			bool hasVisibleItems = false;
			for (QTreeWidgetItem *tWI : getChildren(ALL_CHILDREN, group))
			{
				if (!tWI->isHidden())
				{
					hasVisibleItems = true;
					break;
				}
			}
			group->setHidden(!hasVisibleItems);
		}
		else
		{
			group->setHidden(false);
		}
	}
}

bool PlaylistWidget::isAlwaysSynced(QTreeWidgetItem *tWI, bool parentOnly)
{
	if (QTreeWidgetItem *item = ((parentOnly && tWI) ? tWI->parent() : tWI))
	{
		do
		{
			if (PlaylistWidget::getFlags(item) & Playlist::Entry::AlwaysSync)
				return true;
		} while ((item = item->parent()));
	}
	return false;
}

void PlaylistWidget::setEntryFont(QTreeWidgetItem *tWI, const int flags)
{
	QFont font = tWI->font(0);
	font.setBold(flags & Playlist::Entry::Locked);
	font.setStrikeOut(flags & Playlist::Entry::Skip);
	font.setItalic(flags & Playlist::Entry::StopAfter);
	tWI->setFont(0, font);
}

QTreeWidgetItem *PlaylistWidget::newGroup(const QString &name, const QString &url, QTreeWidgetItem *parent, int insertChildAt, QStringList *existingEntries)
{
	QTreeWidgetItem *tWI = new QTreeWidgetItem;

	tWI->setFlags(tWI->flags() | Qt::ItemIsEditable);
	QMPlay2GUI.setTreeWidgetItemIcon(tWI, url.isEmpty() ? *QMPlay2GUI.groupIcon : *QMPlay2GUI.folderIcon, 0, this);
	tWI->setText(0, name);
	tWI->setData(0, Qt::UserRole, url);

	if (existingEntries)
		entryCreated(url, insertChildAt, *existingEntries);

	QMetaObject::invokeMethod(this, "insertItem", Q_ARG(QTreeWidgetItem *, tWI), Q_ARG(QTreeWidgetItem *, parent), Q_ARG(int, insertChildAt));
	return tWI;
}
QTreeWidgetItem *PlaylistWidget::newEntry(const Playlist::Entry &entry, QTreeWidgetItem *parent, const Functions::DemuxersInfo &demuxersInfo, int insertChildAt, QStringList *existingEntries)
{
	QTreeWidgetItem *tWI = new QTreeWidgetItem;

	QIcon icon;
	Functions::getDataIfHasPluginPrefix(entry.url, nullptr, nullptr, &icon, nullptr, demuxersInfo);
	setEntryIcon(icon, tWI);

	tWI->setFlags(tWI->flags() &~ Qt::ItemIsDropEnabled);
	tWI->setText(0, entry.name);
	tWI->setData(0, Qt::UserRole, entry.url);
	tWI->setText(2, Functions::timeToStr(entry.length));
	tWI->setData(2, Qt::UserRole, entry.length);

	if (existingEntries)
		entryCreated(entry.url, insertChildAt, *existingEntries);

	QMetaObject::invokeMethod(this, "insertItem", Q_ARG(QTreeWidgetItem *, tWI), Q_ARG(QTreeWidgetItem *, parent), Q_ARG(int, insertChildAt));
	return tWI;
}

void PlaylistWidget::setEntryIcon(const QIcon &icon, QTreeWidgetItem *tWI)
{
	if (icon.isNull())
	{
		if (tWI == currentPlaying)
			currentPlayingItemIcon = *QMPlay2GUI.mediaIcon;
		else
			QMPlay2GUI.setTreeWidgetItemIcon(tWI, *QMPlay2GUI.mediaIcon, 0, this);
	}
	else
	{
		QMetaObject::invokeMethod(this, "setItemIcon", Q_ARG(QTreeWidgetItem *, (tWI == currentPlaying) ? nullptr : tWI), Q_ARG(QIcon, icon));
	}
}

void PlaylistWidget::quickSyncScanDirs(const QString &pth, QTreeWidgetItem *par, bool &mustRefresh, bool recursive)
{
	QStringList dirEntries = getDirEntries(pth);
	QStringList existingEntries;

	for (int i = par->childCount() - 1; i >= 0; --i)
	{
		QTreeWidgetItem *item = par->child(i);

		const QString itemFileName = Functions::fileName(item->data(0, Qt::UserRole).toString());
		const bool isGroup = PlaylistWidget::isGroup(item);
		const int urlIdx = dirEntries.indexOf(itemFileName);
		QString fullPth = urlIdx > -1 ? pth + dirEntries.at(urlIdx) : QString();

		if (urlIdx > -1 && isGroup == QFileInfo(fullPth).isDir())
		{
			existingEntries.prepend(itemFileName);
			dirEntries.removeAt(urlIdx);
			if (isGroup && recursive)
			{
				if (!fullPth.endsWith('/'))
					fullPth.append('/');
				quickSyncScanDirs(fullPth, item, mustRefresh, recursive);
			}
		}
		else
		{
			mustRefresh = true;
			delete item;
		}
	}

	if (!dirEntries.isEmpty())
	{
		prependPath(dirEntries, pth);
		add(dirEntries, par, existingEntries, false, true);
		mustRefresh = false;
	}
}

void PlaylistWidget::createExtensionsMenu()
{
	QTreeWidgetItem *currItem = currentItem();
	const QString entryUrl = getUrl();
	if (!currItem || entryUrl.isEmpty())
		return;

	QMenu *extensions = playlistMenu()->extensions;
	extensions->clear();

	const QString entryName = currItem->text(0);
	const double entryLength = currItem->data(2, Qt::UserRole).toDouble();

	QString addressPrefixName, url, param;
	const bool splitFlag = Functions::splitPrefixAndUrlIfHasPluginPrefix(entryUrl, &addressPrefixName, &url, &param);
	for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
	{
		QVector<QAction *> actions;
		if (splitFlag)
			actions = QMPlay2Ext->getActions(entryName, entryLength, url, addressPrefixName, param);
		else
			actions = QMPlay2Ext->getActions(entryName, entryLength, entryUrl);
		for (QAction *act : asConst(actions))
		{
			act->setParent(extensions);
			extensions->addAction(act);
		}
	}

	extensions->setEnabled(!extensions->isEmpty());
}

void PlaylistWidget::mouseMoveEvent(QMouseEvent *e)
{
	const bool modifier = (e->modifiers() == Qt::MetaModifier) || (e->modifiers() == Qt::AltModifier);
	if ((e->buttons() & Qt::MidButton) || ((e->buttons() & Qt::LeftButton) && modifier))
	{
		const QList<QUrl> urls = getUrls();
		if (!urls.isEmpty())
		{
			QMimeData *mimeData = new QMimeData;
			mimeData->setUrls(urls);

			QDrag *drag = new QDrag(this);
			drag->setMimeData(mimeData);

			QPixmap pix;
			QTreeWidgetItem *currI = currentItem();
			if (currI && currI != currentPlaying)
				pix = Functions::getPixmapFromIcon(currI->icon(0), iconSize(), this);
			if (pix.isNull())
				pix = Functions::getPixmapFromIcon(*QMPlay2GUI.mediaIcon, iconSize(), this);
			drag->setPixmap(pix);

			drag->exec(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction, Qt::CopyAction);
		}
	}
	else if (canModify(false) && !hasHiddenItems)
	{
		internalDrag = false;
		QTreeWidget::mouseMoveEvent(e);
		if (internalDrag)
			refresh();
	}
}
void PlaylistWidget::dragEnterEvent(QDragEnterEvent *e)
{
	if (addThr.isRunning() || !e)
		return;
	if (Functions::chkMimeData(e->mimeData()))
		e->accept();
	else
	{
		QTreeWidget::dragEnterEvent(e);
		internalDrag = true;
	}
}
void PlaylistWidget::dragMoveEvent(QDragMoveEvent *e)
{
	if (!e)
		return;
	if (e->source() == this)
		QTreeWidget::dragMoveEvent(e);
	else
		setCurrentItem(itemAt(e->pos()));
}
void PlaylistWidget::dropEvent(QDropEvent *e)
{
	if (!e)
		return;
	if (e->source() == this)
	{
		QTreeWidgetItem *cI = currentItem();
		setItemsResizeToContents(false);
		QTreeWidget::dropEvent(e);
		setItemsResizeToContents(true);
		setCurrentItem(cI);
	}
	else if (e->mimeData())
	{
		selectAfterAdd = true;
		add(Functions::getUrlsFromMimeData(e->mimeData()), itemAt(e->pos()));
		e->accept();
	}
}
void PlaylistWidget::paintEvent(QPaintEvent *e)
{
	QTreeWidget::paintEvent(e);
	if (!animationTimer.isActive())
		return;

	QPainter p(viewport());
	p.setRenderHint(QPainter::Antialiasing);

	QRect arcRect = getArcRect(48);
	p.drawArc(arcRect, rotation * 16, 120 * 16);
	p.drawArc(arcRect, (rotation + 180) * 16, 120 * 16);
}
void PlaylistWidget::scrollContentsBy(int dx, int dy)
{
	if (dx || dy)
		repaintAll = true;
	QAbstractScrollArea::scrollContentsBy(dx, dy);
}

QRect PlaylistWidget::getArcRect(int size)
{
	return QRect(QPoint(width() / 2 - size / 2, height() / 2 - size / 2), QSize(size, size));
}

void PlaylistWidget::insertItem(QTreeWidgetItem *tWI, QTreeWidgetItem *parent, int insertChildAt)
{
	QTreeWidgetItem *childItem = parent;
	if (parent && !isGroup(parent))
		parent = parent->parent();
	if (insertChildAt < 0 && childItem && childItem->parent() == parent)
	{
		if (parent)
			parent->insertChild(parent->indexOfChild(childItem) + 1, tWI);
		else
			insertTopLevelItem(indexOfTopLevelItem(childItem) + 1, tWI);
	}
	else if (parent)
	{
		if (insertChildAt < 0)
			parent->addChild(tWI);
		else
			parent->insertChild(insertChildAt, tWI);
	}
	else
	{
		addTopLevelItem(tWI);
	}
}
void PlaylistWidget::popupContextMenu(const QPoint &p)
{
	playlistMenu()->popup(mapToGlobal(p));
}
void PlaylistWidget::setItemIcon(QTreeWidgetItem *tWI, const QIcon &icon)
{
	if (tWI)
		QMPlay2GUI.setTreeWidgetItemIcon(tWI, icon, 0, this);
	else
		currentPlayingItemIcon = icon;
}
void PlaylistWidget::animationUpdate()
{
	rotation -= 6;
	if (!repaintAll)
		viewport()->update(getArcRect(50));
	else
	{
		viewport()->update();
		repaintAll = false;
	}
}
void PlaylistWidget::addTimerElapsed()
{
	if (canModify() && !enqueuedAddData.isEmpty())
	{
		const AddData addData = enqueuedAddData.dequeue();
		addThr.setData(addData.urls, addData.existingEntries, addData.par, addData.loadList);
	}
	if (enqueuedAddData.isEmpty())
		addTimer.stop();
}

void PlaylistWidget::modifyMenu()
{
	const QString entryUrl = getUrl();

	QTreeWidgetItem *currItem = currentItem();

	const bool isLocked = (getFlags(currItem) & Playlist::Entry::Locked);
	const bool isItemGroup = isGroup(currItem);
	const bool syncVisible = (isItemGroup && !entryUrl.isEmpty());

	playlistMenu()->saveGroup->setVisible(isItemGroup);
	playlistMenu()->lock->setText(isLocked ? tr("Un&lock") : tr("&Lock"));
	playlistMenu()->lock->setVisible(currItem);
	playlistMenu()->alwaysSync->setChecked(isItemGroup && isAlwaysSynced(currItem));
	playlistMenu()->alwaysSync->setVisible(isItemGroup);
	playlistMenu()->alwaysSync->setEnabled(isItemGroup && !isAlwaysSynced(currItem, true));
	playlistMenu()->sync->setVisible(syncVisible);
	playlistMenu()->quickSync->setVisible(syncVisible && QFileInfo(QString(entryUrl).remove("file://")).isDir());
	playlistMenu()->renameGroup->setVisible(isItemGroup);
	playlistMenu()->entryProperties->setVisible(currItem);
	playlistMenu()->queue->setVisible(currItem && !isItemGroup);
	playlistMenu()->skip->setVisible(currItem && !isItemGroup);
	playlistMenu()->stopAfter->setVisible(currItem && !isItemGroup);
	playlistMenu()->goToPlayback->setVisible(currentPlaying);
	playlistMenu()->copy->setVisible(selectedItems().count());
}
