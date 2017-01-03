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

static inline MenuBar::Playlist *playlistMenu()
{
	return QMPlay2GUI.menuBar->playlist;
}

static const int IconSize = 22;
static const int IconSizeDiv2 = IconSize / 2;

/* UpdateEntryThr class */
UpdateEntryThr::UpdateEntryThr(PlaylistWidget &pLW) :
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
	itemsToUpdate += (ItemToUpdate){item, pLW.getUrl(item), item->data(2, Qt::UserRole).toDouble(), name, length};
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
			Functions::getDataIfHasPluginPrefix(url, &url, &itu.name, &iu.img, &ioCtrl);
			iu.updateImg = true;

			IOController<Demuxer> &demuxer = ioCtrl.toRef<Demuxer>();
			if (Demuxer::create(url, demuxer))
			{
				if (!displayOnlyFileName && itu.name.isEmpty())
					itu.name = demuxer->title();
				itu.length = demuxer->length();
				demuxer.clear();
			}
			else
				updateTitle = false;
		}
		else
			iu.updateImg = false;

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
		ioCtrl.clear();
	}
}

void UpdateEntryThr::updateItem(ItemUpdated iu)
{
	if (iu.updateImg)
		pLW.setEntryIcon(iu.img, iu.item);
	if (!iu.name.isNull())
		iu.item->setText(0, iu.name);
	if (iu.updateLength)
	{
		iu.item->setText(2, Functions::timeToStr(iu.length));
		iu.item->setData(2, Qt::UserRole, iu.length);
	}
	if (iu.item == pLW.currentPlaying)
		QMetaObject::invokeMethod(QMPlay2GUI.mainW, "updateWindowTitle", Q_ARG(QString, iu.item->text(0)));
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
	pLW(pLW)
{
	connect(this, SIGNAL(finished()), this, SLOT(finished()));
}

void AddThr::setData(const QStringList &_urls, QTreeWidgetItem *_par, bool _loadList, SYNC _sync)
{
	ioCtrl.resetAbort();

	urls = _urls;
	par = _par;
	loadList = _loadList;
	sync = _sync;

	firstItem = NULL;
	lastItem = pLW.currentItem();

	pLW.setItemsResizeToContents(false);

	emit pLW.visibleItemsCount(-1);

	if (sync)
	{
		while (par->childCount())
			delete par->child(0);
		pLW.refresh(PlaylistWidget::REFRESH_CURRPLAYING);
	}

	emit QMPlay2Core.busyCursor();
	if (!loadList)
	{
		pLW.rotation = 0;
		pLW.animationTimer.start(50);
		playlistMenu()->stopLoading->setVisible(true);
		start();
	}
	else
		run();
}
void AddThr::setDataForSync(const QString &pth, QTreeWidgetItem *par, bool notDir)
{
	if (notDir)
		setData(QStringList() << pth, par, false, FILE_SYNC); //File synchronization needs only one file!
	else
	{
		QStringList d_urls = QDir(pth).entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::DirsFirst);
		for (int i = d_urls.size() - 1; i >= 0; --i)
			d_urls[i] = pth + d_urls[i];
		setData(d_urls, par, false, DIR_SYNC);
	}
}

void AddThr::stop()
{
	ioCtrl.abort();
	wait(TERMINATE_TIMEOUT);
	if (isRunning())
	{
		terminate();
		wait(1000);
		ioCtrl.clear();
	}
}

void AddThr::changeItemText0(QTreeWidgetItem *tWI, QString name)
{
	if (!tWI->data(0, Qt::UserRole + 1).toBool()) //It can be set in "EntryProperties::accept()"
		tWI->setText(0, name);
	tWI->setData(0, Qt::UserRole + 1, QVariant());
}
void AddThr::deleteTreeWidgetItem(QTreeWidgetItem *tWI)
{
	delete tWI;
}

void AddThr::run()
{
	Functions::DemuxersInfo demuxersInfo;
	foreach (Module *module, QMPlay2Core.getPluginsInstance())
		foreach (const Module::Info &mod, module->getModulesInfo())
			if (mod.type == Module::DEMUXER)
				demuxersInfo += (Functions::DemuxerInfo){mod.name, mod.img.isNull() ? module->image() : mod.img, mod.extensions};
	add(urls, par, demuxersInfo, loadList);
	if (currentThread() == pLW.thread()) //jeżeli funkcja działa w głównym wątku
		finished();
}

