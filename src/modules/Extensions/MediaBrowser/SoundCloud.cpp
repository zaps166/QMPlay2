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

constexpr char client_id[] = "2add0f709fcfae1fd7a198ec7573d2d4";
constexpr char g_url[]  = "http://api.soundcloud.com";

/**/

SoundCloud::SoundCloud(NetworkAccess &net) :
	MediaBrowserCommon(net, "SoundCloud", ":/soundcloud")
{}
SoundCloud::~SoundCloud()
{}

void SoundCloud::prepareWidget(QTreeWidget *treeW)
{
	MediaBrowserCommon::prepareWidget(treeW);

	treeW->headerItem()->setText(0, tr("Title"));
	treeW->headerItem()->setText(1, tr("Artist"));
	treeW->headerItem()->setText(2, tr("Genre"));
	treeW->headerItem()->setText(3, tr("Length"));

	Functions::setHeaderSectionResizeMode(treeW->header(), 3, QHeaderView::ResizeToContents);
}

QString SoundCloud::getQMPlay2Url(const QString &text) const
{
	return QString("%1://{%2/tracks/%3}").arg(m_name, g_url, text);
}

NetworkReply *SoundCloud::getSearchReply(const QString &text, const qint32 page)
{
	return m_net.start(QString("%1/tracks?q=%2&client_id=%3&offset=%4&limit=20").arg(g_url, QUrl::toPercentEncoding(text), client_id).arg(page));
}
MediaBrowserCommon::Description SoundCloud::addSearchResults(const QByteArray &reply, QTreeWidget *treeW)
{
	const Json json = Json::parse(reply);
	if (json.is_array())
	{
		const QIcon soundcloudIcon = icon();

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
	return {};
}
bool SoundCloud::hasMultiplePages() const
{
	return true;
}

bool SoundCloud::hasWebpage() const
{
	return false;
}
QString SoundCloud::getWebpageUrl(const QString &text) const
{
	return QString();
}

MediaBrowserCommon::CompleterMode SoundCloud::completerMode() const
{
	return CompleterMode::Continuous;
}
NetworkReply *SoundCloud::getCompleterReply(const QString &text)
{
	return nullptr;
}
QStringList SoundCloud::getCompletions(const QByteArray &reply)
{
	return {};
}

QAction *SoundCloud::getAction() const
{
	QAction *act = new QAction(tr("Search on SoundCloud"), nullptr);
	act->setIcon(icon());
	return act;
}

bool SoundCloud::convertAddress(const QString &prefix, const QString &url, QString *streamUrl, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl)
{
	if (prefix != m_name)
		return false;

	if (img)
		*img = m_img;
	if (extension)
		*extension = ".mp3";
	if (ioCtrl)
	{
		NetworkAccess net;
		net.setMaxDownloadSize(0x200000 /* 2 MiB */);

		IOController<NetworkReply> &netReply = ioCtrl->toRef<NetworkReply>();
		if (net.startAndWait(netReply, g_url + QString("/resolve?url=%1&client_id=%2").arg(url, client_id)))
		{
			const Json json = Json::parse(netReply->readAll());
			if (json.is_object())
			{
				if (streamUrl)
					*streamUrl = QString("%1?client_id=%2").arg(json["stream_url"].string_value(), client_id);
				if (name)
					*name = json["title"].string_value();
			}
			netReply.clear();
		}
	}
	return true;
}
