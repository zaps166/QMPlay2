#include <YouTube.hpp>

#include <Functions.hpp>
#include <LineEdit.hpp>
#include <Playlist.hpp>
#include <Reader.hpp>

#include <QStringListModel>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTextDocument>
#include <QProgressBar>
#include <QApplication>
#include <QHeaderView>
#include <QGridLayout>
#include <QMouseEvent>
#include <QToolButton>
#include <QCompleter>
#include <QClipboard>
#include <QMimeData>
#include <QSpinBox>
#include <QProcess>
#include <QAction>
#include <QLabel>
#include <QDrag>
#include <QFile>
#include <QDir>

static const char cantFindTheTitle[] = "(Can't find the title)";
static QMap<int, QString> itag_arr;

static inline QUrl getYtUrl(const QString &title, const int page)
{
	return QString("https://www.youtube.com/results?search_query=%1&page=%2").arg(title).arg(page);
}
static inline QUrl getAutocompleteUrl(const QString &text)
{
	return QString("http://suggestqueries.google.com/complete/search?client=firefox&ds=yt&q=%1").arg(text);
}
static inline QString getSubsUrl(const QString &langCode, const QString &vidId)
{
	return QString("https://www.youtube.com/api/timedtext?lang=%1&fmt=vtt&v=%2").arg(langCode).arg(vidId);
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

static QString fromU(QString s)
{
	int uIdx = 0;
	for (;;)
	{
		uIdx = s.indexOf("\\u", uIdx);
		if (uIdx == -1)
			break;
		bool ok;
		const QChar chr = s.mid(uIdx + 2, 4).toUShort(&ok, 16);
		if (ok)
		{
			s.replace(uIdx, 6, chr);
			++uIdx;
		}
		else
			uIdx += 6;
	}
	return s;
}

static inline bool isPlaylist(QTreeWidgetItem *tWI)
{
	return tWI->data(1, Qt::UserRole).toBool();
}

/**/

static bool youtubedl_updating;

class YouTubeDL : public BasicIO
{
public:
	inline YouTubeDL(const QString &youtubedl) :
		youtubedl(youtubedl),
		aborted(false)
	{}

	void addr(const QString &url, const QString &param, QString *stream_url, QString *name, QString *extension)
	{
		if (stream_url || name)
		{
			QStringList paramList = QStringList() << "-e";
			if (!param.isEmpty())
				paramList << "-f" << param;
			QStringList ytdl_stdout = exec(url, paramList);
			if (!ytdl_stdout.isEmpty())
			{
				QString title;
				if (ytdl_stdout.count() > 1 && !ytdl_stdout.at(0).contains("://"))
					title = ytdl_stdout.takeFirst();
				if (stream_url)
				{
					if (ytdl_stdout.count() == 1)
						*stream_url = ytdl_stdout.at(0);
					else
					{
						*stream_url = "FFmpeg://{";
						foreach (const QString &tmpUrl, ytdl_stdout)
							*stream_url += "[" + tmpUrl + "]";
						*stream_url += "}";
					}
				}
				if (name && !title.isEmpty())
					*name = title;
				if (extension)
				{
					QStringList extensions;
					foreach (const QString &tmpUrl, ytdl_stdout)
					{
						if (tmpUrl.contains("mp4"))
							extensions += ".mp4";
						else if (tmpUrl.contains("webm"))
							extensions += ".webm";
						else if (tmpUrl.contains("mkv"))
							extensions += ".mkv";
						else if (tmpUrl.contains("mpg"))
							extensions += ".mpg";
						else if (tmpUrl.contains("mpeg"))
							extensions += ".mpeg";
						else if (tmpUrl.contains("flv"))
							extensions += ".flv";
					}
					if (extensions.count() == 1)
						*extension = extensions.at(0);
					else foreach (const QString &tmpExt, extensions)
						*extension += "[" + tmpExt + "]";
				}
			}
		}
	}

	QStringList exec(const QString &url, const QStringList &args, bool canUpdate = true)
	{
#ifndef Q_OS_WIN
		QFile youtube_dl_file(youtubedl);
		if (youtube_dl_file.exists())
		{
			QFile::Permissions exeFlags = QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther;
			if ((youtube_dl_file.permissions() & exeFlags) != exeFlags)
				youtube_dl_file.setPermissions(youtube_dl_file.permissions() | exeFlags);
		}
#endif
		youtubedl_process.start(youtubedl, QStringList() << url << "-g" << args);
		if (youtubedl_process.waitForFinished() && !aborted)
		{
			if (youtubedl_process.exitCode() != 0)
			{
				QString error = youtubedl_process.readAllStandardError();
				int idx = error.indexOf("ERROR:");
				if (idx > -1)
					error.remove(0, idx);
				if (canUpdate && error.contains("youtube-dl -U")) //Update is necessary
				{
					youtubedl_updating = true;
					youtubedl_process.start(youtubedl, QStringList() << "-U");
					if (youtubedl_process.waitForFinished(-1) && !aborted)
					{
						const QString error2 = youtubedl_process.readAllStandardOutput() + youtubedl_process.readAllStandardError();
						if (error2.contains("ERROR:") || error2.contains("package manager"))
							error += "\n" + error2;
						else if (youtubedl_process.exitCode() == 0)
						{
#ifdef Q_OS_WIN
							const QString updatedFile = youtubedl + ".new";
							QFile::remove(Functions::filePath(youtubedl) + "youtube-dl-updater.bat");
							if (QFile::exists(updatedFile))
							{
								Functions::s_wait(0.1); //Wait 100ms to be sure that file is closed
								QFile::remove(youtubedl);
								QFile::rename(updatedFile, youtubedl);
#endif
								youtubedl_updating = false;
								return exec(url, args, false);
#ifdef Q_OS_WIN
							}
							else
								error += "\nUpdated youtube-dl file: \"" + updatedFile + "\" not found!";
#endif
						}
					}
					youtubedl_updating = false;
				}
				if (!aborted)
					emit QMPlay2Core.sendMessage(error, YouTubeName + QString(" (%1)").arg(Functions::fileName(youtubedl)), 3, 0);
				return QStringList();
			}
			return QString(youtubedl_process.readAllStandardOutput()).split('\n', QString::SkipEmptyParts);
		}
		else if (!aborted && youtubedl_process.error() == QProcess::FailedToStart)
			emit QMPlay2Core.sendMessage(YouTubeW::tr("There is no external program - 'youtube-dl'. Download it, and then set the path to it in the YouTube module options."), YouTubeName, 2, 0);
		return QStringList();
	}
private:
	void abort()
	{
		aborted = true;
		youtubedl_process.kill();
	}

	QProcess youtubedl_process;
	QString youtubedl;
	bool aborted;
};

/**/

ResultsYoutube::ResultsYoutube()
{
	setAnimated(true);
	setIndentation(12);
	setExpandsOnDoubleClick(false);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	headerItem()->setText(0, tr("Title"));
	headerItem()->setText(1, tr("Length"));
	headerItem()->setText(2, tr("User"));

	header()->setStretchLastSection(false);
#if QT_VERSION < 0x050000
	header()->setResizeMode(0, QHeaderView::Stretch);
	header()->setResizeMode(1, QHeaderView::ResizeToContents);
#else
	header()->setSectionResizeMode(0, QHeaderView::Stretch);
	header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
#endif

	connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(playEntry(QTreeWidgetItem *)));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenu(const QPoint &)));
	setContextMenuPolicy(Qt::CustomContextMenu);
}