bool AddThr::add(const QStringList &urls, QTreeWidgetItem *parent, const Functions::DemuxersInfo &demuxersInfo, bool loadList)
{
	const bool displayOnlyFileName = QMPlay2Core.getSettings().getBool("DisplayOnlyFileName");
	QTreeWidgetItem *currentItem = parent;
	bool added = false;
	for (int i = 0; i < urls.size(); ++i)
	{
		if (ioCtrl.isAborted())
			break;

		QString url, entryName;
		{
			//Get the default entry name - it'll be used if not exists in stream
			QString addressPrefixName;
			Functions::splitPrefixAndUrlIfHasPluginPrefix(urls.at(i), &addressPrefixName, &url, &entryName);
			if (addressPrefixName != "QMPlay2EntryName")
			{
				url = Functions::Url(urls.at(i));
				entryName.clear();
			}
		}

		QString name;
		const Playlist::Entries entries = Playlist::read(url, &name);
		if (!name.isEmpty()) //Loading playlist
		{
			if (loadList) //Loading QMPlay2 playlist on startup
				pLW.clear(); //This can be executed only from GUI thread!
			else
			{
				const QString groupName = Functions::fileName(url, false);
				if (sync != FILE_SYNC)
					currentItem =  pLW.newGroup(groupName, url, currentItem); //Adding a new playlist group
				else
				{
					//Reuse current playlist group
					QMetaObject::invokeMethod(this, "changeItemText0", Q_ARG(QTreeWidgetItem *, parent), Q_ARG(QString, groupName));
					currentItem = parent;
					sync = NO_SYNC;
				}
			}
			QTreeWidgetItem *tmpFirstItem = insertPlaylistEntries(entries, currentItem, demuxersInfo);
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
				QStringList d_urls = QDir(dUrl).entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::DirsFirst);
				if (!d_urls.isEmpty())
				{
					url = Functions::cleanPath(url);
					QTreeWidgetItem *p = pLW.newGroup(Functions::fileName(url), url, currentItem);
					for (int j = d_urls.size() - 1; j >= 0; j--)
					{
						d_urls[j].prepend(url);
#ifdef Q_OS_WIN
						d_urls[j].replace("file://", "file:///");
#endif
					}
					if (add(d_urls, p, demuxersInfo))
						added = true;
					else
						QMetaObject::invokeMethod(this, "deleteTreeWidgetItem", Q_ARG(QTreeWidgetItem *, p));
				}
			}
			else
			{
				bool hasOneEntry = pLW.dontUpdateAfterAdd;
				bool tracksAdded = false;

				Playlist::Entry entry;
				entry.url = url;

				if (!pLW.dontUpdateAfterAdd) //Don't try to get the real address from extension plugin in this case (no needed for tracks)
					Functions::getDataIfHasPluginPrefix(url, &url, &entry.name, NULL, &ioCtrl, demuxersInfo);
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
						QTreeWidgetItem *tmpFirstItem = insertPlaylistEntries(fetchTracks.tracks, currentItem, demuxersInfo);
						if (!firstItem)
							firstItem = tmpFirstItem;
						hasOneEntry = false;
						tracksAdded = true;
					}
					demuxer.clear();
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
					currentItem = pLW.newEntry(entry, currentItem, demuxersInfo);
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
QTreeWidgetItem *AddThr::insertPlaylistEntries(const Playlist::Entries &entries, QTreeWidgetItem *parent, const Functions::DemuxersInfo &demuxersInfo)
{
	QList<QTreeWidgetItem *> groupList;
	const int queueSize = pLW.queue.size();
	QTreeWidgetItem *firstItem = NULL;
	foreach (const Playlist::Entry &entry, entries)
	{
		QTreeWidgetItem *currentItem = NULL, *tmpParent = NULL;
		const int idx = entry.parent - 1;
		if (idx >= 0 && groupList.size() > idx)
			tmpParent = groupList.at(idx);
		else
			tmpParent = parent;
		if (entry.GID)
		{
			if (sync != FILE_SYNC)
				groupList += pLW.newGroup(entry.name, entry.url, tmpParent);
			else
			{
				 //Reuse current file group
				QMetaObject::invokeMethod(this, "changeItemText0", Q_ARG(QTreeWidgetItem *, tmpParent), Q_ARG(QString, entry.name));
				groupList += tmpParent;
			}
		}
		else
		{
			currentItem = pLW.newEntry(entry, tmpParent, demuxersInfo);
			if (entry.queue) //Rebuild queue
			{
				for (int j = pLW.queue.size(); j <= queueSize + entry.queue - 1; ++j)
					pLW.queue += NULL;
				pLW.queue[queueSize + entry.queue - 1] = currentItem;
			}
			if (!firstItem)
				firstItem = currentItem;
		}
		if (entry.selected)
			firstItem = entry.GID ? groupList.last() : currentItem;
	}
	return firstItem;
}

void AddThr::finished()
{
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
	playlistMenu()->stopLoading->setVisible(false);
	if (!pLW.currentPlaying && !pLW.currentPlayingUrl.isEmpty())
	{
		foreach (QTreeWidgetItem *tWI, pLW.findItems(QString(), Qt::MatchRecursive | Qt::MatchContains))
			if (pLW.getUrl(tWI) == pLW.currentPlayingUrl)
			{
				pLW.setCurrentPlaying(tWI);
				break;
			}
	}
	emit QMPlay2Core.restoreCursor();
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
#if QT_VERSION < 0x050000
	header()->setResizeMode(0, QHeaderView::Stretch);
#else
	header()->setSectionResizeMode(0, QHeaderView::Stretch);
#endif
	header()->hideSection(1);
	setItemsResizeToContents(true);
	setIconSize(QSize(IconSize, IconSize));

	currentPlaying = NULL;
	selectAfterAdd = hasHiddenItems = dontUpdateAfterAdd = false;

	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupContextMenu(const QPoint &)));
	connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(modifyMenu()));
	connect(&animationTimer, SIGNAL(timeout()), this, SLOT(animationUpdate()));
	connect(&addTimer, SIGNAL(timeout()), this, SLOT(addTimerElapsed()));
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
#if QT_VERSION < 0x050000
		header()->setResizeMode(i, rm);
