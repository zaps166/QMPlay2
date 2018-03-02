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

#include <MediaBrowser/AnimeOdcinki.hpp>

#include <NetworkAccess.hpp>
#include <Functions.hpp>
#include <YouTubeDL.hpp>

#include <QTextDocumentFragment>
#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTreeWidget>
#include <QJsonArray>

Q_LOGGING_CATEGORY(animeodcinki, "AnimeOdcinki")

constexpr const char *g_url = "https://a-o.ninja/anime/";
constexpr const char *g_linkexpander = "https://linkexpander.com/get_url.php";
constexpr const char *g_serverPrioritiesUrl = "https://raw.githubusercontent.com/zaps166/QMPlay2OnlineContents/master/AnimeOdcinki.json";

static AnimeOdcinki::AnimePairList parseAnimeList(const QByteArray &data, AnimeOdcinki::AnimePair *episodeImgDescr)
{
	AnimeOdcinki::AnimePairList animePairList;

	int idx1 = 0, idx2 = 0;

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
		if (getRange(data, {"<p>", "</p>", "field-type-text-with-summary"}))
			episodeImgDescr->second = QTextDocumentFragment::fromHtml(data.mid(idx1, idx2 - idx1)).toPlainText();
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

static inline QString decryptUrl(const QString &saltHex, const QString &cipheredBase64)
{
	const QByteArray json = "{\"url\":" + Functions::decryptAes256Cbc(
		QByteArray::fromBase64("czA1ejlHcGQ9c3lHXjd7"),
		QByteArray::fromHex(saltHex.toLatin1()),
		QByteArray::fromBase64(cipheredBase64.toLatin1())
	) + "}";
	return QJsonDocument::fromJson(json).object()["url"].toString();
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
				return QTextDocumentFragment::fromHtml(data.mid(idx1, idx2 - idx1)).toPlainText();
		}
	}
	return QString();
}

/**/

AnimeOdcinki::AnimeOdcinki(NetworkAccess &net) :
	MediaBrowserCommon(net, "AnimeOdcinki", ":/video.svgz")
{}
AnimeOdcinki::~AnimeOdcinki()
{}

void AnimeOdcinki::prepareWidget(QTreeWidget *treeW)
{
	MediaBrowserCommon::prepareWidget(treeW);

	treeW->headerItem()->setText(0, tr("Episode name"));

	m_currentAnime.clear();
}

QString AnimeOdcinki::getQMPlay2Url(const QString &text) const
{
	return QString("%1://{%2}").arg(m_name, getWebpageUrl(text));
}