void ResultsYoutube::clearAll()
{
	removeTmpFile();
	clear();
}

QTreeWidgetItem *ResultsYoutube::getDefaultQuality(const QTreeWidgetItem *tWI)
{
	if (!tWI->childCount())
		return NULL;
	foreach (int itag, itags)
		for (int i = 0; i < tWI->childCount(); ++i)
			if (tWI->child(i)->data(0, Qt::UserRole + 2).toInt() == itag)
				return tWI->child(i);
	return tWI->child(0);
}

void ResultsYoutube::removeTmpFile()
{
	if (!fileToRemove.isEmpty())
	{
		QFile::remove(fileToRemove);
		fileToRemove.clear();
	}
}

void ResultsYoutube::mouseMoveEvent(QMouseEvent *e)
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
			drag->setPixmap(tWI->data(0, Qt::DecorationRole).value<QPixmap>());
			drag->exec(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction);
			return;
		}
	}
	QTreeWidget::mouseMoveEvent(e);
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
void ResultsYoutube::playOrEnqueue(const QString &param, QTreeWidgetItem *tWI)
{
	if (!tWI)
		return;
	if (!isPlaylist(tWI))
		emit QMPlay2Core.processParam(param, getQMPlay2Url(tWI));
	else
	{
		const QStringList ytPlaylist = tWI->data(0, Qt::UserRole + 1).toStringList();
		Playlist::Entries entries;
		for (int i = 0; i < ytPlaylist.count() ; i += 2)
		{
			Playlist::Entry entry;
			entry.name = ytPlaylist[i+1];
			entry.url = "YouTube://{https://www.youtube.com/watch?v=" + ytPlaylist[i+0] + "}";
			entries += entry;
		}
		if (!entries.isEmpty())
		{
			const QString fileName = QDir::tempPath() + "/" + Functions::cleanFileName(tWI->text(0)) + ".pls";
			removeTmpFile();
			if (Playlist::write(entries, "file://" + fileName))
			{
				emit QMPlay2Core.processParam(param, fileName);
				fileToRemove = fileName;
			}
		}
	}
}

