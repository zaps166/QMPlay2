/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

#include <SoundCloud.hpp>

static const char client_id[] = "2add0f709fcfae1fd7a198ec7573d2d4";

#include <Functions.hpp>
#include <LineEdit.hpp>
#include <Reader.hpp>
#include <Json11.hpp>

#include <QProgressBar>
#include <QGridLayout>
#include <QHeaderView>
#include <QToolButton>
#include <QUrl>

static const char SoundCloudURL[] = "http://api.soundcloud.com";

static inline QString getQMPlay2Url(const QTreeWidgetItem *tWI)
{
	return QString(SoundCloudName"://{%1/tracks/%2}").arg(SoundCloudURL, tWI->data(0, Qt::UserRole).toString());
}

/**/

ResultsSound::ResultsSound()
{
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setIconSize(QSize(22, 22));
	setSortingEnabled(true);
	setIndentation(0);

	headerItem()->setText(0, tr("Title"));
	headerItem()->setText(1, tr("Artist"));
	headerItem()->setText(2, tr("Genre"));
	headerItem()->setText(3, tr("Length"));

	header()->setStretchLastSection(false);
#if QT_VERSION < 0x050000
	header()->setResizeMode(0, QHeaderView::Stretch);
	header()->setResizeMode(3, QHeaderView::ResizeToContents);
#else
	header()->setSectionResizeMode(0, QHeaderView::Stretch);
	header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
#endif

	connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(playEntry(QTreeWidgetItem *)));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenu(const QPoint &)));
	setContextMenuPolicy(Qt::CustomContextMenu);
}

void ResultsSound::enqueue()
{
	QTreeWidgetItem *tWI = currentItem();
	if (tWI)
		emit QMPlay2Core.processParam("enqueue", getQMPlay2Url(tWI));
}
void ResultsSound::playCurrentEntry()
{
	playEntry(currentItem());
}

void ResultsSound::playEntry(QTreeWidgetItem *tWI)
{
	if (tWI)
		emit QMPlay2Core.processParam("open", getQMPlay2Url(tWI));
}

void ResultsSound::contextMenu(const QPoint &point)
{
	menu.clear();
	if (QTreeWidgetItem *tWI = currentItem())
	{
		menu.addAction(tr("Enqueue"), this, SLOT(enqueue()));
		menu.addAction(tr("Play"), this, SLOT(playCurrentEntry()));
		menu.addSeparator();
		QString addressPrefixName, url, param;
		if (Functions::splitPrefixAndUrlIfHasPluginPrefix(getQMPlay2Url(tWI), &addressPrefixName, &url, &param))
		{
			const QString name = tWI->text(0);
			QAction *act;
			for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
				if (!dynamic_cast<SoundCloud *>(QMPlay2Ext) && (act = QMPlay2Ext->getAction(name, -2, url, addressPrefixName, param)))
				{
					act->setParent(&menu);
					menu.addAction(act);
				}
		}
		menu.popup(viewport()->mapToGlobal(point));
	}
}

/**/

SoundCloudW::SoundCloudW() :
	currPage(1),
	searchReply(nullptr),
	net(this)
{
	dw = new DockWidget;
	connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(setEnabled(bool)));
	dw->setWindowTitle(SoundCloudName);
	dw->setObjectName(SoundCloudName);
	dw->setWidget(this);

	searchE = new LineEdit;
	connect(searchE, SIGNAL(clearButtonClicked()), this, SLOT(search()));
	connect(searchE, SIGNAL(returnPressed()), this, SLOT(search()));

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

	resultsW = new ResultsSound;

	connect(&net, SIGNAL(finished(NetworkReply *)), this, SLOT(netFinished(NetworkReply *)));

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(searchE, 0, 0, 1, 1);
	layout->addWidget(searchB, 0, 1, 1, 1);
	layout->addWidget(nextPageB, 0, 2, 1, 1);
	layout->addWidget(resultsW, 1, 0, 1, 3);
	layout->addWidget(progressB, 2, 0, 1, 3);
}

void SoundCloudW::next()
{
	++currPage;
	search();
}

