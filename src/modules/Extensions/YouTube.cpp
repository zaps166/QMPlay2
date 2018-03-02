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

#include <YouTube.hpp>

#include <Functions.hpp>
#include <YouTubeDL.hpp>
#include <LineEdit.hpp>
#include <Playlist.hpp>

#include <QLoggingCategory>
#include <QStringListModel>
#include <QDesktopServices>
#include <QJsonParseError>
#include <QTextDocument>
#include <QJsonDocument>
#include <QProgressBar>
#include <QApplication>
#include <QJsonObject>
#include <QHeaderView>
#include <QGridLayout>
#include <QMouseEvent>
#include <QToolButton>
#include <QMessageBox>
#include <QJsonArray>
#include <QCompleter>
#include <QClipboard>
#include <QMimeData>
#include <QSpinBox>
#include <QProcess>
#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QDrag>
#include <QFile>
#include <QDir>
#include <QUrl>

Q_LOGGING_CATEGORY(youtube, "Extensions/YouTube")

#define YOUTUBE_URL "https://www.youtube.com"

constexpr const char *g_cantFindTheTitle = "(Can't find the title)";
static QMap<int, QString> g_itagArr;

static inline QString toPercentEncoding(const QString &txt)
{
	return txt.toUtf8().toPercentEncoding();
}

static inline QString getYtUrl(const QString &title, const int page, const int sortByIdx)
{
	static constexpr const char *sortBy[4] {
		"",             // Relevance ("&sp=CAA%253D")
		"&sp=CAI%253D", // Upload date
		"&sp=CAM%253D", // View count
		"&sp=CAE%253D", // Rating
	};
	Q_ASSERT(sortByIdx >= 0 && sortByIdx <= 3);
	return QString(YOUTUBE_URL "/results?search_query=%1%2&page=%3").arg(toPercentEncoding(title), sortBy[sortByIdx]).arg(page);
}
static inline QString getAutocompleteUrl(const QString &text)
{
	return QString("http://suggestqueries.google.com/complete/search?client=firefox&ds=yt&q=%1").arg(toPercentEncoding(text));
}
static inline QString getSubsUrl(const QString &langCode, const QString &vidId)
{
	return QString(YOUTUBE_URL "/api/timedtext?lang=%1&fmt=vtt&v=%2").arg(langCode, vidId);
}

static inline QString getFileExtension(const QString &ItagName)
{
	if (ItagName.contains("WebM") || ItagName.contains("VP9") || ItagName.contains("VP8") || ItagName.contains("Vorbis") || ItagName.contains("Opus"))
		return ".webm";
	else if (ItagName.contains("AAC") || ItagName.contains("H.264"))
		return ".mp4";
	else if (ItagName.contains("FLV"))
		return ".flv";
	return ".unknown";
}

static inline QString getQMPlay2Url(const QTreeWidgetItem *tWI)
{
	if (tWI->parent())
		return "YouTube://{" + tWI->parent()->data(0, Qt::UserRole).toString() + "}" + tWI->data(0, Qt::UserRole + 1).toString();
	return "YouTube://{" + tWI->data(0, Qt::UserRole).toString() + "}";
}

static inline QString unescape(const QString &str)
{
	return QByteArray::fromPercentEncoding(str.toLatin1());
}

static inline bool isPlaylist(QTreeWidgetItem *tWI)
{
	return tWI->data(1, Qt::UserRole).toBool();
}

/**/

ResultsYoutube::ResultsYoutube() :
	menu(new QMenu(this)),
	pixels(0)
{
	setAnimated(true);
	setIndentation(12);
	setIconSize({100, 100});
	setExpandsOnDoubleClick(false);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	headerItem()->setText(0, tr("Title"));
	headerItem()->setText(1, tr("Length"));
	headerItem()->setText(2, tr("User"));

	header()->setStretchLastSection(false);
	header()->setSectionResizeMode(0, QHeaderView::Stretch);
	header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

	connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(playEntry(QTreeWidgetItem *)));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenu(const QPoint &)));
	setContextMenuPolicy(Qt::CustomContextMenu);
}
ResultsYoutube::~ResultsYoutube()
{}

QTreeWidgetItem *ResultsYoutube::getDefaultQuality(const QTreeWidgetItem *tWI)
{
	if (!tWI->childCount())
		return nullptr;
	for (int itag : asConst(itags))
		for (int i = 0; i < tWI->childCount(); ++i)
			if (tWI->child(i)->data(0, Qt::UserRole + 2).toInt() == itag)
				return tWI->child(i);
	return tWI->child(0);
}

void ResultsYoutube::playOrEnqueue(const QString &param, QTreeWidgetItem *tWI)
{
	if (!tWI)
		return;
	if (!isPlaylist(tWI))
		emit QMPlay2Core.processParam(param, getQMPlay2Url(tWI));
	else
	{
		const QStringList ytPlaylist = tWI->data(0, Qt::UserRole + 1).toStringList();
		QMPlay2CoreClass::GroupEntries entries;
		for (int i = 0; i < ytPlaylist.count() ; i += 2)
			entries += {ytPlaylist[i+1], "YouTube://{" YOUTUBE_URL "/watch?v=" + ytPlaylist[i+0] + "}"};
		if (!entries.isEmpty())
		{
			const bool enqueue = (param == "enqueue");
			QMPlay2Core.loadPlaylistGroup(YouTubeName "/" + QString(tWI->text(0)).replace('/', '_'), entries, enqueue);
		}
	}
}