void ResultsYoutube::openPage()
{
	QTreeWidgetItem *tWI = currentItem();
	if (tWI)
	{
		if (tWI->parent())
			tWI = tWI->parent();
		QMPlay2Core.run(tWI->data(0, Qt::UserRole).toString());
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
	menu.clear();
	QTreeWidgetItem *tWI = currentItem();
	if (tWI)
	{
		const bool isOK = !tWI->isDisabled();
		if (isOK)
		{
			menu.addAction(tr("Enqueue"), this, SLOT(enqueue()));
			menu.addAction(tr("Play"), this, SLOT(playCurrentEntry()));
			menu.addSeparator();
		}
		menu.addAction(tr("Open the page in the browser"), this, SLOT(openPage()));
		menu.addAction(tr("Copy page address"), this, SLOT(copyPageURL()));
		menu.addSeparator();
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
				menu.addAction(tr("Copy stream address"), this, SLOT(copyStreamURL()))->setProperty("StreamUrl", streamUrl);
				menu.addSeparator();
			}

			const QString name = tWI->parent() ? tWI->parent()->text(0) : tWI->text(0);
			foreach (QMPlay2Extensions *QMPlay2Ext, QMPlay2Extensions::QMPlay2ExtensionsList())
				if (!dynamic_cast<YouTube *>(QMPlay2Ext))
				{
					QString addressPrefixName, url, param;
					if (Functions::splitPrefixAndUrlIfHasPluginPrefix(getQMPlay2Url(tWI), &addressPrefixName, &url, &param))
						if (QAction *act = QMPlay2Ext->getAction(name, -2, url, addressPrefixName, param))
						{
							act->setParent(&menu);
							menu.addAction(act);
						}
				}
		}
		menu.popup(viewport()->mapToGlobal(point));
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

YouTubeW::YouTubeW(QWidget *parent) :
	QWidget(parent),
	imgSize(QSize(100, 100)),
	completer(new QCompleter(new QStringListModel(this), this)),
	currPage(1),
	autocompleteReply(NULL), searchReply(NULL),
	net(this)
{
	dw = new DockWidget;
	connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(setEnabled(bool)));
	dw->setWindowTitle("YouTube");
	dw->setObjectName(YouTubeName);
	dw->setWidget(this);

	completer->setCaseSensitivity(Qt::CaseInsensitive);

	searchE = new LineEdit;
	connect(searchE, SIGNAL(textEdited(const QString &)), this, SLOT(searchTextEdited(const QString &)));
	connect(searchE, SIGNAL(clearButtonClicked()), this, SLOT(search()));
	connect(searchE, SIGNAL(returnPressed()), this, SLOT(search()));
	searchE->setCompleter(completer);

	searchB = new QToolButton;
	connect(searchB, SIGNAL(clicked()), this, SLOT(search()));
	searchB->setIcon(QIcon(":/browserengine"));
	searchB->setToolTip(tr("Search"));
	searchB->setAutoRaise(true);

	showSettingsB = new QToolButton;
	connect(showSettingsB, SIGNAL(clicked()), this, SLOT(showSettings()));
	showSettingsB->setIcon(QMPlay2Core.getIconFromTheme("configure"));
	showSettingsB->setToolTip(tr("Settings"));
	showSettingsB->setAutoRaise(true);

	resultsW = new ResultsYoutube;

	progressB = new QProgressBar;
	progressB->hide();

	pageSwitcher = new PageSwitcher(this);
	pageSwitcher->hide();

	connect(&net, SIGNAL(finished(QNetworkReply *)), this, SLOT(netFinished(QNetworkReply *)));

	QGridLayout *layout = new QGridLayout;
	layout->addWidget(showSettingsB, 0, 0, 1, 1);
	layout->addWidget(searchE, 0, 1, 1, 1);
	layout->addWidget(searchB, 0, 2, 1, 1);
	layout->addWidget(pageSwitcher, 0, 3, 1, 1);
	layout->addWidget(resultsW, 1, 0, 1, 4);
	layout->addWidget(progressB, 2, 0, 1, 4);
	setLayout(layout);
}