void SoundCloudW::search()
{
	const QString name = QUrl::toPercentEncoding(searchE->text());
	if (searchReply)
	{
		searchReply->deleteLater();
		searchReply = nullptr;
	}
	resultsW->clear();
	if (!name.isEmpty())
	{
		if (lastName != name || sender() == searchE || sender() == searchB)
			currPage = 1;
		searchReply = net.start(SoundCloudURL + QString("/tracks?q=%1&client_id=%2&offset=%3&limit=20").arg(name, client_id).arg(currPage));
		progressB->show();
	}
	else
	{
		nextPageB->hide();
		progressB->hide();
	}
	lastName = name;
}

void SoundCloudW::netFinished(NetworkReply *reply)
{
	if (reply->hasError())
	{
		if (reply == searchReply)
		{
			lastName.clear();
			nextPageB->hide();
			progressB->hide();
			emit QMPlay2Core.sendMessage(tr("Connection error"), SoundCloudName, 3);
		}
	}
	else
	{
		const QByteArray replyData = reply->readAll();
		if (reply == searchReply)
		{
			const Json json = Json::parse(replyData.constData());
			if (json.is_array())
			{
				const QIcon soundcloudIcon(":/soundcloud");
				for (const Json &track : json.array_items())
				{
					if (!track.is_object())
						continue;

					QTreeWidgetItem *tWI = new QTreeWidgetItem(resultsW);
					tWI->setData(0, Qt::UserRole, QString::number(track["id"].int_value()));
					tWI->setIcon(0, soundcloudIcon);

					const QString title = track["title"].string_value().c_str();
					tWI->setText(0, title);
					tWI->setToolTip(0, title);

					const QString artist = track["user"]["username"].string_value().c_str();
					tWI->setText(1, artist);
					tWI->setToolTip(1, artist);

					const QString genre = track["genre"].string_value().c_str();
					tWI->setText(2, genre);
					tWI->setToolTip(2, genre);

					const QString duration = Functions::timeToStr(track["duration"].int_value() / 1000.0);
					tWI->setText(3, duration);
					tWI->setToolTip(3, duration);
				}
			}

			nextPageB->setVisible(resultsW->topLevelItemCount());
		}
	}
	if (reply == searchReply)
	{
		searchReply = nullptr;
		progressB->hide();
	}
	reply->deleteLater();
}

void SoundCloudW::searchMenu()
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

SoundCloud::SoundCloud(Module &module)
{
	SetModule(module);
}

bool SoundCloud::set()
{
	return true;
}

DockWidget *SoundCloud::getDockWidget()
{
	return w.dw;
}

QList<SoundCloud::AddressPrefix> SoundCloud::addressPrefixList(bool img) const
{
	return {AddressPrefix(SoundCloudName, img ? QImage(":/soundcloud") : QImage())};
}
void SoundCloud::convertAddress(const QString &prefix, const QString &url, const QString &param, QString *stream_url, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl)
{
	Q_UNUSED(param)
	if (!stream_url && !img)
		return;
	if (prefix == SoundCloudName)
	{
		if (img)
			*img = QImage(":/soundcloud");
		if (extension)
			*extension = ".mp3";
		if (ioCtrl)
		{
			IOController<Reader> &reader = ioCtrl->toRef<Reader>();
			if (Reader::create(SoundCloudURL + QString("/resolve?url=%1&client_id=%2").arg(url, client_id), reader))
			{
				std::string replyData;
				while (reader->readyRead() && !reader->atEnd() && replyData.size() < 0x200000 /* 2 MiB */)
				{
					QByteArray arr =  reader->read(4096);;
					if (arr.isEmpty())
						break;
					replyData += arr.constData();
				}
				reader.clear();

				const Json json = Json::parse(replyData);
				if (json.is_object())
				{
					if (stream_url)
						*stream_url = json["stream_url"].string_value().c_str() + QString("?client_id=") + client_id;
					if (name)
						*name = json["title"].string_value().c_str();
				}
			}
		}
	}
}

QAction *SoundCloud::getAction(const QString &name, double, const QString &url, const QString &, const QString &)
{
	if (name != url)
	{
		QAction *act = new QAction(SoundCloudW::tr("Search on SoundCloud"), nullptr);
		act->connect(act, SIGNAL(triggered()), &w, SLOT(searchMenu()));
		act->setIcon(QIcon(":/soundcloud"));
		act->setProperty("name", name);
		return act;
	}
	return nullptr;
}