void ResultsYoutube::mouseMoveEvent(QMouseEvent *e)
{
	if (++pixels == 25)
	{
		QTreeWidgetItem *tWI = currentItem();
		if (tWI && !isPlaylist(tWI))
		{
			QString url;
			if (e->buttons() & Qt::LeftButton)
				url = getQMPlay2Url(tWI);
			else if (e->buttons() & Qt::MiddleButton) //Link do strumienia
			{
				QTreeWidgetItem *tWI2 = tWI->parent() ? tWI : getDefaultQuality(tWI);
				if (tWI2)
					url = tWI2->data(0, Qt::UserRole).toString();
			}
			if (!url.isEmpty())
			{
				QMimeData *mimeData = new QMimeData;
				if (e->buttons() & Qt::LeftButton)
					mimeData->setText(url);
				else if (e->buttons() & Qt::MiddleButton)
					mimeData->setUrls(QList<QUrl>() << url);

				if (tWI->parent())
					tWI = tWI->parent();

				QDrag *drag = new QDrag(this);
				drag->setMimeData(mimeData);
				drag->setPixmap(Functions::getPixmapFromIcon(tWI->icon(0), iconSize(), this));
				drag->exec(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction);
				pixels = 0;
				return;
			}
		}
	}
	QTreeWidget::mouseMoveEvent(e);
}
void ResultsYoutube::mouseReleaseEvent(QMouseEvent *e)
{
	pixels = 0;
	QTreeWidget::mouseReleaseEvent(e);
}

void ResultsYoutube::enqueue()
{
	playOrEnqueue("enqueue", currentItem());
}
void ResultsYoutube::playCurrentEntry()
{
	playOrEnqueue("open", currentItem());
}
void ResultsYoutube::playEntry(QTreeWidgetItem *tWI)
{
	playOrEnqueue("open", tWI);
}

void ResultsYoutube::openPage()
{
	QTreeWidgetItem *tWI = currentItem();
	if (tWI)
	{
		if (tWI->parent())
			tWI = tWI->parent();
		QDesktopServices::openUrl(tWI->data(0, Qt::UserRole).toString());
	}
}
void ResultsYoutube::copyPageURL()
{
	QTreeWidgetItem *tWI = currentItem();
	if (tWI)
	{
		if (tWI->parent())
			tWI = tWI->parent();
		QMimeData *mimeData = new QMimeData;
		mimeData->setText(tWI->data(0, Qt::UserRole).toString());
		QApplication::clipboard()->setMimeData(mimeData);
	}
}
void ResultsYoutube::copyStreamURL()
{
	const QString streamUrl = sender()->property("StreamUrl").toString();
	if (!streamUrl.isEmpty())
	{
		QMimeData *mimeData = new QMimeData;
		mimeData->setText(streamUrl);
		QApplication::clipboard()->setMimeData(mimeData);
	}
}

void ResultsYoutube::contextMenu(const QPoint &point)
{
	menu->clear();
	QTreeWidgetItem *tWI = currentItem();
	if (tWI)
	{
		const bool isOK = !tWI->isDisabled();
		if (isOK)
		{
			menu->addAction(tr("Enqueue"), this, SLOT(enqueue()));
			menu->addAction(tr("Play"), this, SLOT(playCurrentEntry()));
			menu->addSeparator();
		}
		menu->addAction(tr("Open the page in the browser"), this, SLOT(openPage()));
		menu->addAction(tr("Copy page address"), this, SLOT(copyPageURL()));
		menu->addSeparator();
		if (isOK && !isPlaylist(tWI))
		{
			QVariant streamUrl;
			QTreeWidgetItem *tWI_2 = tWI;
			if (!tWI_2->parent())
				tWI_2 = getDefaultQuality(tWI_2);
			if (tWI_2)
				streamUrl = tWI_2->data(0, Qt::UserRole);

			if (!streamUrl.isNull())
			{
				menu->addAction(tr("Copy stream address"), this, SLOT(copyStreamURL()))->setProperty("StreamUrl", streamUrl);
				menu->addSeparator();
			}

			const QString name = tWI->parent() ? tWI->parent()->text(0) : tWI->text(0);
			for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
			{
				if (!dynamic_cast<YouTube *>(QMPlay2Ext))
				{
					QString addressPrefixName, url, param;
					if (Functions::splitPrefixAndUrlIfHasPluginPrefix(getQMPlay2Url(tWI), &addressPrefixName, &url, &param))
					{
						for (QAction *act : QMPlay2Ext->getActions(name, -2, url, addressPrefixName, param))
						{
							act->setParent(menu);
							menu->addAction(act);
						}
					}
				}
			}
		}
		menu->popup(viewport()->mapToGlobal(point));
	}
}

/**/

PageSwitcher::PageSwitcher(QWidget *youTubeW)
{
	prevB = new QToolButton;
	connect(prevB, SIGNAL(clicked()), youTubeW, SLOT(prev()));
	prevB->setAutoRaise(true);
	prevB->setArrowType(Qt::LeftArrow);

	currPageB = new QSpinBox;
	connect(currPageB, SIGNAL(editingFinished()), youTubeW, SLOT(chPage()));
	currPageB->setMinimum(1);
	currPageB->setMaximum(50); //1000 wyników, po 20 wyników na stronę

	nextB = new QToolButton;
	connect(nextB, SIGNAL(clicked()), youTubeW, SLOT(next()));
	nextB->setAutoRaise(true);
	nextB->setArrowType(Qt::RightArrow);

	QHBoxLayout *hLayout = new QHBoxLayout(this);
	hLayout->setContentsMargins(0, 0, 0, 0);
	hLayout->addWidget(prevB);
	hLayout->addWidget(currPageB);
	hLayout->addWidget(nextB);
}

/**/