void YouTubeW::showSettings()
{
	emit QMPlay2Core.showSettings("Extensions");
}

void YouTubeW::next()
{
	pageSwitcher->currPageB->setValue(pageSwitcher->currPageB->value() + 1);
	chPage();
}
void YouTubeW::prev()
{
	pageSwitcher->currPageB->setValue(pageSwitcher->currPageB->value() - 1);
	chPage();
}
void YouTubeW::chPage()
{
	if (currPage != pageSwitcher->currPageB->value())
	{
		currPage = pageSwitcher->currPageB->value();
		search();
	}
}

void YouTubeW::searchTextEdited(const QString &text)
{
	if (autocompleteReply)
	{
		autocompleteReply->deleteLater();
		autocompleteReply = NULL;
	}
	if (text.isEmpty())
		((QStringListModel *)completer->model())->setStringList(QStringList());
	else
		autocompleteReply = net.get(QNetworkRequest(getAutocompleteUrl(text)));
}
void YouTubeW::search()
{
	const QString title = searchE->text();
	deleteReplies();
	if (autocompleteReply)
	{
		autocompleteReply->deleteLater();
		autocompleteReply = NULL;
	}
	if (searchReply)
	{
		searchReply->deleteLater();
		searchReply = NULL;
	}
	resultsW->clearAll();
	if (!title.isEmpty())
	{
		if (lastTitle != title || sender() == searchE || sender() == searchB)
			currPage = 1;
		searchReply = net.get(QNetworkRequest(getYtUrl(title, currPage)));
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

void YouTubeW::netFinished(QNetworkReply *reply)
{
	const QUrl redirected = reply->property("Redirected").toBool() ? QUrl() : reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
	QNetworkReply *redirectedReply = (!reply->error() && redirected.isValid()) ? net.get(QNetworkRequest(redirected)) : NULL;
	if (redirectedReply)
	{
		redirectedReply->setProperty("tWI", reply->property("tWI"));
		redirectedReply->setProperty("Redirected", true);
	}

	if (reply->error())
	{
		if (reply == searchReply)
		{
			deleteReplies();
			resultsW->clearAll();
			lastTitle.clear();
			progressB->hide();
			pageSwitcher->hide();
			emit QMPlay2Core.sendMessage(tr("Connection error"), YouTubeName, 3);
		}
	}
	else if (!redirectedReply)
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
				tWI->setData(0, Qt::DecorationRole, p.scaled(imgSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		}
	}

	if (reply == autocompleteReply)
		autocompleteReply = redirectedReply;
	else if (reply == searchReply)
		searchReply = redirectedReply;
	else if (linkReplies.contains(reply))
	{
		linkReplies.removeOne(reply);
		if (!redirectedReply)
			progressB->setValue(progressB->value() + 1);
		else
			linkReplies += redirectedReply;
	}
	else if (imageReplies.contains(reply))
	{
		imageReplies.removeOne(reply);
		if (!redirectedReply)
			progressB->setValue(progressB->value() + 1);
		else
			imageReplies += redirectedReply;
	}

	if (progressB->isVisible() && linkReplies.isEmpty() && imageReplies.isEmpty())
		progressB->hide();

	reply->deleteLater();
}

void YouTubeW::searchMenu()
{
	const QString name = sender()->property("name").toString();
	if (!name.isEmpty())
	{
		if (!dw->isVisible())
			dw->show();
		dw->raise();
		sender()->setProperty("name", QVariant());
		searchE->setText(name);
		search();
	}
}

void YouTubeW::deleteReplies()
{
	while (!linkReplies.isEmpty())
		linkReplies.takeFirst()->deleteLater();
	while (!imageReplies.isEmpty())
		imageReplies.takeFirst()->deleteLater();
}

void YouTubeW::setAutocomplete(const QByteArray &data)
{
	QStringList suggestions = fromU(QString(data).remove('"').remove('[').remove(']')).split(',');
	if (suggestions.size() > 1)
	{
		suggestions.removeFirst();
		((QStringListModel *)completer->model())->setStringList(suggestions);
		if (searchE->hasFocus())
			completer->complete();
	}
}
void YouTubeW::setSearchResults(QString data)
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
						videoInfoLink.prepend("https://www.youtube.com");
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
					if (image.endsWith(".gif")) //GIF nie jest miniaturką - jest to pojedynczy piksel :D
						image.clear();
					else if (image.startsWith("//"))
						image.prepend("https:");
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
				.arg(resultsW->headerItem()->text(0)).arg(tWI->text(0))
				.arg(!isPlaylist ? resultsW->headerItem()->text(1) : tr("Playlist")).arg(!isPlaylist ? tWI->text(1) : tr("yes"))
				.arg(resultsW->headerItem()->text(2)).arg(tWI->text(2))
			);

			tWI->setData(0, Qt::UserRole, videoInfoLink);
			tWI->setData(1, Qt::UserRole, isPlaylist);

			QNetworkReply *linkReply = net.get(QNetworkRequest(videoInfoLink));
			QNetworkReply *imageReply = net.get(QNetworkRequest(image));
			linkReply->setProperty("tWI", qVariantFromValue((void *)tWI));
			imageReply->setProperty("tWI", qVariantFromValue((void *)tWI));
			linkReplies += linkReply;
			imageReplies += imageReply;
		}
	}

	if (i == 1)
		resultsW->clearAll();
	else
	{
		pageSwitcher->currPageB->setValue(currPage);
		pageSwitcher->show();

		progressB->setMaximum(linkReplies.count() + imageReplies.count());
		progressB->setValue(0);
	}
}