#else
		header()->setSectionResizeMode(i, rm);
#endif
}

void PlaylistWidget::sortCurrentGroup(int column, Qt::SortOrder sortOrder)
{
	QTreeWidgetItem *item = selectedItems().isEmpty() ? NULL : currentItem();
	while (item && !isGroup(item))
		item = item->parent();
	if (item)
		item->sortChildren(column, sortOrder);
	else
		sortItems(column, sortOrder);
}

bool PlaylistWidget::add(const QStringList &urls, QTreeWidgetItem *par, bool loadList)
{
	if (urls.isEmpty())
		return false;
	if (canModify())
		addThr.setData(urls, par, loadList);
	else
	{
		enqueuedAddData += (AddData){urls, par, loadList};
		addTimer.start();
	}
	return true;
}
bool PlaylistWidget::add(const QStringList &urls, bool atEndOfList)
{
	selectAfterAdd = true;
	return add(urls, atEndOfList ? NULL : (!selectedItems().count() ? NULL : currentItem()), false);
}
void PlaylistWidget::sync(const QString &pth, QTreeWidgetItem *par, bool notDir)
{
	if (canModify())
		addThr.setDataForSync(pth, par, notDir);
}

void PlaylistWidget::setCurrentPlaying(QTreeWidgetItem *tWI)
{
	if (!tWI)
		return;
	currentPlaying = tWI;
	/* Ikona */
	currentPlayingItemIcon = currentPlaying->icon(0);
	QIcon play_icon = QMPlay2Core.getIconFromTheme("media-playback-start");
	if (!play_icon.availableSizes().isEmpty() && !play_icon.availableSizes().contains(QSize(IconSize, IconSize)))
	{
		const QPixmap playPix = play_icon.pixmap(play_icon.availableSizes().at(0));
		QPixmap pix(IconSize, IconSize);
		pix.fill(Qt::transparent);
		QPainter p(&pix);
		p.drawPixmap(IconSizeDiv2- playPix.width() / 2, IconSizeDiv2 - playPix.height() / 2, playPix);
		play_icon = QIcon(pix);
	}
	currentPlaying->setIcon(0, play_icon);
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
	currentPlaying = NULL;
	if (url)
		currentPlayingUrl.clear();
	currentPlayingItemIcon = QIcon();
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
	foreach (Module *module, QMPlay2Core.getPluginsInstance())
		foreach (const Module::Info &mod, module->getModulesInfo())
			if (mod.type == Module::DEMUXER && !mod.name.contains(' '))
				protocolsToAvoid += mod.name;

	foreach (QTreeWidgetItem *tWI, selectedItems())
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

QTreeWidgetItem *PlaylistWidget::newGroup(const QString &name, const QString &url, QTreeWidgetItem *parent, bool insertChildAt0Idx)
{
	QTreeWidgetItem *tWI = new QTreeWidgetItem;

	tWI->setFlags(tWI->flags() | Qt::ItemIsEditable);
	tWI->setIcon(0, url.isEmpty() ? *QMPlay2GUI.groupIcon : *QMPlay2GUI.folderIcon);
	tWI->setText(0, name);
	tWI->setData(0, Qt::UserRole, url);

	QMetaObject::invokeMethod(this, "insertItem", Q_ARG(QTreeWidgetItem *, tWI), Q_ARG(QTreeWidgetItem *, parent), Q_ARG(bool, insertChildAt0Idx));
	return tWI;
}
QTreeWidgetItem *PlaylistWidget::newGroup()
{
	return newGroup(QString(), QString(), !selectedItems().count() ? NULL : currentItem(), true);
}

QTreeWidgetItem *PlaylistWidget::newEntry(const Playlist::Entry &entry, QTreeWidgetItem *parent, const Functions::DemuxersInfo &demuxersInfo)
{
	QTreeWidgetItem *tWI = new QTreeWidgetItem;

	QImage img;
	Functions::getDataIfHasPluginPrefix(entry.url, NULL, NULL, &img, NULL, demuxersInfo);
	setEntryIcon(img, tWI);

	tWI->setFlags(tWI->flags() &~ Qt::ItemIsDropEnabled);
	tWI->setText(0, entry.name);
	tWI->setData(0, Qt::UserRole, entry.url);
	tWI->setText(2, Functions::timeToStr(entry.length));
	tWI->setData(2, Qt::UserRole, entry.length);

	QMetaObject::invokeMethod(this, "insertItem", Q_ARG(QTreeWidgetItem *, tWI), Q_ARG(QTreeWidgetItem *, parent), Q_ARG(bool, false));
	return tWI;
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
	if (addThr.isRunning())
		return false;
	if (all)
	{
		if (updateEntryThr.isRunning())
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
	foreach (QTreeWidgetItem *tWI, selectedItems())
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
			foreach (QTreeWidgetItem *tWI, getChildren(ALL_CHILDREN, groups.at(i)))
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
	foreach (QTreeWidgetItem *tWI, getChildren(PlaylistWidget::ONLY_NON_GROUPS, NULL))
	{
		if (itemsToShow)
			tWI->setHidden(!itemsToShow->contains(tWI));
		if (!tWI->isHidden())
			count++;
		else
			hasHiddenItems = true;
	}
	emit visibleItemsCount(count);

	if (!itemsToShow)
		return;

	//ukrywanie pustych grup
	const QList<QTreeWidgetItem *> groups = getChildren(ONLY_GROUPS);
	for (int i = groups.size() - 1; i >= 0; i--)
	{
		if (hideGroups)
		{
			bool hasVisibleItems = false;
			foreach (QTreeWidgetItem *tWI, getChildren(ONLY_NON_GROUPS, groups.at(i)))
			{
				if (!tWI->isHidden())
				{
					hasVisibleItems = true;
					break;
				}
			}
			groups.at(i)->setHidden(!hasVisibleItems);
		}
		else
			groups.at(i)->setHidden(false);
	}
}

void PlaylistWidget::setEntryIcon(const QImage &origImg, QTreeWidgetItem *tWI)
{
	QImage img = origImg;
	if (img.isNull())
	{
		if (tWI == currentPlaying)
			currentPlayingItemIcon = *QMPlay2GUI.mediaIcon;
		else
			tWI->setIcon(0, *QMPlay2GUI.mediaIcon);
	}
	else
	{
		if (img.height() == img.width() && img.height() > IconSize && img.width() > IconSize)
			img = img.scaled(IconSize, IconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		else if (img.height() > img.width() && img.height() > IconSize)
			img = img.scaledToHeight(IconSize, Qt::SmoothTransformation);
		else if (img.width() > img.height() && img.width() > IconSize)
			img = img.scaledToWidth(IconSize, Qt::SmoothTransformation);
		QMetaObject::invokeMethod(this, "setItemIcon", Q_ARG(QTreeWidgetItem *, (tWI == currentPlaying) ? NULL : tWI), Q_ARG(QImage, img));
	}
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
				pix = currI->icon(0).pixmap(IconSize, IconSize);
			if (pix.isNull())
				pix = QMPlay2Core.getIconFromTheme("applications-multimedia").pixmap(IconSize, IconSize);
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

void PlaylistWidget::insertItem(QTreeWidgetItem *tWI, QTreeWidgetItem *parent, bool insertChildAt0Idx)
{
	QTreeWidgetItem *cI = parent;
	if (parent && !isGroup(parent))
		parent = parent->parent();
	if (cI && cI->parent() == parent)
	{
		if (parent)
			parent->insertChild(parent->indexOfChild(cI) + 1, tWI);
		else
			insertTopLevelItem(indexOfTopLevelItem(cI) + 1, tWI);
	}
	else
	{
		if (parent)
		{
			if (!insertChildAt0Idx)
				parent->addChild(tWI);
			else
				parent->insertChild(0, tWI);
		}
		else
			addTopLevelItem(tWI);
	}
}
void PlaylistWidget::popupContextMenu(const QPoint &p)
{
	playlistMenu()->popup(mapToGlobal(p));
}
void PlaylistWidget::setItemIcon(QTreeWidgetItem *tWI, const QImage &img)
{
	if (img.width() == IconSize && img.height() == IconSize)
	{
		if (tWI)
			tWI->setIcon(0, QPixmap::fromImage(img));
		else
			currentPlayingItemIcon = QPixmap::fromImage(img);
	}
	else
	{
		QPixmap canvas(IconSize, IconSize);
		canvas.fill(QColor(0, 0, 0, 0));
		QPainter p(&canvas);
		p.drawImage(IconSizeDiv2 - img.width() / 2, IconSizeDiv2 - img.height() / 2, img);
		p.end();
		if (tWI)
			tWI->setIcon(0, canvas);
		else
			currentPlayingItemIcon = canvas;
	}
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
		AddData addData = enqueuedAddData.dequeue();
		addThr.setData(addData.urls, addData.par, addData.loadList);
	}
	if (enqueuedAddData.isEmpty())
		addTimer.stop();
}

void PlaylistWidget::modifyMenu()
{
	QString entryUrl = getUrl();
	QString entryName;
	double entryLength = -2.0;
	if (currentItem())
	{
		entryName = currentItem()->text(0);
		entryLength = currentItem()->data(2, Qt::UserRole).toDouble();
	}

	QTreeWidgetItem *currItem = currentItem();
	bool isItemGroup = isGroup(currItem);
	playlistMenu()->saveGroup->setVisible(currItem && isItemGroup);
	playlistMenu()->sync->setVisible(currItem && isItemGroup && !entryUrl.isEmpty());
	playlistMenu()->renameGroup->setVisible(currItem && isItemGroup);
	playlistMenu()->entryProperties->setVisible(currItem);
	playlistMenu()->queue->setVisible(currItem);
	playlistMenu()->goToPlayback->setVisible(currentPlaying);
	playlistMenu()->copy->setVisible(selectedItems().count());

	playlistMenu()->extensions->clear();
	QString addressPrefixName, url, param;
	bool splitFlag = Functions::splitPrefixAndUrlIfHasPluginPrefix(entryUrl, &addressPrefixName, &url, &param);
	foreach (QMPlay2Extensions *QMPlay2Ext, QMPlay2Extensions::QMPlay2ExtensionsList())
	{
		QAction *act;
		if (splitFlag)
			act = QMPlay2Ext->getAction(entryName, entryLength, url, addressPrefixName, param);
		else
			act = QMPlay2Ext->getAction(entryName, entryLength, entryUrl);
		if (act)
		{
			act->setParent(playlistMenu()->extensions);
			playlistMenu()->extensions->addAction(act);
		}
	}
	playlistMenu()->extensions->setEnabled(playlistMenu()->extensions->actions().count());
}