QList<int> *YouTube::getQualityPresets()
{
	static QList<int> qualityPresets[QUALITY_PRESETS_COUNT];
	static bool firstTime = true;
	if (firstTime)
	{
		qualityPresets[_2160p60] << 315 << 299 << 303 << 298 << 302;
		qualityPresets[_1080p60] << 299 << 303 << 298 << 302;
		qualityPresets[_720p60] << 298 << 302;

		qualityPresets[_2160p] << 266 << 313 << 137 << 248 << 136 << 247 << 135;
		qualityPresets[_1080p] << 137 << 248 << 136 << 247 << 135;
		qualityPresets[_720p] << 136 << 247 << 135;
		qualityPresets[_480p] << 135;

		//Append also non-60 FPS itags to 60 FPS itags
		qualityPresets[_2160p60] += qualityPresets[_2160p];
		qualityPresets[_1080p60] += qualityPresets[_1080p];
		qualityPresets[_720p60] += qualityPresets[_720p];

		firstTime = false;
	}
	return qualityPresets;
}
QStringList YouTube::getQualityPresetString(int qualityIdx)
{
	QStringList videoItags;
	for (int itag : asConst(getQualityPresets()[qualityIdx]))
		videoItags.append(QString::number(itag));
	return videoItags;
}

YouTube::YouTube(Module &module) :
	completer(new QCompleter(new QStringListModel(this), this)),
	currPage(1),
	net(this)
{
	youtubeIcon = QIcon(":/youtube.svgz");
	videoIcon = QIcon(":/video.svgz");

	dw = new DockWidget;
	connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(setEnabled(bool)));
	dw->setWindowTitle("YouTube");
	dw->setObjectName(YouTubeName);
	dw->setWidget(this);

	completer->setCaseSensitivity(Qt::CaseInsensitive);

	searchE = new LineEdit;
#ifndef Q_OS_ANDROID
	connect(searchE, SIGNAL(textEdited(const QString &)), this, SLOT(searchTextEdited(const QString &)));
#endif
	connect(searchE, SIGNAL(clearButtonClicked()), this, SLOT(search()));
	connect(searchE, SIGNAL(returnPressed()), this, SLOT(search()));
	searchE->setCompleter(completer);

	searchB = new QToolButton;
	connect(searchB, SIGNAL(clicked()), this, SLOT(search()));
	searchB->setIcon(QMPlay2Core.getIconFromTheme("edit-find"));
	searchB->setToolTip(tr("Search"));
	searchB->setAutoRaise(true);

	QToolButton *showSettingsB = new QToolButton;
	connect(showSettingsB, &QToolButton::clicked, this, [] {
		emit QMPlay2Core.showSettings("Extensions");
	});
	showSettingsB->setIcon(QMPlay2Core.getIconFromTheme("configure"));
	showSettingsB->setToolTip(tr("Settings"));
	showSettingsB->setAutoRaise(true);

	m_qualityGroup = new QActionGroup(this);
	m_qualityGroup->addAction("2160p 60FPS");
	m_qualityGroup->addAction("1080p 60FPS");
	m_qualityGroup->addAction("720p 60FPS");
	m_qualityGroup->addAction("2160p");
	m_qualityGroup->addAction("1080p");
	m_qualityGroup->addAction("720p");
	m_qualityGroup->addAction("480p");

	QMenu *qualityMenu = new QMenu(this);
	int qualityIdx = 0;
	for (QAction *act : m_qualityGroup->actions())
	{
		connect(act, &QAction::triggered, this, [=] {
			sets().set("YouTube/MultiStream", true);
			sets().set("YouTube/ItagVideoList", getQualityPresetString(qualityIdx));
			sets().set("YouTube/ItagAudioList", QStringList{"251", "171", "140"});
			setItags();
		});
		act->setCheckable(true);
		qualityMenu->addAction(act);
		++qualityIdx;
	}
	qualityMenu->insertSeparator(qualityMenu->actions().at(3));

	QToolButton *qualityB = new QToolButton;
	qualityB->setPopupMode(QToolButton::InstantPopup);
	qualityB->setToolTip(tr("Quality"));
	qualityB->setIcon(QMPlay2Core.getIconFromTheme("video-display"));
	qualityB->setMenu(qualityMenu);
	qualityB->setAutoRaise(true);

	m_sortByGroup = new QActionGroup(this);
	m_sortByGroup->addAction(tr("Relevance"));
	m_sortByGroup->addAction(tr("Upload date"));
	m_sortByGroup->addAction(tr("View count"));
	m_sortByGroup->addAction(tr("Rating"));

	QMenu *sortByMenu = new QMenu(this);
	int sortByIdx = 0;
	for (QAction *act : m_sortByGroup->actions())
	{
		connect(act, &QAction::triggered, this, [=] {
			if (m_sortByIdx != sortByIdx)
			{
				m_sortByIdx = sortByIdx;
				sets().set("YouTube/SortBy", m_sortByIdx);
				search();
			}
		});
		act->setCheckable(true);
		sortByMenu->addAction(act);
		++sortByIdx;
	}

	QToolButton *sortByB = new QToolButton;
	sortByB->setPopupMode(QToolButton::InstantPopup);
	sortByB->setToolTip(tr("Sort search results by ..."));
	{
		// FIXME: Add icon
		QFont f(sortByB->font());
		f.setBold(true);
		f.setPointSize(f.pointSize() - 1);
		sortByB->setFont(f);
		sortByB->setText("A-z");
	}
	sortByB->setMenu(sortByMenu);
	sortByB->setAutoRaise(true);

	resultsW = new ResultsYoutube;

	progressB = new QProgressBar;
	progressB->hide();

	pageSwitcher = new PageSwitcher(this);
	pageSwitcher->hide();

	connect(&net, SIGNAL(finished(NetworkReply *)), this, SLOT(netFinished(NetworkReply *)));

	QGridLayout *layout = new QGridLayout;
	layout->addWidget(showSettingsB, 0, 0, 1, 1);
	layout->addWidget(qualityB, 0, 1, 1, 1);
	layout->addWidget(sortByB, 0, 2, 1, 1);
	layout->addWidget(searchE, 0, 3, 1, 1);
	layout->addWidget(searchB, 0, 4, 1, 1);
	layout->addWidget(pageSwitcher, 0, 5, 1, 1);
	layout->addWidget(resultsW, 1, 0, 1, 6);
	layout->addWidget(progressB, 2, 0, 1, 6);
	layout->setSpacing(3);
	setLayout(layout);

	SetModule(module);
}
YouTube::~YouTube()
{}

