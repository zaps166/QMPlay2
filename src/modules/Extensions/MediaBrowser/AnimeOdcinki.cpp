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

#include <MediaBrowser/AnimeOdcinki.hpp>

#include <NetworkAccess.hpp>
#include <Functions.hpp>

#include <QTextDocumentFragment>
#include <QTreeWidget>
#include <QDebug>

constexpr char g_url[]  = "https://anime-odcinki.pl/anime/";

static AnimeOdcinki::AnimePairList parseAnimeList(const QByteArray &data, AnimeOdcinki::AnimePair *episodeImgDescr)
{
	AnimeOdcinki::AnimePairList animePairList;

	int idx1, idx2;

	const auto getRange = [&idx1, &idx2](const QByteArray &data, const QVector<QByteArray> &args) {
		const QByteArray first = args.value(0);
		const QByteArray second = args.value(1);
		const QByteArray preFirst = args.value(2);
		if (!preFirst.isEmpty())
		{
			idx1 = data.indexOf(preFirst);
			if (idx1 < 0)
				return false;
		}
		idx1 = data.indexOf(first, idx1);
		if (idx1 < 0)
			return false;
		idx1 += first.length();
		idx2 = data.indexOf(second, idx1);
		if (idx2 < 0)
			return false;
		return true;
	};

	idx1 = 0;
	if (episodeImgDescr)
	{
		if (!getRange(data, {"<ul>", "</ul>", "view-id-lista_odcink_w view-display-id-block"}))
			return AnimeOdcinki::AnimePairList();
	}
	else
	{
		if (!getRange(data, {"<tbody>", "</tbody>"}))
			return AnimeOdcinki::AnimePairList();
	}

	const QByteArray animeTable = data.mid(idx1, idx2 - idx1);
	idx1 = 0;
	for (;;)
	{
		if (!getRange(animeTable, {"<a href=\"", "\""}))
			break;

		const QByteArray url = animeTable.mid(idx1, idx2 - idx1);

		if (!getRange(animeTable, {">", "</a>"}))
			break;

		const QByteArray title = animeTable.mid(idx1, idx2 - idx1);

		if (!url.isEmpty() && !title.isEmpty())
		{
			animePairList.append({
				QTextDocumentFragment::fromHtml(title).toPlainText(),
				QByteArray::fromPercentEncoding(url.mid(url.lastIndexOf('/') + 1))
			});
		}
	}

	if (episodeImgDescr)
	{
		if (getRange(data, {"<img src=\"", "\"", "field-name-field-okladka"}))
			episodeImgDescr->first = data.mid(idx1, idx2 - idx1);
		if (getRange(data, {"<p><p>", "</p>", "field-type-text-with-summary"}))
			episodeImgDescr->second = data.mid(idx1, idx2 - idx1);
	}

	return animePairList;
}

static QByteArray getAdFlyUrl(const QByteArray &data)
{
	int idx1 = data.indexOf("http://adf.ly/");
	if (idx1 > -1)
	{
		int idx2 = data.indexOf('"', idx1);
		if (idx2 > -1)
			return data.mid(idx1, idx2 - idx1);
	}
	return QByteArray();
}

/**/

AnimeOdcinki::AnimeOdcinki(NetworkAccess &net) :
	MediaBrowserCommon(net, "AnimeOdcinki", ":/video")
{}
AnimeOdcinki::~AnimeOdcinki()
{}

void AnimeOdcinki::prepareWidget(QTreeWidget *treeW)
{
	MediaBrowserCommon::prepareWidget(treeW);

	treeW->headerItem()->setText(0, tr("Episode name"));

	m_currentAnime.clear();

	if (m_animePairList.isEmpty() && !m_animeListReply)
	{
		m_animeListReply = start(g_url);
		connect(m_animeListReply, SIGNAL(finished()), this, SLOT(gotAnimeList()));
	}
}

QString AnimeOdcinki::getQMPlay2Url(const QString &text)
{
	return QString("%1://{%2}").arg(m_name, getWebpageUrl(text));
}