QStringList YouTubeW::getYouTubeVideo(const QString &data, const QString &PARAM, QTreeWidgetItem *tWI, const QString &url, IOController<YouTubeDL> *youtube_dl)
{
	QString subsUrl;
	if (subtitles && !tWI)
	{
		QStringList langCodes;
		int captionIdx = data.indexOf("caption_tracks\":\"");
		if (captionIdx > -1)
		{
			captionIdx += 17;
			const int captionEndIdx = data.indexOf('"', captionIdx);
			if (captionEndIdx > -1)
			{
				foreach (const QString &caption, data.mid(captionIdx, captionEndIdx - captionIdx).split(','))
				{
					bool isAutomated = false;
					QString lc;
					foreach (const QString &captionParams, caption.split("\\u0026"))
					{
						const QStringList paramL = captionParams.split('=');
						if (paramL.count() == 2)
						{
							if (paramL[0] == "lc")
								lc = paramL[1];
							else if (paramL[0] == "k")
								isAutomated = paramL[1].startsWith("asr");
						}
					}
					if (!isAutomated && !lc.isEmpty())
						langCodes += lc;
				}
			}
		}
		if (!langCodes.isEmpty())
		{
			QStringList simplifiedLangCodes;
			int idx = url.indexOf("v=");
			foreach (const QString &lc, langCodes)
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
					const int idx = simplifiedLangCodes.indexOf(QMPlay2Core.getSettings().getString("Language"));
					if (idx > -1)
						lang = langCodes.at(idx);
				}
				if (lang.isEmpty())
					lang = langCodes.at(0);
				idx += 2;
				subsUrl = getSubsUrl(lang, url.mid(idx, url.indexOf("&", idx)));
			}
		}
	}

	QStringList ret;
	for (int i = 0; i <= 1; ++i)
	{
		const QString fmts = QString(i ? "adaptive_fmts" : "url_encoded_fmt_stream_map") + "\":\""; //"adaptive_fmts" contains audio or video urls
		int streamsIdx = data.indexOf(fmts);
		if (streamsIdx > -1)
		{
			streamsIdx += fmts.length();
			const int streamsEndIdx = data.indexOf('"', streamsIdx);
			if (streamsEndIdx > -1)
			{
				foreach (const QString &stream, data.mid(streamsIdx, streamsEndIdx - streamsIdx).split(','))
				{
					int itag = -1;
					QString ITAG, URL, Signature;
					foreach (const QString &streamParams, stream.split("\\u0026"))
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
								URL = unescape(paramL[1]);
							else if (paramL[0] == "sig")
								Signature = paramL[1];
							else if (paramL[0] == "s")
								Signature = "ENCRYPTED";
						}
					}

					if (!URL.isEmpty() && itag_arr.contains(itag) && (!Signature.isEmpty() || URL.contains("signature")))
					{
						if (!Signature.isEmpty())
							URL += "&signature=" + Signature;
						if (!tWI)
						{
							if (ITAG == PARAM)
							{
								ret << URL << getFileExtension(itag_arr[itag]);
								++i; //ensures end of the loop
								break;
							}
							else if (PARAM.isEmpty())
								ret << URL << getFileExtension(itag_arr[itag]) << QString::number(itag);
						}
						else
						{
							QTreeWidgetItem *ch = new QTreeWidgetItem(tWI);
							ch->setText(0, itag_arr[itag]); //Tekst widoczny, informacje o jakości
							if (!URL.contains("ENCRYPTED")) //youtube-dl działa za wolno, żeby go tu wykonać
								ch->setData(0, Qt::UserRole + 0, URL); //Adres do pliku
							ch->setData(0, Qt::UserRole + 1, ITAG); //Dodatkowy parametr
							ch->setData(0, Qt::UserRole + 2, itag); //Dodatkowy parametr (jako liczba)
						}
					}
				}
			}
		}
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
				ret = QStringList() << "FFmpeg://{[" + video[0] + "][" + audio[0] + "]" << "[" + video[1] + "][" + audio[1] + "]";
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
		tWI->setDisabled(false);
	else if (ret.count() == 2) //Pobiera tytuł
	{
		int ytplayerIdx = data.indexOf("ytplayer.config");
		if (ytplayerIdx > -1)
		{
			int titleIdx = data.indexOf("title\":\"", ytplayerIdx);
			if (titleIdx > -1)
			{
				int titleEndIdx = titleIdx += 8;
				for (;;) //szukanie końca tytułu
				{
					titleEndIdx = data.indexOf('"', titleEndIdx);
					if (titleEndIdx < 0 || data[titleEndIdx-1] != '\\')
						break;
					++titleEndIdx;
				}
				if (titleEndIdx > -1)
					ret << fromU(data.mid(titleIdx, titleEndIdx - titleIdx).replace("\\\"", "\"").replace("\\/", "/"));
			}
		}
		if (ret.count() == 2)
			ret << cantFindTheTitle;
	}

	if (ret.count() == 3 && ret.at(0).contains("ENCRYPTED"))
	{
		if (ret.at(0).contains("itag=") && !youtubedl_updating && youtube_dl->assign(new YouTubeDL(youtubedl)))
		{
			int itagsCount = 0;
			QString itags;
			foreach (const QString &ITAG, ret.at(0).split("itag=", QString::SkipEmptyParts))
			{
				const int itag = atoi(ITAG.toLatin1());
				if (itag > 0)
				{
					itags += QString::number(itag) + ",";
					++itagsCount;
				}
			}
			itags.chop(1);

			const QStringList ytdl_stdout = (*youtube_dl)->exec(url, QStringList() << "-f" << itags);
			if (ytdl_stdout.count() != itagsCount)
				ret.clear();
			else
			{
				if (itagsCount == 1 && subsUrl.isEmpty())
					ret[0] = ytdl_stdout[0];
				else
				{
					ret[0] = "FFmpeg://{";
					foreach (const QString &url, ytdl_stdout)
						ret[0] += "[" + url + "]";
					if (!subsUrl.isEmpty())
						ret[0] += "[" + subsUrl + "]";
					ret[0] += "}";
				}
			}
			youtube_dl->clear();
		}
	}
	else if (!tWI && ret.isEmpty() && !youtubedl_updating && youtube_dl->assign(new YouTubeDL(youtubedl))) //cannot find URL at normal way
	{
		QString stream_url, name, extension;
		(*youtube_dl)->addr(url, PARAM.right(PARAM.length() - 5), &stream_url, &name, &extension); //extension doesn't work on youtube in this function
		if (!stream_url.isEmpty())
		{
			if (name.isEmpty())
				name = cantFindTheTitle;
			ret << stream_url << extension << name;
		}
		youtube_dl->clear();
	}

	return ret;
}
QStringList YouTubeW::getUrlByItagPriority(const QList<int> &itags, QStringList ret)
{
	foreach (int itag, itags)
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
		return QStringList();
	ret.erase(ret.begin()+2, ret.end());
	return ret;
}