ItagNames YouTube::getItagNames(const QStringList &itagList, MediaType mediaType)
{
	if (g_itagArr.isEmpty())
	{
		/* Video + Audio */
		g_itagArr[43] = "360p WebM (VP8 + Vorbis 128kbps)";
		g_itagArr[36] = "180p MP4 (MPEG4 + AAC 32kbps)";
		g_itagArr[22] = "720p MP4 (H.264 + AAC 192kbps)"; //default
		g_itagArr[18] = "360p MP4 (H.264 + AAC 96kbps)";
		g_itagArr[ 5] = "240p FLV (FLV + MP3 64kbps)";

		/* Video */
		g_itagArr[247] = "Video  720p (VP9)";
		g_itagArr[248] = "Video 1080p (VP9)";
		g_itagArr[271] = "Video 1440p (VP9)";
		g_itagArr[313] = "Video 2160p (VP9)";
		g_itagArr[272] = "Video 4320p/2160p (VP9)";

		g_itagArr[302] = "Video  720p 60fps (VP9)";
		g_itagArr[303] = "Video 1080p 60fps (VP9)";
		g_itagArr[308] = "Video 1440p 60fps (VP9)";
		g_itagArr[315] = "Video 2160p 60fps (VP9)";

		g_itagArr[298] = "Video  720p 60fps (H.264)";
		g_itagArr[299] = "Video 1080p 60fps (H.264)";

		g_itagArr[135] = "Video  480p (H.264)";
		g_itagArr[136] = "Video  720p (H.264)";
		g_itagArr[137] = "Video 1080p (H.264)";
		g_itagArr[264] = "Video 1440p (H.264)";
		g_itagArr[266] = "Video 2160p (H.264)";

		g_itagArr[170] = "Video  480p (VP8)";
		g_itagArr[168] = "Video  720p (VP8)";
		g_itagArr[170] = "Video 1080p (VP8)";

		/* Audio */
		g_itagArr[139] = "Audio (AAC 48kbps)";
		g_itagArr[140] = "Audio (AAC 128kbps)";
		g_itagArr[141] = "Audio (AAC 256kbps)"; //?

		g_itagArr[171] = "Audio (Vorbis 128kbps)";
		g_itagArr[172] = "Audio (Vorbis 256kbps)"; //?

		g_itagArr[249] = "Audio (Opus 50kbps)";
		g_itagArr[250] = "Audio (Opus 70kbps)";
		g_itagArr[251] = "Audio (Opus 160kbps)";
	}

	ItagNames itagPair;
	for (auto it = g_itagArr.constBegin(), it_end = g_itagArr.constEnd(); it != it_end; ++it)
	{
		switch (mediaType)
		{
			case MEDIA_AV:
				if (it.value().startsWith("Video") || it.value().startsWith("Audio"))
					continue;
				break;
			case MEDIA_VIDEO:
				if (!it.value().startsWith("Video"))
					continue;
				break;
			case MEDIA_AUDIO:
				if (!it.value().startsWith("Audio"))
					continue;
				break;
		}
		itagPair.first += it.value();
		itagPair.second += it.key();
	}
	for (int i = 0, j = 0; i < itagList.count(); ++i)
	{
		const int idx = itagPair.second.indexOf(itagList[i].toInt());
		if (idx > -1)
		{
			itagPair.first.swap(j, idx);
			itagPair.second.swap(j, idx);
			++j;
		}
	}
	return itagPair;
}

bool YouTube::set()
{
	setItags();
	resultsW->setColumnCount(sets().getBool("YouTube/ShowAdditionalInfo") ? 3 : 1);
	subtitles = sets().getBool("YouTube/Subtitles");
	youtubedl = sets().getString("YouTube/youtubedl");
	if (youtubedl.isEmpty())
		youtubedl = "youtube-dl";
#ifdef Q_OS_WIN
	youtubedl.replace('\\', '/');
#endif
	m_sortByIdx = qBound(0, sets().getInt("YouTube/SortBy"), 3);
	m_sortByGroup->actions().at(m_sortByIdx)->setChecked(true);
	return true;
}

DockWidget *YouTube::getDockWidget()
{
	return dw;
}

QList<YouTube::AddressPrefix> YouTube::addressPrefixList(bool img) const
{
	return {
		AddressPrefix("YouTube", img ? youtubeIcon : QIcon()),
		AddressPrefix("youtube-dl", img ? videoIcon : QIcon())
	};
}
void YouTube::convertAddress(const QString &prefix, const QString &url, const QString &param, QString *stream_url, QString *name, QIcon *icon, QString *extension, IOController<> *ioCtrl)
{
	if (!stream_url && !name && !icon)
		return;
	if (prefix == "YouTube")
	{
		if (icon)
			*icon = youtubeIcon;
		if (ioCtrl && (stream_url || name))
		{
			NetworkAccess net;
			net.setMaxDownloadSize(0x200000 /* 2 MiB */);

			IOController<NetworkReply> &netReply = ioCtrl->toRef<NetworkReply>();
			if (net.startAndWait(netReply, url))
			{
				const QStringList youTubeVideo = getYouTubeVideo(netReply->readAll(), param, nullptr, url, ioCtrl->toPtr<YouTubeDL>());
				if (youTubeVideo.count() == 3)
				{
					if (stream_url)
						*stream_url = youTubeVideo[0];
					if (name)
						*name = youTubeVideo[2];
					if (extension)
						*extension = youTubeVideo[1];
				}
			}
			netReply.reset();
		}
	}
	else if (prefix == "youtube-dl")
	{
		if (icon)
			*icon = videoIcon;
		if (ioCtrl)
		{
			IOController<YouTubeDL> &youtube_dl = ioCtrl->toRef<YouTubeDL>();
			if (ioCtrl->assign(new YouTubeDL))
			{
				youtube_dl->addr(url, param, stream_url, name, extension);
				ioCtrl->reset();
			}
		}
	}
}