NetworkReply *AnimeOdcinki::getSearchReply(const QString &text, const qint32 page)
{
	Q_UNUSED(page)
	m_currentAnime.clear();
	for (const auto &animePair : asConst(m_animePairList))
	{
		if (animePair.first == text)
		{
			m_currentAnime = animePair.second;
			break;
		}
	}
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

MediaBrowserCommon::PagesMode AnimeOdcinki::pagesMode() const
{
	return PagesMode::Single;
}

bool AnimeOdcinki::hasWebpage() const
{
	return true;
}
QString AnimeOdcinki::getWebpageUrl(const QString &text) const
{
	return QString("%1%2").arg(g_url, text);
}

MediaBrowserCommon::CompleterMode AnimeOdcinki::completerMode() const
{
	return CompleterMode::All;
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
	for (const auto &animePair : asConst(m_animePairList))
		completions.append(animePair.first);
	return completions;
}
void AnimeOdcinki::setCompleterListCallback(const CompleterReadyCallback &callback)
{
	m_completerListCallback = callback;
	if (m_completerListCallback)
	{
		if (m_animePairList.isEmpty() && !m_animeListReply)
		{
			m_animeListReply = start(g_url);
			connect(m_animeListReply, SIGNAL(finished()), this, SLOT(gotAnimeList()));
		}
		else if (!m_animePairList.isEmpty())
		{
			m_completerListCallback();
			m_completerListCallback = nullptr;
		}
	}
}

QAction *AnimeOdcinki::getAction() const
{
	return nullptr;
}

bool AnimeOdcinki::convertAddress(const QString &prefix, const QString &url, const QString &param, QString *streamUrl, QString *name, QIcon *icon, QString *extension, IOController<> *ioCtrl)
{
	Q_UNUSED(param)

	if (prefix != m_name)
		return false;

	if (icon)
		*icon = m_icon;
	if (ioCtrl && streamUrl)
	{
		NetworkAccess net;
		net.setMaxDownloadSize(0x200000 /* 2 MiB */);

		IOController<NetworkReply> &netReply = ioCtrl->toRef<NetworkReply>();
		if (net.startAndWait(netReply, url))
		{
			const QByteArray reply = netReply->readAll();

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

			bool hasStreamUrl = false;
			QString error;

			const auto getStreamUrl = [&](const QString &animeUrl)->bool {
				return YouTubeDL::fixUrl(animeUrl, *streamUrl, ioCtrl, hasName ? nullptr : name, extension, &error);
			};

			const auto getDownloadButtonUrl = [&](bool allowGDriveRawFile) {
				// Might not work due to linkexpander problems.
				const QByteArray adFlyUrl = getAdFlyUrl(reply).toPercentEncoding();
				if (!adFlyUrl.isEmpty() && net.startAndWait(netReply, g_linkexpander, "url=" + adFlyUrl, NetworkAccess::UrlEncoded, 3))
				{
					QByteArray data = netReply->readAll();
					const int idx = data.indexOf("<");
					if (idx > -1)
					{
						const QString animeUrl = data.left(idx);
						if (!allowGDriveRawFile || !animeUrl.contains("docs.google.com"))
							hasStreamUrl = getStreamUrl(animeUrl);
						else if (net.startAndWait(netReply, animeUrl))
						{
							// Download raw file from Google Drive
							data = netReply->readAll().constData();
							int idx1 = data.indexOf("uc-download-link");
							if (idx1 > -1)
							{
								idx1 = data.indexOf("href=\"", idx1);
								if (idx1 > -1)
								{
									idx1 += 6;
									int idx2 = data.indexOf('"', idx1);
									if (idx2 > -1)
									{
										const QString path = QTextDocumentFragment::fromHtml(data.mid(idx1, idx2 - idx1)).toPlainText();
										if (path.startsWith("/"))
										{
											*streamUrl = "https://docs.google.com" + path;
											QMPlay2Core.addCookies(*streamUrl, netReply->getCookies());
											hasStreamUrl = true;
										}
									}
								}
							}
						}
					}
				}
			};

			if (extension && !ioCtrl->isAborted()) // Download only
			{
				getDownloadButtonUrl(true);
				if (extension->isEmpty())
					*extension = ".mp4"; // Probably all videos here have MP4 file format
			}

			if (!hasStreamUrl && !ioCtrl->isAborted())
			{
				maybeFetchConfiguration(netReply);
				for (const QJsonObject &json : getEmbeddedPlayers(reply))
				{
					QString playerUrl = decryptUrl(json["v"].toString(), json["a"].toString());
					if (!playerUrl.isEmpty())
					{
						if (playerUrl.contains("gamedor.usermd.net") && net.startAndWait(netReply, playerUrl, QByteArray(), "Referer: " + url.toUtf8()))
						{
							playerUrl = getGamedorUsermdUrl(netReply->readAll());
							if (playerUrl.isEmpty())
								continue;
						}
						hasStreamUrl = getStreamUrl(playerUrl);
						if (hasStreamUrl)
							break;
					}
				}
			}

			if (!extension && !hasStreamUrl && !ioCtrl->isAborted()) // Fallback to download button...
				getDownloadButtonUrl(false);

			if (!hasStreamUrl && !error.isEmpty() && !ioCtrl->isAborted())
				emit QMPlay2Core.sendMessage(error, m_name, 3, 0);

			ioCtrl->reset();
		}
	}
	return true;
}

void AnimeOdcinki::maybeFetchConfiguration(IOController<NetworkReply> &netReply)
{
	if (!m_serverPriorities.isEmpty())
		return;

	NetworkAccess net;
	if (net.startAndWait(netReply, g_serverPrioritiesUrl))
	{
		const QJsonArray array = QJsonDocument::fromJson(netReply->readAll()).array();
		for (const QJsonValue &v : array)
		{
			const QJsonObject o = v.toObject();
			const QString name = o["Name"].toString();
			const int priority = o["Priority"].toInt(-1);
			if (!name.isEmpty() && (priority == 0 || priority == 1))
				m_serverPriorities[name] = priority;
		}
		netReply.reset();
	}

	if (m_serverPriorities.isEmpty())
		qCWarning(animeodcinki) << "Can't fetch server priorities";
}
AnimeOdcinki::EmbeddedPlayers AnimeOdcinki::getEmbeddedPlayers(const QByteArray &data) const
{
	EmbeddedPlayers ret;

	for (int pos = 0; ;)
	{
		int idx1 = data.indexOf("data-hash='", pos);
		if (idx1 < 0)
			break;

		idx1 += 11;

		int idx2 = data.indexOf("'", idx1);
		if (idx2 < 0)
			break;

		QJsonDocument json = QJsonDocument::fromJson(data.mid(idx1, idx2 - idx1));
		if (json.isObject())
		{
			idx1 = idx2 + 2;
			idx2 = data.indexOf("<", idx1);
			if (idx2 > -1)
			{
				QByteArray name = data.mid(idx1, idx2 - idx1).simplified().toLower();
				const int idx = name.indexOf(' ');
				if (idx > -1)
					name = name.left(name.indexOf(' '));

				const int priority = m_serverPriorities.value(name, -1);
				if (m_serverPriorities.isEmpty() || priority == 1)
					ret.push_back(json.object());
				else if (priority == 0)
					ret.push_front(json.object());
			}
		}

		pos = idx2;
	}

	return ret;
}

void AnimeOdcinki::gotAnimeList()
{
	NetworkReply *animeListReply = qobject_cast<NetworkReply *>(sender());
	if (animeListReply && m_animeListReply == animeListReply)
	{
		if (!m_animeListReply->hasError())
		{
			m_animePairList = parseAnimeList(m_animeListReply->readAll(), nullptr);
			if (m_completerListCallback)
				m_completerListCallback();
		}
		m_completerListCallback = nullptr;
		m_animeListReply->deleteLater();
	}
}