void YouTubeW::preparePlaylist(const QString &data, QTreeWidgetItem *tWI)
{
	int idx = data.indexOf("playlist-videos-container");
	if (idx > -1)
	{
		const QString tags[2] = {"video-id", "video-title"};
		QStringList playlist, entries = data.mid(idx).split("yt-uix-scroller-scroll-unit", QString::SkipEmptyParts);
		entries.removeFirst();
		foreach (const QString &entry, entries)
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

/**/

YouTube::YouTube(Module &module)
{
	SetModule(module);
}

ItagNames YouTube::getItagNames(const QStringList &itagList, MediaType mediaType)
{
	if (itag_arr.isEmpty())
	{
		/* Video + Audio */
		itag_arr[43] = "360p WebM (VP8 + Vorbis 128kbps)";
		itag_arr[36] = "180p MP4 (MPEG4 + AAC 32kbps)";
		itag_arr[22] = "720p MP4 (H.264 + AAC 192kbps)"; //default
		itag_arr[18] = "360p MP4 (H.264 + AAC 96kbps)";
		itag_arr[ 5] = "240p FLV (FLV + MP3 64kbps)";

		/* Video */
		itag_arr[247] = "Video  720p (VP9)";
		itag_arr[248] = "Video 1080p (VP9)";
		itag_arr[271] = "Video 1440p (VP9)";
		itag_arr[313] = "Video 2160p (VP9)";
		itag_arr[272] = "Video 4320p/2160p (VP9)";

		itag_arr[302] = "Video  720p 60fps (VP9)";
		itag_arr[303] = "Video 1080p 60fps (VP9)";
		itag_arr[308] = "Video 1440p 60fps (VP9)";
		itag_arr[315] = "Video 2160p 60fps (VP9)";

		itag_arr[298] = "Video  720p 60fps (H.264)";
		itag_arr[299] = "Video 1080p 60fps (H.264)";

		itag_arr[135] = "Video  480p (H.264)";
		itag_arr[136] = "Video  720p (H.264)";
		itag_arr[137] = "Video 1080p (H.264)";
		itag_arr[264] = "Video 1440p (H.264)";
		itag_arr[266] = "Video 2160p (H.264)";

		itag_arr[170] = "Video  480p (VP8)";
		itag_arr[168] = "Video  720p (VP8)";
		itag_arr[170] = "Video 1080p (VP8)";

		/* Audio */
		itag_arr[139] = "Audio (AAC 48kbps)";
		itag_arr[140] = "Audio (AAC 128kbps)";
		itag_arr[141] = "Audio (AAC 256kbps)"; //?

		itag_arr[171] = "Audio (Vorbis 128kbps)";
		itag_arr[172] = "Audio (Vorbis 256kbps)"; //?

		itag_arr[249] = "Audio (Opus 50kbps)";
		itag_arr[250] = "Audio (Opus 70kbps)";
		itag_arr[251] = "Audio (Opus 160kbps)";
	}

	ItagNames itagPair;
	for (QMap<int, QString>::const_iterator it = itag_arr.constBegin(), it_end = itag_arr.constEnd(); it != it_end; ++it)
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
	w.resultsW->setColumnCount(sets().getBool("YouTube/ShowAdditionalInfo") ? 3 : 1);
	w.resultsW->itagsVideo = getItagNames(sets().get("YouTube/ItagVideoList").toStringList(), MEDIA_VIDEO).second;
	w.resultsW->itagsAudio = getItagNames(sets().get("YouTube/ItagAudioList").toStringList(), MEDIA_AUDIO).second;
	w.resultsW->itags = getItagNames(sets().get("YouTube/ItagList").toStringList(), MEDIA_AV).second;
	w.multiStream = sets().getBool("YouTube/MultiStream");
	w.subtitles = sets().getBool("YouTube/Subtitles");
	w.youtubedl = sets().getString("YouTube/youtubedl");
	if (w.youtubedl.isEmpty())
		w.youtubedl = "youtube-dl";
#ifdef Q_OS_WIN
	w.youtubedl.replace('\\', '/');
#endif
	return true;
}

DockWidget *YouTube::getDockWidget()
{
	return w.dw;
}

QList<YouTube::AddressPrefix> YouTube::addressPrefixList(bool img)
{
	return QList<AddressPrefix>() << AddressPrefix("YouTube", img ? QImage(":/youtube") : QImage()) << AddressPrefix("youtube-dl", img ? QImage(":/video") : QImage());
}
void YouTube::convertAddress(const QString &prefix, const QString &url, const QString &param, QString *stream_url, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl)
{
	if (!stream_url && !name && !img)
		return;
	if (prefix == "YouTube")
	{
		if (img)
			*img = QImage(":/youtube");
		if (ioCtrl && (stream_url || name))
		{
			IOController<Reader> &reader = ioCtrl->toRef<Reader>();
			if (Reader::create(url, reader))
			{
				QByteArray replyData;
				while (reader->readyRead() && !reader->atEnd() && replyData.size() < 0x200000 /* < 2 MiB */)
				{
					const QByteArray arr = reader->read(4096);
					if (!arr.size())
						break;
					replyData += arr;
				}
				reader.clear();

				const bool multiStream = w.multiStream;
				const bool subtitles = w.subtitles;
				if (extension) //Don't use multi stream and subtitles when downloading
				{
					w.multiStream = false;
					w.subtitles = false;
				}
				const QStringList youTubeVideo = w.getYouTubeVideo(replyData, param, NULL, url, ioCtrl->toPtr<YouTubeDL>());
				w.multiStream = multiStream;
				w.subtitles = subtitles;
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
		}
	}
	else if (prefix == "youtube-dl")
	{
		if (img)
			*img = QImage(":/video");
		if (ioCtrl && !youtubedl_updating)
		{
			IOController<YouTubeDL> &youtube_dl = ioCtrl->toRef<YouTubeDL>();
			if (ioCtrl->assign(new YouTubeDL(w.youtubedl)))
			{
				youtube_dl->addr(url, param, stream_url, name, extension);
				ioCtrl->clear();
			}
		}
	}
}

QAction *YouTube::getAction(const QString &name, double, const QString &url, const QString &, const QString &)
{
	if (name != url)
	{
		QAction *act = new QAction(YouTubeW::tr("Search on YouTube"), NULL);
		act->connect(act, SIGNAL(triggered()), &w, SLOT(searchMenu()));
		act->setIcon(QIcon(":/youtube"));
		act->setProperty("name", name);
		return act;
	}
	return NULL;
}
