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

#include <MediaBrowser/SoundCloud.hpp>

#include <QMPlay2Extensions.hpp>
#include <NetworkAccess.hpp>
#include <Functions.hpp>
#include <Json11.hpp>

#include <QTextDocument>
#include <QHeaderView>
#include <QTreeWidget>
#include <QAction>
#include <QUrl>

constexpr char g_name[] = "SoundCloud";
constexpr char client_id[] = "2add0f709fcfae1fd7a198ec7573d2d4";
constexpr char g_url[]  = "http://api.soundcloud.com";

/**/

void SoundCloud::prepareWidget(QTreeWidget *treeW)
{
	MediaBrowserCommon::prepareWidget(treeW);

	treeW->headerItem()->setText(0, tr("Title"));
	treeW->headerItem()->setText(1, tr("Artist"));
	treeW->headerItem()->setText(2, tr("Genre"));
	treeW->headerItem()->setText(3, tr("Length"));

#if QT_VERSION < 0x050000
	treeW->header()->setResizeMode(0, QHeaderView::Stretch);
	treeW->header()->setResizeMode(3, QHeaderView::ResizeToContents);
#else
	treeW->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	treeW->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
#endif
}

QString SoundCloud::name() const
{
	return g_name;
}
QIcon SoundCloud::icon() const
{
	return QIcon(":/soundcloud");
}

QString SoundCloud::getQMPlay2Url(const QString &text)
{
	return QString("%1://{%2/tracks/%3}").arg(g_name, g_url, text);
}

NetworkReply *SoundCloud::getSearchReply(const QString &text, const qint32 page, NetworkAccess &net)
{
	return net.start(QString("%1/tracks?q=%2&client_id=%3&offset=%4&limit=20").arg(g_url, QUrl::toPercentEncoding(text), client_id).arg(page));
}
void SoundCloud::addSearchResults(const QByteArray &reply, QTreeWidget *treeW)
{
	const Json json = Json::parse(reply);
	if (json.is_array())
	{
		const QIcon soundcloudIcon(":/soundcloud");

		for (const Json &track : json.array_items())
		{
			if (!track.is_object())
				continue;

			QTreeWidgetItem *tWI = new QTreeWidgetItem(treeW);
			tWI->setData(0, Qt::UserRole, QString::number(track["id"].int_value()));
			tWI->setIcon(0, soundcloudIcon);

			const QString title = track["title"].string_value();
			tWI->setText(0, title);
			tWI->setToolTip(0, title);

			const QString artist = track["user"]["username"].string_value();
			tWI->setText(1, artist);
			tWI->setToolTip(1, artist);

			const QString genre = track["genre"].string_value();
			tWI->setText(2, genre);
			tWI->setToolTip(2, genre);

			const QString duration = Functions::timeToStr(track["duration"].int_value() / 1000.0);
			tWI->setText(3, duration);
			tWI->setToolTip(3, duration);
		}
	}
}

bool SoundCloud::hasWebpage()
{
	return false;
}
QString SoundCloud::getWebpageUrl(const QString &text)
{
	return QString();
}

bool SoundCloud::hasCompleter()
{
	return false;
}
NetworkReply *SoundCloud::getCompleterReply(const QString &text, NetworkAccess &net)
{
	return nullptr;
}
QStringList SoundCloud::getCompletions(const QByteArray &reply)
{
	return {};
}

QMPlay2Extensions::AddressPrefix SoundCloud::addressPrefixList(bool img) const
{
	return QMPlay2Extensions::AddressPrefix(g_name, img ? QImage(":/soundcloud") : QImage());
}

QAction *SoundCloud::getAction() const
{
	QAction *act = new QAction(tr("Search on SoundCloud"), nullptr);
	act->setIcon(QIcon(":/soundcloud"));
	return act;
}

bool SoundCloud::convertAddress(const QString &prefix, const QString &url, QString *streamUrl, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl)
{
	if (prefix == g_name)
	{
		if (img)
			*img = QImage(":/soundcloud");
		if (extension)
			*extension = ".mp3";
		if (ioCtrl)
		{
			NetworkAccess net;
			net.setMaxDownloadSize(0x200000 /* 2 MiB */);

			IOController<NetworkReply> &netReply = ioCtrl->toRef<NetworkReply>();
			net.start(netReply, g_url + QString("/resolve?url=%1&client_id=%2").arg(url, client_id));
			netReply->waitForFinished();
			if (!netReply->hasError())
			{
				const Json json = Json::parse(netReply->readAll());
				if (json.is_object())
				{
					if (streamUrl)
						*streamUrl = QString("%1?client_id=%2").arg(json["stream_url"].string_value(), client_id);
					if (name)
						*name = json["title"].string_value();
				}
			}
			netReply.clear();
		}
		return true;
	}
	return false;
}