QVector<QAction *> YouTube::getActions(const QString &name, double, const QString &url, const QString &, const QString &)
{
	if (name != url)
	{
		QAction *act = new QAction(YouTube::tr("Search on YouTube"), nullptr);
		act->connect(act, SIGNAL(triggered()), this, SLOT(searchMenu()));
		act->setIcon(youtubeIcon);
		act->setProperty("name", name);
		return {act};
	}
	return {};
}

inline QString YouTube::getYtDlPath() const
{
	return youtubedl;
}

void YouTube::next()
{
	pageSwitcher->currPageB->setValue(pageSwitcher->currPageB->value() + 1);
	chPage();
}
void YouTube::prev()
{
	pageSwitcher->currPageB->setValue(pageSwitcher->currPageB->value() - 1);
	chPage();
}
void YouTube::chPage()
{
	if (currPage != pageSwitcher->currPageB->value())
	{
		currPage = pageSwitcher->currPageB->value();
		search();
	}
}

void YouTube::searchTextEdited(const QString &text)
{
	if (autocompleteReply)
		autocompleteReply->deleteLater();
	if (text.isEmpty())
		((QStringListModel *)completer->model())->setStringList({});
	else
		autocompleteReply = net.start(getAutocompleteUrl(text));
}
void YouTube::search()
{
	const QString title = searchE->text();
	deleteReplies();
	if (autocompleteReply)
		autocompleteReply->deleteLater();
	if (searchReply)
		searchReply->deleteLater();
	resultsW->clear();
	if (!title.isEmpty())
	{
		if (lastTitle != title || sender() == searchE || sender() == searchB || qobject_cast<QAction *>(sender()))
			currPage = 1;
		searchReply = net.start(getYtUrl(title, currPage, m_sortByIdx));
		progressB->setRange(0, 0);
		progressB->show();
	}
	else
	{
		pageSwitcher->hide();
		progressB->hide();
	}
	lastTitle = title;
}

void YouTube::netFinished(NetworkReply *reply)
{
	if (reply->hasError())
	{
		if (reply == searchReply)
		{
			deleteReplies();
			resultsW->clear();
			lastTitle.clear();
			progressB->hide();
			pageSwitcher->hide();
			emit QMPlay2Core.sendMessage(tr("Connection error"), YouTubeName, 3);
		}
	}
	else
	{
		QTreeWidgetItem *tWI = ((QTreeWidgetItem *)reply->property("tWI").value<void *>());
		const QByteArray replyData = reply->readAll();
		if (reply == autocompleteReply)
			setAutocomplete(replyData);
		else if (reply == searchReply)
			setSearchResults(replyData);
		else if (linkReplies.contains(reply))
		{
			if (!isPlaylist(tWI))
				getYouTubeVideo(replyData, QString(), tWI);
			else
				preparePlaylist(replyData, tWI);
		}
		else if (imageReplies.contains(reply))
		{
			QPixmap p;
			if (p.loadFromData(replyData))
				tWI->setIcon(0, p);
		}
	}

	if (linkReplies.contains(reply))
	{
		linkReplies.removeOne(reply);
		progressB->setValue(progressB->value() + 1);
	}
	else if (imageReplies.contains(reply))
	{
		imageReplies.removeOne(reply);
		progressB->setValue(progressB->value() + 1);
	}

	if (progressB->isVisible() && linkReplies.isEmpty() && imageReplies.isEmpty())
		progressB->hide();

	reply->deleteLater();
}

void YouTube::searchMenu()
{
	const QString name = sender()->property("name").toString();
	if (!name.isEmpty())
	{
		if (!dw->isVisible())
			dw->show();
		dw->raise();
		searchE->setText(name);
		search();
	}
}

void YouTube::setItags()
{
	resultsW->itagsVideo = YouTube::getItagNames(sets().getStringList("YouTube/ItagVideoList"), YouTube::MEDIA_VIDEO).second;
	resultsW->itagsAudio = YouTube::getItagNames(sets().getStringList("YouTube/ItagAudioList"), YouTube::MEDIA_AUDIO).second;
	resultsW->itags = YouTube::getItagNames(sets().getStringList("YouTube/ItagList"), YouTube::MEDIA_AV).second;
	multiStream = sets().getBool("YouTube/MultiStream");

	if (multiStream)
	{
		const bool audioOK = (resultsW->itagsAudio.count() >= 2 && (resultsW->itagsAudio.at(0) == 251 || resultsW->itagsAudio.at(0) == 171));
		if (audioOK)
		{
			for (int i = 0; i < QUALITY_PRESETS_COUNT; ++i)
			{
				const QList<int> *qualityPresets = getQualityPresets();
				if (resultsW->itagsVideo.mid(0, qualityPresets[i].count()) == qualityPresets[i])
				{
					m_qualityGroup->actions().at(i)->setChecked(true);
					return;
				}
			}
		}
	}

	for (QAction *act : m_qualityGroup->actions())
		if (act->isChecked())
			act->setChecked(false);
}

void YouTube::deleteReplies()
{
	while (!linkReplies.isEmpty())
		linkReplies.takeFirst()->deleteLater();
	while (!imageReplies.isEmpty())
		imageReplies.takeFirst()->deleteLater();
}

