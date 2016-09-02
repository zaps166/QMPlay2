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

#include <ProstoPleer.hpp>

#include <Functions.hpp>
#include <LineEdit.hpp>
#include <Reader.hpp>

#include <QStringListModel>
#include <QDesktopServices>
#include <QTextDocument>
#include <QApplication>
#include <QProgressBar>
#include <QGridLayout>
#include <QHeaderView>
#include <QToolButton>
#include <QCompleter>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>

static QString ProstoPleerURL("http://pleer.net");

static inline QString getQMPlay2Url(const QTreeWidgetItem *tWI)
{
	return ProstoPleerName"://{" + ProstoPleerURL + "/en/tracks/" + tWI->data(0, Qt::UserRole).toString() + "}";
}
static inline QString getPageUrl(const QTreeWidgetItem *tWI)
{
	return ProstoPleerURL + "/en/tracks/" + tWI->data(0, Qt::UserRole).toString();
}

/**/

ResultsPleer::ResultsPleer()
{
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setIconSize(QSize(22, 22));
	setSortingEnabled(true);
	setIndentation(0);

	headerItem()->setText(0, tr("Title"));
	headerItem()->setText(1, tr("Artist"));
	headerItem()->setText(2, tr("Length"));
	headerItem()->setText(3, tr("Bitrate"));

	header()->setStretchLastSection(false);
#if QT_VERSION < 0x050000
	header()->setResizeMode(0, QHeaderView::Stretch);
	header()->setResizeMode(2, QHeaderView::ResizeToContents);
	header()->setResizeMode(3, QHeaderView::ResizeToContents);
#else
	header()->setSectionResizeMode(0, QHeaderView::Stretch);
	header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
#endif

	connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(playEntry(QTreeWidgetItem *)));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenu(const QPoint &)));
	setContextMenuPolicy(Qt::CustomContextMenu);
}

void ResultsPleer::enqueue()
{
	QTreeWidgetItem *tWI = currentItem();
	if (tWI)
		emit QMPlay2Core.processParam("enqueue", getQMPlay2Url(tWI));
}
void ResultsPleer::playCurrentEntry()
{
	playEntry(currentItem());
}
void ResultsPleer::openPage()
{
	QTreeWidgetItem *tWI = currentItem();
	if (tWI)
		QDesktopServices::openUrl(getPageUrl(tWI));
}
void ResultsPleer::copyPageURL()
{
	QTreeWidgetItem *tWI = currentItem();
	if (tWI)
	{
		QMimeData *mimeData = new QMimeData;
		mimeData->setText(getPageUrl(tWI));
		QApplication::clipboard()->setMimeData(mimeData);
	}
}

void ResultsPleer::playEntry(QTreeWidgetItem *tWI)
{
	if (tWI)
		emit QMPlay2Core.processParam("open", getQMPlay2Url(tWI));
}

void ResultsPleer::contextMenu(const QPoint &point)
{
	menu.clear();
	QTreeWidgetItem *tWI = currentItem();
	if (tWI)
	{
		menu.addAction(tr("Enqueue"), this, SLOT(enqueue()));
		menu.addAction(tr("Play"), this, SLOT(playCurrentEntry()));
		menu.addSeparator();
		menu.addAction(tr("Open the page in the browser"), this, SLOT(openPage()));
		menu.addAction(tr("Copy page address"), this, SLOT(copyPageURL()));
		menu.addSeparator();
		const QString name = tWI->text(0);
		foreach (QMPlay2Extensions *QMPlay2Ext, QMPlay2Extensions::QMPlay2ExtensionsList())
			if (!dynamic_cast<ProstoPleer *>(QMPlay2Ext))
			{
				QString addressPrefixName, url, param;
				if (Functions::splitPrefixAndUrlIfHasPluginPrefix(getQMPlay2Url(tWI), &addressPrefixName, &url, &param))
					if (QAction *act = QMPlay2Ext->getAction(name, -2, url, addressPrefixName, param))
					{
						act->setParent(&menu);
						menu.addAction(act);
					}
			}
		menu.popup(viewport()->mapToGlobal(point));
	}
}

/**/