NetworkReply *AnimeOdcinki::getSearchReply(const QString &text, const qint32 page)
{
	Q_UNUSED(page)
	for (const auto &animePair : m_animePairList)
	{
		if (animePair.first.compare(text, Qt::CaseInsensitive) == 0)
		{
			m_currentAnime = animePair.second;
			return m_net.start(g_url + m_currentAnime);
		}
	}
	return nullptr;
}
MediaBrowserCommon::Description AnimeOdcinki::addSearchResults(const QByteArray &reply, QTreeWidget *treeW)
{
	AnimePair episodeImgDescr;
	const AnimePairList animePairList = parseAnimeList(reply, &episodeImgDescr);
	const QIcon animeOdcinkiIcon = icon();
	for (const auto &animePair : animePairList)
	{
		QTreeWidgetItem *tWI = new QTreeWidgetItem(treeW);
		tWI->setData(0, Qt::UserRole, QString(m_currentAnime + "/" + animePair.second));
		tWI->setText(0, animePair.first);
		tWI->setIcon(0, animeOdcinkiIcon);
	}
	return {
		episodeImgDescr.second,
		m_net.start(episodeImgDescr.first)
	};
}
bool AnimeOdcinki::hasMultiplePages() const
{
	return false;
}

bool AnimeOdcinki::hasWebpage()
{
	return true;
}
QString AnimeOdcinki::getWebpageUrl(const QString &text)
{
	return QString("%1%2").arg(g_url, text);
}

bool AnimeOdcinki::hasCompleter()
{
	return true;
}
NetworkReply *AnimeOdcinki::getCompleterReply(const QString &text)
{
	Q_UNUSED(text)
	return nullptr;
}
QStringList AnimeOdcinki::getCompletions(const QByteArray &reply)
{
	Q_UNUSED(reply)
	QStringList completions;
	for (const auto &animePair : m_animePairList)
		completions.append(animePair.first);
	return completions;
}

QMPlay2Extensions::AddressPrefix AnimeOdcinki::addressPrefix(bool img) const
{
	return QMPlay2Extensions::AddressPrefix(m_name, img ? m_img : QImage());
}

QAction *AnimeOdcinki::getAction() const
{
	return nullptr;
}

bool AnimeOdcinki::convertAddress(const QString &prefix, const QString &url, QString *streamUrl, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl)
{
	Q_UNUSED(extension)
	if (prefix == m_name)
	{
		if (img)
			*img = m_img;
		if (ioCtrl && streamUrl)
		{
			NetworkAccess net;
			net.setMaxDownloadSize(0x200000 /* 2 MiB */);

			IOController<NetworkReply> &netReply = ioCtrl->toRef<NetworkReply>();
			if (net.start(netReply, url))
			{
				netReply->waitForFinished();
				if (!netReply->hasError())
				{
					const QByteArray adFlyUrl = getAdFlyUrl(netReply->readAll()).toPercentEncoding();
					if (!adFlyUrl.isEmpty() && net.start(netReply, "http://www.linkexpander.com/get_url.php", "url=" + adFlyUrl, NetworkAccess::UrlEncoded))
					{
						netReply->waitForFinished();
						if (!netReply->hasError())
						{
							const QByteArray data = netReply->readAll();
							const int idx = data.indexOf("<div");
							if (idx > -1)
							{
								const QString animeUrl = data.left(idx);
								Functions::getDataIfHasPluginPrefix("youtube-dl://{" + animeUrl + "}", streamUrl, name, nullptr, ioCtrl);
							}
						}
					}
				}
				netReply.clear();
			}
		}
		return true;
	}
	return false;
}

void AnimeOdcinki::gotAnimeList()
{
	NetworkReply *animeListReply = qobject_cast<NetworkReply *>(sender());
	if (animeListReply && m_animeListReply == animeListReply)
	{
		m_animePairList = parseAnimeList(m_animeListReply->readAll(), nullptr);
		m_animeListReply->deleteLater();
		m_animeListReply = nullptr;
	}
}