void YouTube::setAutocomplete(const QByteArray &data)
{
	QJsonParseError jsonErr;
	const QJsonDocument json = QJsonDocument::fromJson(data, &jsonErr);
	if (jsonErr.error != QJsonParseError::NoError)
	{
		qCWarning(youtube) << "Cannot parse autocomplete JSON:" << jsonErr.errorString();
		return;
	}
	const QJsonArray mainArr = json.array();
	if (mainArr.count() < 2)
	{
		qCWarning(youtube) << "Invalid autocomplete JSON array";
		return;
	}
	const QJsonArray arr = mainArr.at(1).toArray();
	if (arr.isEmpty())
		return;
	QStringList list;
	list.reserve(arr.count());
	for (const QJsonValue &val : arr)
		list += val.toString();
	((QStringListModel *)completer->model())->setStringList(list);
	if (searchE->hasFocus())
		completer->complete();
}
void YouTube::setSearchResults(QString data)
{
	/* Usuwanie komentarzy HTML */
	for (int commentIdx = 0 ;;)
	{
		if ((commentIdx = data.indexOf("<!--", commentIdx)) < 0)
			break;
		int commentEndIdx = data.indexOf("-->", commentIdx);
		if (commentEndIdx >= 0) //Jeżeli jest koniec komentarza
			data.remove(commentIdx, commentEndIdx - commentIdx + 3); //Wyrzuć zakomentowany fragment
		else
		{
			data.remove(commentIdx, data.length() - commentIdx); //Wyrzuć cały tekst do końca
			break;
		}
	}

	int i;
	const QStringList splitted = data.split("yt-lockup ");
	for (i = 1; i < splitted.count(); ++i)
	{
		QString title, videoInfoLink, duration, image, user;
		const QString &entry = splitted[i];
		int idx;

		if (entry.contains("yt-lockup-channel")) //Ignore channels
			continue;

		const bool isPlaylist = entry.contains("yt-lockup-playlist");

		if ((idx = entry.indexOf("yt-lockup-title")) > -1)
		{
			int urlIdx = entry.indexOf("href=\"", idx);
			int titleIdx = entry.indexOf("title=\"", idx);
			if (titleIdx > -1 && urlIdx > -1 && titleIdx > urlIdx)
			{
				const int endUrlIdx = entry.indexOf("\"", urlIdx += 6);
				const int endTitleIdx = entry.indexOf("\"", titleIdx += 7);
				if (endTitleIdx > -1 && endUrlIdx > -1 && endTitleIdx > endUrlIdx)
				{
					videoInfoLink = entry.mid(urlIdx, endUrlIdx - urlIdx).replace("&amp;", "&");
					if (!videoInfoLink.isEmpty() && videoInfoLink.startsWith('/'))
						videoInfoLink.prepend(YOUTUBE_URL);
					title = entry.mid(titleIdx, endTitleIdx - titleIdx);
				}
			}
		}
		if ((idx = entry.indexOf("video-thumb")) > -1)
		{
			int skip = 0;
			int imgIdx = entry.indexOf("data-thumb=\"", idx);
			if (imgIdx > -1)
				skip = 12;
			else
			{
				imgIdx = entry.indexOf("src=\"", idx);
				skip = 5;
			}
			if (imgIdx > -1)
			{
				int imgEndIdx = entry.indexOf("\"", imgIdx += skip);
				if (imgEndIdx > -1)
				{
					image = entry.mid(imgIdx, imgEndIdx - imgIdx);
					if (image.endsWith(".gif")) //GIF nie jest miniaturką - jest to pojedynczy piksel :D (very old code, is it still relevant?)
						image.clear();
					else if (image.startsWith("//"))
						image.prepend("https:");
					if ((idx = image.indexOf("?")) > 0)
						image.truncate(idx);
				}
			}
		}
		if (!isPlaylist && (idx = entry.indexOf("video-time")) > -1 && (idx = entry.indexOf(">", idx)) > -1)
		{
			int endIdx = entry.indexOf("<", idx += 1);
			if (endIdx > -1)
			{
				duration = entry.mid(idx, endIdx - idx);
				if (!duration.startsWith("0") && duration.indexOf(":") == 1 && duration.count(":") == 1)
					duration.prepend("0");
			}
		}
		if ((idx = entry.indexOf("yt-lockup-byline")) > -1)
		{
			int endIdx = entry.indexOf("</a>", idx);
			if (endIdx > -1 && (idx = entry.lastIndexOf(">", endIdx)) > -1)
			{
				++idx;
				user = entry.mid(idx, endIdx - idx);
			}
		}

		if (!title.isEmpty() && !videoInfoLink.isEmpty())
		{
			QTreeWidgetItem *tWI = new QTreeWidgetItem(resultsW);
			tWI->setDisabled(true);

			QTextDocument txtDoc;
			txtDoc.setHtml(title);

			tWI->setText(0, txtDoc.toPlainText());
			tWI->setText(1, !isPlaylist ? duration : tr("Playlist"));
			tWI->setText(2, user);

			tWI->setToolTip(0, QString("%1: %2\n%3: %4\n%5: %6")
				.arg(resultsW->headerItem()->text(0), tWI->text(0),
				!isPlaylist ? resultsW->headerItem()->text(1) : tr("Playlist"),
				!isPlaylist ? tWI->text(1) : tr("yes"),
				resultsW->headerItem()->text(2), tWI->text(2))
			);

			tWI->setData(0, Qt::UserRole, videoInfoLink);
			tWI->setData(1, Qt::UserRole, isPlaylist);

			NetworkReply *linkReply = net.start(videoInfoLink);
			NetworkReply *imageReply = net.start(image);
			linkReply->setProperty("tWI", qVariantFromValue((void *)tWI));
			imageReply->setProperty("tWI", qVariantFromValue((void *)tWI));
			linkReplies += linkReply;
			imageReplies += imageReply;
		}
	}

	if (i == 1)
		resultsW->clear();
	else
	{
		pageSwitcher->currPageB->setValue(currPage);
		pageSwitcher->show();

		progressB->setMaximum(linkReplies.count() + imageReplies.count());
		progressB->setValue(0);
	}
}

