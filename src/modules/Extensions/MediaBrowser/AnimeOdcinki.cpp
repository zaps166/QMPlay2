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
#include <YouTubeDL.hpp>
#include <Json11.hpp>

#include <QTextDocumentFragment>
#include <QTreeWidget>
#include <QProcess>

using EmbeddedPlayers = std::vector<Json>;

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

static EmbeddedPlayers getEmbeddedPlayers(const QByteArray &data)
{
	EmbeddedPlayers ret;
	ret.reserve(2);

	for (int pos = 0; ;)
	{
		int idx1 = data.indexOf("data-hash='", pos);
		if (idx1 < 0)
			break;

		idx1 += 11;

		int idx2 = data.indexOf("'", idx1);
		if (idx2 < 0)
			break;

		Json json = Json::parse(data.mid(idx1, idx2 - idx1));
		if (json.is_object())
		{
			idx1 = idx2 + 2;
			idx2 = data.indexOf("<", idx1);
			if (idx2 > -1)
			{
				QByteArray name = data.mid(idx1, idx2 - idx1).simplified();
				name = name.left(name.indexOf(' ')).toLower();

				const bool isGoogle   = (name == "google");
				const bool isOpenload = (name == "openload");
				const bool isVk       = (name == "vk");
				// TODO: VidFile ?

				if (isGoogle || isOpenload || isVk)
					ret.insert(isGoogle ? ret.begin() : ret.end(), std::move(json));
			}
		}

		pos = idx2;
	}

	return ret;
}
static QString decryptUrl(const QByteArray &saltHex, const QByteArray &ivHex, const QByteArray &cipheredBase64)
{
	QProcess process;
	process.start("openssl", {
		"enc", "-aes-256-cbc", "-d",
		"-iv", ivHex,
		"-k",  QByteArray::fromBase64("czA1ejlHcGQ9c3lHXjd7")
	});
	if (process.waitForStarted(2000))
	{
		process.write("Salted__" + QByteArray::fromHex(saltHex) + QByteArray::fromBase64(cipheredBase64));
		process.closeWriteChannel();
		if (process.waitForFinished(2000))
			return Json::parse(process.readAllStandardOutput()).string_value();
	}
	return QString();
}

static QString getGamedorUsermdUrl(const QByteArray &data)
{
	int idx1 = data.indexOf("iframe");
	if (idx1 > -1)
	{
		idx1 = data.indexOf("src=\"", idx1);
		if (idx1 > -1)
		{
			idx1 += 5;

			int idx2 = data.indexOf('"', idx1);
			if (idx2 > -1)
				return data.mid(idx1, idx2 - idx1);
		}
	}
	return QString();
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

	maybeDownloadAnimeList();
}

QString AnimeOdcinki::getQMPlay2Url(const QString &text)
{
	return QString("%1://{%2}").arg(m_name, getWebpageUrl(text));
}

NetworkReply *AnimeOdcinki::getSearchReply(const QString &text, const qint32 page)
{
	Q_UNUSED(page)
	m_currentAnime.clear();
	for (const auto &animePair : m_animePairList)
	{
		if (animePair.first.compare(text, Qt::CaseInsensitive) == 0)
		{
			m_currentAnime = animePair.second;
			break;
		}
	}
	if (m_currentAnime.isEmpty())
		m_currentAnime = text.toLower().replace(' ', '-');
	if (!m_currentAnime.isEmpty())
		return m_net.start(g_url + m_currentAnime);
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
	maybeDownloadAnimeList();
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
					const QByteArray reply = netReply->readAll();
					const QByteArray adFlyUrl = getAdFlyUrl(reply).toPercentEncoding();

					bool hasName = false;
					if (name)
					{
						int idx1 = reply.indexOf("page-header");
						if (idx1 > -1)
						{
							idx1 = reply.indexOf(">", idx1);
							if (idx1 > -1)
							{
								idx1 += 1;

								int idx2 = reply.indexOf("<", idx1);
								if (idx2 > -1)
								{
									*name = QTextDocumentFragment::fromHtml(reply.mid(idx1, idx2 - idx1)).toPlainText();
									if (!name->isEmpty())
										hasName = true;
								}
							}
						}
					}

					QString error;
					const auto getStreamUrl = [streamUrl, name, ioCtrl, hasName, &error](const QString &animeUrl)->bool {
						IOController<YouTubeDL> &ytDl = ioCtrl->toRef<YouTubeDL>();
						if (!YouTubeDL::isUpdating() && ytDl.assign(new YouTubeDL))
						{
							QString newUrl;
							ytDl->addr(animeUrl, QString(), &newUrl, hasName ? nullptr : name, nullptr, &error);
							ytDl.clear();
							if (!newUrl.isEmpty())
							{
								*streamUrl = newUrl;
								return true;
							}
						}
						return false;
					};

					bool hasStreamUrl = false;

					if (!adFlyUrl.isEmpty() && net.start(netReply, "http://www.linkexpander.com/get_url.php", "url=" + adFlyUrl, NetworkAccess::UrlEncoded))
					{
						netReply->waitForFinished();
						if (!netReply->hasError())
						{
							const QByteArray data = netReply->readAll();
							const int idx = data.indexOf("<");
							if (idx > -1)
								hasStreamUrl = getStreamUrl(data.left(idx));
						}
					}

					if (!hasStreamUrl)
					{
						for (const Json &json : getEmbeddedPlayers(reply))
						{
							QString playerUrl = decryptUrl(json["v"].string_value(), json["b"].string_value(), json["a"].string_value());
							if (!playerUrl.isEmpty())
							{
								if (playerUrl.contains("gamedor.usermd.net") && net.start(netReply, playerUrl, QByteArray(), "Referer: " + url.toUtf8()))
								{
									netReply->waitForFinished();
									if (!netReply->hasError())
									{
										playerUrl = getGamedorUsermdUrl(netReply->readAll());
										if (playerUrl.isEmpty())
											continue;
									}
								}
								hasStreamUrl = getStreamUrl(playerUrl);
								if (hasStreamUrl)
									break;
							}
						}
					}

					if (!hasStreamUrl && !error.isEmpty())
						emit QMPlay2Core.sendMessage(error, m_name, 3, 0);
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
		if (!m_animeListReply->hasError())
			m_animePairList = parseAnimeList(m_animeListReply->readAll(), nullptr);
		m_animeListReply->deleteLater();
	}
}

void AnimeOdcinki::maybeDownloadAnimeList()
{
	if (m_animePairList.isEmpty() && !m_animeListReply)
	{
		m_animeListReply = start(g_url);
		connect(m_animeListReply, SIGNAL(finished()), this, SLOT(gotAnimeList()));
	}
}