ProstoPleerW::ProstoPleerW() :
	completer(new QCompleter(new QStringListModel(this), this)),
	currPage(1),
	autocompleteReply(NULL), searchReply(NULL),
	net(this)
{
	dw = new DockWidget;
	connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(setEnabled(bool)));
	dw->setWindowTitle(ProstoPleerName);
	dw->setObjectName(ProstoPleerName);
	dw->setWidget(this);

	completer->setCaseSensitivity(Qt::CaseInsensitive);

	searchE = new LineEdit;
	connect(searchE, SIGNAL(textEdited(const QString &)), this, SLOT(searchTextEdited(const QString &)));
	connect(searchE, SIGNAL(clearButtonClicked()), this, SLOT(search()));
	connect(searchE, SIGNAL(returnPressed()), this, SLOT(search()));
	searchE->setCompleter(completer);

	searchB = new QToolButton;
	connect(searchB, SIGNAL(clicked()), this, SLOT(search()));
	searchB->setIcon(QMPlay2Core.getIconFromTheme("edit-find"));
	searchB->setToolTip(tr("Search"));
	searchB->setAutoRaise(true);

	nextPageB = new QToolButton;
	connect(nextPageB, SIGNAL(clicked()), this, SLOT(next()));
	nextPageB->setAutoRaise(true);
	nextPageB->setArrowType(Qt::RightArrow);
	nextPageB->setToolTip(tr("Next page"));
	nextPageB->hide();

	progressB = new QProgressBar;
	progressB->setRange(0, 0);
	progressB->hide();

	resultsW = new ResultsPleer;

	connect(&net, SIGNAL(finished(HttpReply *)), this, SLOT(netFinished(HttpReply *)));

	QGridLayout *layout = new QGridLayout;
	layout->addWidget(searchE, 0, 0, 1, 1);
	layout->addWidget(searchB, 0, 1, 1, 1);
	layout->addWidget(nextPageB, 0, 2, 1, 1);
	layout->addWidget(resultsW, 1, 0, 1, 3);
	layout->addWidget(progressB, 2, 0, 1, 3);
	setLayout(layout);
}

void ProstoPleerW::next()
{
	++currPage;
	search();
}

void ProstoPleerW::searchTextEdited(const QString &text)
{
	if (autocompleteReply)
	{
		autocompleteReply->deleteLater();
		autocompleteReply = NULL;
	}
	if (text.isEmpty())
		((QStringListModel *)completer->model())->setStringList(QStringList());
	else
		autocompleteReply = net.start(ProstoPleerURL + "/search_suggest", QByteArray("part=" + text.toUtf8()), "Content-Type: application/x-www-form-urlencoded");
}
void ProstoPleerW::search()
{
	const QString name = searchE->text();
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
	resultsW->clear();
	if (!name.isEmpty())
	{
		if (lastName != name || sender() == searchE || sender() == searchB)
			currPage = 1;
		searchReply = net.start(ProstoPleerURL + QString("/search?q=%1&page=%2").arg(name).arg(currPage));
		progressB->show();
	}
	else
	{
		nextPageB->hide();
		progressB->hide();
	}
	lastName = name;
}