QStringList YouTube::getYouTubeVideo(const QString &data, const QString &PARAM, QTreeWidgetItem *tWI, const QString &url, IOController<YouTubeDL> *youtube_dl)
{
	QJsonObject args;
	int idx = data.indexOf(QRegularExpression("ytplayer.config\\s*="));
	if (idx > -1)
	{
		idx = data.indexOf('{', idx);
		if (idx > -1)
		{
			int bracketStack = 0;
			for (int i = idx; i < data.length(); ++i)
			{
				const QChar c = data.at(i);
				if (c == '{')
					++bracketStack;
				else if (c == '}')
					--bracketStack;
				if (bracketStack == 0)
				{
					QJsonParseError jsonErr;
					const QJsonDocument json = QJsonDocument::fromJson(data.midRef(idx, i - idx + 1).toUtf8(), &jsonErr);
					if (!json.isObject())
					{
						qCWarning(youtube) << "Cannot parse JSON:" << jsonErr.errorString();
						return {};
					}
					else
					{
						args = json.object()["args"].toObject();
					}
					break;
				}
			}
		}
		if (args.isEmpty())
		{
			qCWarning(youtube) << "Invalid JSON at \"ytplayer.config\"";
		}
	}

	QString subsUrl;
	if (subtitles && !tWI)
	{
		const QJsonDocument playerResponseJson = QJsonDocument::fromJson(args["player_response"].toString().toUtf8());
		const QJsonArray captionTracks = playerResponseJson.object()["captions"].toObject()["playerCaptionsTracklistRenderer"].toObject()["captionTracks"].toArray();
		const int count = captionTracks.count();
		QStringList langCodes;
		for (int i = 0; i < count; ++i)
		{
			const QString langCode = captionTracks[i].toObject()["languageCode"].toString();
			if (!langCode.isEmpty())
				langCodes += langCode;
		}
		if (!langCodes.isEmpty())
		{
			QStringList simplifiedLangCodes;
			int idx = url.indexOf("v=");
			for (const QString &lc : asConst(langCodes))
			{
				// Remove language suffix after "-" - not supported in QMPlay2
				const int idx = lc.indexOf('-');
				if (idx > -1)
					simplifiedLangCodes += lc.mid(0, idx);
				else
					simplifiedLangCodes += lc;
			}
			if (idx > -1)
			{
				QString lang = QMPlay2Core.getSettings().getString("SubtitlesLanguage");
				if (!lang.isEmpty())
				{
					// Try to convert full language name into short language code
					for (int i = QLocale::C + 1; i <= QLocale::LastLanguage; ++i)
					{
						const QLocale::Language ll = (QLocale::Language)i;
						if (lang == QLocale::languageToString(ll))
						{
							lang = QLocale(ll).name();
							const int idx = lang.indexOf('_');
							if (idx > -1)
								lang.remove(idx, lang.length() - idx);
							break;
						}
					}
					const int idx = simplifiedLangCodes.indexOf(lang);
					if (idx > -1)
						lang = langCodes.at(idx);
					else
						lang.clear();
				}
				if (lang.isEmpty())
				{
					const int idx = simplifiedLangCodes.indexOf(QMPlay2Core.getLanguage());
					if (idx > -1)
						lang = langCodes.at(idx);
				}
				if (lang.isEmpty())
					lang = langCodes.at(0);
				idx += 2;
				const int ampIdx = url.indexOf('&', idx);
				subsUrl = getSubsUrl(lang, url.mid(idx, ampIdx > -1 ? (ampIdx - idx) : -1));
			}
		}
	}

	const bool isLive = args["hlsvp"].isString();
	QStringList fmts, ret;
	if (!isLive)
	{
		fmts = QStringList {
			args["url_encoded_fmt_stream_map"].toString(),
			args["adaptive_fmts"].toString(), // contains audio or video urls
		};
	}
	for (const QString &fmt : asConst(fmts))
	{
		bool br = false;
		for (const QString &stream : fmt.split(','))
		{
			int itag = -1;
			QString ITAG, URL, Signature;
			for (const QString &streamParams : stream.split('&'))
			{
				const QStringList paramL = streamParams.split('=');
				if (paramL.count() == 2)
				{
					if (paramL[0] == "itag")
					{
						ITAG = "itag=" + paramL[1];
						itag = paramL[1].toInt();
					}
					else if (paramL[0] == "url")
					{
						URL = unescape(paramL[1]);
					}
					else if (paramL[0] == "sig")
					{
						Signature = paramL[1];
					}
					else if (paramL[0] == "s")
					{
						Signature = "ENCRYPTED";
					}
				}
			}

			if (!URL.isEmpty() && g_itagArr.contains(itag) && (!Signature.isEmpty() || URL.contains("signature")))
			{
				if (!Signature.isEmpty())
				{
					URL += "&signature=" + Signature;
				}
				if (!tWI)
				{
					if (ITAG == PARAM)
					{
						ret << URL << getFileExtension(g_itagArr[itag]);
						br = true;
						break;
					}
					else if (PARAM.isEmpty())
					{
						ret << URL << getFileExtension(g_itagArr[itag]) << QString::number(itag);
					}
				}
				else
				{
					QTreeWidgetItem *ch = new QTreeWidgetItem(tWI);
					ch->setText(0, g_itagArr[itag]); // texts visible, quality information
					if (!URL.contains("ENCRYPTED")) // youtube-dl works too slowly to run it here
						ch->setData(0, Qt::UserRole + 0, URL); // file address
					ch->setData(0, Qt::UserRole + 1, ITAG); // additional parameter
					ch->setData(0, Qt::UserRole + 2, itag); // additional parameter (as integer)
				}
			}
		}
		if (br)
			break;
	}

	if (PARAM.isEmpty() && ret.count() >= 3) //Wyszukiwanie domyślnej jakości
	{
		bool forceSingleStream = false;
		if (multiStream)
		{
			const QStringList video = getUrlByItagPriority(resultsW->itagsVideo, ret);
			const QStringList audio = getUrlByItagPriority(resultsW->itagsAudio, ret);
			if (video.count() == 2 && audio.count() == 2)
			{
				ret = QStringList {
					"FFmpeg://{[" + video[0] + "][" + audio[0] + "]",
					"[" + video[1] + "][" + audio[1] + "]"
				};
				if (!subsUrl.isEmpty())
				{
					ret[0] += "[" + subsUrl + "]";
					ret[1] += "[.vtt]";
				}
				ret[0] += "}";
			}
			else
				forceSingleStream = true;
		}
		if (!multiStream || forceSingleStream)
		{
			ret = getUrlByItagPriority(resultsW->itags, ret);
			if (ret.count() == 2 && !subsUrl.isEmpty())
			{
				ret[0] = "FFmpeg://{[" + ret[0] + "][" + subsUrl + "]}";
				ret[1] = "[" + ret[1] + "][.vtt]";
			}
		}
	}

	if (tWI) //Włącza item
	{
		tWI->setDisabled(false);
	}
	else if (ret.count() == 2) // get title
	{
		const QJsonValue titleVal = args["title"];
		if (titleVal.isString())
			ret << titleVal.toString();
		else
			ret << g_cantFindTheTitle;
	}

	if (ret.count() == 3 && ret.at(0).contains("ENCRYPTED"))
	{
		if (ret.at(0).contains("itag=") && youtube_dl->assign(new YouTubeDL))
		{
			int itagsCount = 0;
			QString itags;
			for (const QString &ITAG : ret.at(0).split("itag=", QString::SkipEmptyParts))
			{
				const int itag = atoi(ITAG.toLatin1());
				if (itag > 0)
				{
					itags += QString::number(itag) + ",";
					++itagsCount;
				}
			}
			itags.chop(1);

			const QStringList ytdl_stdout = (*youtube_dl)->exec(url, {"-f", itags});
			if (ytdl_stdout.count() != itagsCount)
				ret.clear();
			else
			{
				if (itagsCount == 1 && subsUrl.isEmpty())
					ret[0] = ytdl_stdout[0];
				else
				{
					ret[0] = "FFmpeg://{";
					for (const QString &url : ytdl_stdout)
						ret[0] += "[" + url + "]";
					if (!subsUrl.isEmpty())
						ret[0] += "[" + subsUrl + "]";
					ret[0] += "}";
				}
			}
			youtube_dl->reset();
		}
	}
	else if (!tWI && ret.isEmpty() && youtube_dl->assign(new YouTubeDL)) //cannot find URL at normal way
	{
		QString stream_url, name, extension;
		QString cleanUrl = url;
		const int idx = cleanUrl.indexOf("v=");
		if (idx > -1)
		{
			const int ampIdx = cleanUrl.indexOf('&');
			cleanUrl = YOUTUBE_URL "/watch?" + cleanUrl.mid(idx, ampIdx > -1 ? (ampIdx - idx) : -1);
		}
		(*youtube_dl)->addr(cleanUrl, PARAM.right(PARAM.length() - 5), &stream_url, &name, &extension); //extension doesn't work on youtube in this function
		if (!stream_url.isEmpty())
		{
			if (name.isEmpty())
				name = g_cantFindTheTitle;
			ret << stream_url << extension << name;
		}
		youtube_dl->reset();
	}

	return ret;
}
QStringList YouTube::getUrlByItagPriority(const QList<int> &itags, QStringList ret)
{
	for (int itag : itags)
	{
		bool br = false;
		for (int i = 2; i < ret.count(); i += 3)
			if (ret.at(i).toInt() == itag)
			{
				if (i != 2)
				{
					ret[0] = ret.at(i-2); //URL
					ret[1] = ret.at(i-1); //Extension
					ret[2] = ret.at(i-0); //Itag
				}
				br = true;
				break;
			}
		if (br)
			break;
	}
	if (!itags.contains(ret.at(2).toInt()))
		return {};
	ret.erase(ret.begin()+2, ret.end());
	return ret;
}

void YouTube::preparePlaylist(const QString &data, QTreeWidgetItem *tWI)
{
	int idx = data.indexOf("playlist-videos-container");
	if (idx > -1)
	{
		const QString tags[2] = {"video-id", "video-title"};
		QStringList playlist, entries = data.mid(idx).split("yt-uix-scroller-scroll-unit", QString::SkipEmptyParts);
		entries.removeFirst();
		for (const QString &entry : asConst(entries))
		{
			QStringList plistEntry;
			for (int i = 0; i < 2; ++i)
			{
				idx = entry.indexOf(tags[i]);
				if (idx > -1 && (idx = entry.indexOf('"', idx += tags[i].length())) > -1)
				{
					const int endIdx = entry.indexOf('"', idx += 1);
					if (endIdx > -1)
					{
						const QString str = entry.mid(idx, endIdx - idx);
						if (!i)
							plistEntry += str;
						else
						{
							QTextDocument txtDoc;
							txtDoc.setHtml(str);
							plistEntry += txtDoc.toPlainText();
						}
					}
				}
			}
			if (plistEntry.count() == 2)
				playlist += plistEntry;
		}
		if (!playlist.isEmpty())
		{
			tWI->setData(0, Qt::UserRole + 1, playlist);
			tWI->setDisabled(false);
		}
	}
}