void ProstoPleerW::netFinished(HttpReply *reply)
{
	if (reply->error())
	{
		if (reply == searchReply)
		{
			lastName.clear();
			nextPageB->hide();
			progressB->hide();
			emit QMPlay2Core.sendMessage(tr("Connection error"), ProstoPleerName, 3);
		}
	}
	else
	{
		const QString replyData = reply->readAll();
		if (reply == autocompleteReply)
		{
			int idx1 = replyData.indexOf("[");
			int idx2 = replyData.lastIndexOf("]");
			if (idx1 > -1 && idx2 > idx1)
			{
				QTextDocument txtDoc;
				txtDoc.setHtml(replyData.mid(idx1 + 1, idx2 - idx1 - 1));
				const QStringList suggestions = txtDoc.toPlainText().remove('"').split(',');
				if (!suggestions.isEmpty())
				{
					((QStringListModel *)completer->model())->setStringList(suggestions);
					if (searchE->hasFocus())
						completer->complete();
				}
			}
		}
		else if (reply == searchReply)
		{
			QRegExp regexp("<li duration=\"([\\d]+)\"\\s+file_id=\"([^\"]+)\"\\s+singer=\"([^\"]+)\"\\s+song=\"([^\"]+)\"\\s+link=\"([^\"]+)\"\\s+rate=\"([^\"]+)\"\\s+size=\"([^\"]+)\"");
			QIcon prostopleerIcon(":/prostopleer");
			regexp.setMinimal(true);
			QTextDocument txtDoc;
			int offset = 0;
			while ((offset = regexp.indexIn(replyData, offset)) != -1)
			{
				QTreeWidgetItem *tWI = new QTreeWidgetItem(resultsW);
				tWI->setData(0, Qt::UserRole, regexp.cap(5)); //file_id
				tWI->setIcon(0, prostopleerIcon);

				txtDoc.setHtml(regexp.cap(4));
				tWI->setText(0, txtDoc.toPlainText());
				tWI->setToolTip(0, txtDoc.toPlainText());

				txtDoc.setHtml(regexp.cap(3));
				tWI->setText(1, txtDoc.toPlainText());
				tWI->setToolTip(1, txtDoc.toPlainText());

				int time = regexp.cap(1).toInt();
				tWI->setText(2, Functions::timeToStr(time));

				QString bitrate = regexp.cap(6).toLower().remove(' ').replace('/', 'p');
				if (bitrate == "vbr" && time > 0)
				{
					const QStringList fSizeList = regexp.cap(7).toLower().split(' ');
					if (fSizeList.count() >= 2 && fSizeList[1] == "mb")
					{
						float fSize = fSizeList[0].toFloat();
						if (fSize > 0.0f)
						{
							fSize *= 8.0f * 1024.0f;
							bitrate = QString("%1kbps").arg((int)(fSize / time), 3);
						}
					}
				}
				else if (bitrate.length() == 6)
					bitrate.prepend(" ");
				else if (bitrate.length() == 5)
					bitrate.prepend("  ");
				tWI->setText(3, bitrate);

				offset += regexp.matchedLength();
			}
			nextPageB->setVisible(resultsW->topLevelItemCount());
		}
	}
	if (reply == autocompleteReply)
		autocompleteReply = NULL;
	else if (reply == searchReply)
	{
		searchReply = NULL;
		progressB->hide();
	}
	reply->deleteLater();
}

void ProstoPleerW::searchMenu()
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

/**/

ProstoPleer::ProstoPleer(Module &module)
{
	SetModule(module);
}

bool ProstoPleer::set()
{
	return true;
}

DockWidget *ProstoPleer::getDockWidget()
{
	return w.dw;
}

QList<ProstoPleer::AddressPrefix> ProstoPleer::addressPrefixList(bool img) const
{
	return QList<AddressPrefix>() << AddressPrefix(ProstoPleerName, img ? QImage(":/prostopleer") : QImage());
}
void ProstoPleer::convertAddress(const QString &prefix, const QString &url, const QString &param, QString *stream_url, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl)
{
	Q_UNUSED(param)
	Q_UNUSED(name)
	if (!stream_url && !img)
		return;
	if (prefix == ProstoPleerName)
	{
		if (img)
			*img = QImage(":/prostopleer");
		if (extension)
			*extension = ".mp3";
		if (ioCtrl && stream_url)
		{
			QString fileId = url;
			while (fileId.endsWith('/'))
				fileId.truncate(1);
			int idx = url.lastIndexOf('/');

			IOController<Reader> &reader = ioCtrl->toRef<Reader>();
			if (idx > -1 && Reader::create(ProstoPleerURL + "/site_api/files/get_url?id=" + fileId.mid(idx + 1), reader))
			{
				QByteArray replyData;
				while (reader->readyRead() && !reader->atEnd() && replyData.size() < 0x200000 /* 2 MiB */)
				{
					QByteArray arr =  reader->read(4096);;
					if (arr.isEmpty())
						break;
					replyData += arr;
				}
				reader.clear();

				replyData.replace('"', QByteArray());
				int idx = replyData.indexOf("track_link:");
				if (idx > -1)
				{
					idx += 11;
					int idx2 = replyData.indexOf('}', idx);
					if (idx2 > -1)
					{
						*stream_url = replyData.mid(idx, idx2 - idx);
						return;
					}
				}

				if (!replyData.isEmpty())
					QMPlay2Core.sendMessage(ProstoPleerW::tr("Try again later"), ProstoPleerName);
			}
		}
	}
}

QAction *ProstoPleer::getAction(const QString &name, double, const QString &url, const QString &, const QString &)
{
	if (name != url)
	{
		QAction *act = new QAction(ProstoPleerW::tr("Search on Prostopleer"), NULL);
		act->connect(act, SIGNAL(triggered()), &w, SLOT(searchMenu()));
		act->setIcon(QIcon(":/prostopleer"));
		act->setProperty("name", name);
		return act;
	}
	return NULL;
}
