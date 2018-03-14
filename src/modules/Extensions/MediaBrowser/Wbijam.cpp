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

#include <MediaBrowser/Wbijam.hpp>

#include <NetworkAccess.hpp>
#include <Functions.hpp>
#include <YouTubeDL.hpp>

#include <QTextDocumentFragment>
#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTreeWidget>
#include <QHeaderView>
#include <QJsonArray>
#include <QRegExp>

#include <algorithm>

Q_LOGGING_CATEGORY(wbijam, "Wbijam")

constexpr const char *g_serverPrioritiesUrl = "https://raw.githubusercontent.com/zaps166/QMPlay2OnlineContents/master/Wbijam.json";
constexpr const char *g_inneUrl = "http://www.inne.wbijam.pl/";

static inline QString getName(const Wbijam::AnimeTuple &tuple)
{
	return std::get<0>(tuple);
}
static inline QString getUrl(const Wbijam::AnimeTuple &tuple)
{
	return std::get<1>(tuple);
}
static inline bool isPolecane(const Wbijam::AnimeTuple &tuple)
{
	return std::get<2>(tuple);
}

static Wbijam::AnimeTupleList parseAnimeList(const QString &data, const QString &baseUrl, const bool insidePolecane)
{
	Wbijam::AnimeTupleList animeTupleList;

	const auto getAnimeList = [&](const QString &sectionName, const bool polecane) {
		QRegExp sectionRx(QString(">%1<.*</ul>").arg(sectionName));
		sectionRx.setMinimal(true);
		if (sectionRx.indexIn(data) != -1)
		{
			const QString section = sectionRx.cap();

			QRegExp rx("<li>.*<a href=\"(.*)\">(.*)</a>.*</li>");
			rx.setMinimal(true);

			int pos = 0;
			while ((pos = rx.indexIn(section, pos)) != -1)
			{
				QString url = rx.cap(1);
				if (!polecane)
					url.prepend(baseUrl);
				if (!polecane || url != baseUrl)
					animeTupleList.emplace_back(rx.cap(2), url, polecane);
				pos += rx.matchedLength();
			}
		}
	};

	if (insidePolecane)
	{
		getAnimeList("Odcinki anime online", false);
	}
	else
	{
		getAnimeList("Akcja", false);
		getAnimeList("Lżejsze klimaty", false);
		getAnimeList("Polecane serie anime", true);

		std::sort(animeTupleList.begin(), animeTupleList.end(), [](const Wbijam::AnimeTuple &a, const Wbijam::AnimeTuple &b) {
			return getName(a) < getName(b);
		});
	}

	return animeTupleList;
}

/**/

Wbijam::Wbijam(NetworkAccess &net) :
	MediaBrowserCommon(net, "Wbijam", ":/video.svgz"),
	m_treeW(nullptr),
	m_tupleIdx(-1),
	m_page(-1)
{}
Wbijam::~Wbijam()
{}

void Wbijam::prepareWidget(QTreeWidget *treeW)
{
	m_treeW = treeW;

	MediaBrowserCommon::prepareWidget(m_treeW);

	m_treeW->headerItem()->setText(0, tr("Episode name"));
	m_treeW->headerItem()->setText(1, tr("Episode type"));
	m_treeW->headerItem()->setText(2, tr("Episode date"));

	m_treeW->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	m_treeW->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

	m_polecaneSeries.clear();
	m_innePages.clear();
	m_tupleIdx = -1;
	m_page = -1;
}

QString Wbijam::getQMPlay2Url(const QString &text) const
{
	return QString("%1://%2").arg(m_name, text);
}

NetworkReply *Wbijam::getSearchReply(const QString &text, const qint32 page)
{
	if (page < 1)
		return nullptr;

	m_polecaneSeries.clear();
	m_page = page - 1;

	const qint32 lastTupleIdx = m_tupleIdx;

	m_tupleIdx = 0;
	for (const AnimeTuple &tuple : asConst(m_animeTupleList))
	{
		const QString &name = getName(tuple);
		const QString &url  = getUrl(tuple);
		if (name == text)
		{
			m_treeW->setColumnCount(isPolecane(tuple) ? 3 : 1);

			if (m_innePages.empty() || lastTupleIdx != m_tupleIdx)
			{
				m_innePages.clear();
				return m_net.start(url);
			}
			return nullptr;
		}
		++m_tupleIdx;
	}

	m_tupleIdx = -1;
	return nullptr;
}
MediaBrowserCommon::Description Wbijam::addSearchResults(const QByteArray &reply, QTreeWidget *treeW)
{
	if (m_tupleIdx < 0)
		return {};

	const AnimeTuple &animeTuple = m_animeTupleList[m_tupleIdx];
	const QString url = getUrl(animeTuple);
	const QIcon wbijamIcon = icon();
	QString replyStr = reply;

	if (isPolecane(animeTuple))
	{
		if (m_polecaneSeries.empty())
		{
			m_polecaneSeries = parseAnimeList(replyStr, url, true);
			if ((qint32)m_polecaneSeries.size() > m_page)
				return {m_net.start(getUrl(m_polecaneSeries[m_page]))};
		}
		else
		{
			QRegExp rx("<td><a href=\"(.*)\">.*\"\">(.*)</a></td>.*<td.+>(.*)</td>.*<td.+>(.*)</td>");
			rx.setMinimal(true);

			int pos = 0;
			while ((pos = rx.indexIn(replyStr, pos)) != -1)
			{
				QTreeWidgetItem *tWI = new QTreeWidgetItem(treeW);

				tWI->setText(0, QTextDocumentFragment::fromHtml(rx.cap(2)).toPlainText().simplified());
				tWI->setData(0, Qt::UserRole, QString("{%1%2}").arg(url, rx.cap(1)));
				tWI->setIcon(0, wbijamIcon);

				tWI->setText(1, rx.cap(3));

				tWI->setData(2, Qt::DisplayRole, QDate::fromString(rx.cap(4), "d.M.yyyy"));

				pos += rx.matchedLength();
			}
		}
	}
	else
	{
		Description description;
		if (!reply.isEmpty())
		{
			QRegExp mainRx("<img src=\"grafika/(.+)\">");
			mainRx.setMinimal(true);
			int pos = mainRx.indexIn(replyStr);
			if (pos != -1)
			{
				const bool hasHeaders = (replyStr.indexOf("pod_naglowek", pos) > -1);

				QRegExp sectionRx(QString("%1%2").arg(hasHeaders ? "<p class=\"pod_naglowek\">(.*)</p>.*" : QString(), "<tbody>(.*)</tbody>"));
				sectionRx.setMinimal(true);

				m_innePages.clear();

				while ((pos = sectionRx.indexIn(replyStr, pos)) != -1)
				{
					QString title;
					if (hasHeaders)
					{
						title = sectionRx.cap(1).simplified();
						if (title.endsWith(':'))
							title.chop(1);
					}
					else
					{
						title = getName(animeTuple);
					}

					AnimeInneList animeInneList;

					{
						const QString section = sectionRx.cap(hasHeaders ? 2 : 1);

						QRegExp rx("<td>.*>(.*)</td>");
						rx.setMinimal(true);

						int pos = 0;
						while ((pos = rx.indexIn(section, pos)) != -1)
						{
							QString title = rx.cap(1).simplified();
							if (title.endsWith('.'))
								title.chop(1);

							animeInneList.append(title);

							pos += rx.matchedLength();
						}
					}

					m_innePages.append({title, animeInneList});

					pos += sectionRx.matchedLength();
				}

				{
					QRegExp descRx("(<strong>.*</strong>.*)<br");
					descRx.setMinimal(true);

					int pos = 0;
					while ((pos = descRx.indexIn(replyStr, pos)) != -1)
					{
						description.description += descRx.cap(1) + "<br/>";
						pos += descRx.matchedLength();
					}
					description.imageReply = m_net.start(QString("%1grafika/%2").arg(g_inneUrl, mainRx.cap(1)));
				}
			}
		}
		for (const QString &title : m_innePages.value(m_page).second)
		{
			QTreeWidgetItem *tWI = new QTreeWidgetItem(treeW);

			tWI->setText(0, title);
			tWI->setData(0, Qt::UserRole, QString("{%1}%2").arg(url, QString(QByteArray(title.toUtf8()).toBase64())));
			tWI->setIcon(0, wbijamIcon);
		}
		return description;
	}

	return {};
}

MediaBrowserCommon::PagesMode Wbijam::pagesMode() const
{
	return PagesMode::List;
}
QStringList Wbijam::getPagesList() const
{
	QStringList list;
	for (const AnimeTuple &tuple : m_polecaneSeries)
		list += getName(tuple);
	for (const AnimeInnePage &page : m_innePages)
		list += page.first;
	return list;
}

bool Wbijam::hasWebpage() const
{
	return true;
}
QString Wbijam::getWebpageUrl(const QString &text) const
{
	return text.mid(1, text.indexOf('}') - 1);
}

MediaBrowserCommon::CompleterMode Wbijam::completerMode() const
{
	return CompleterMode::All;
}
NetworkReply *Wbijam::getCompleterReply(const QString &text)
{
	Q_UNUSED(text)
	return nullptr;
}
QStringList Wbijam::getCompletions(const QByteArray &reply)
{
	Q_UNUSED(reply)
	QStringList completions;
	for (const AnimeTuple &tuple : asConst(m_animeTupleList))
		completions += getName(tuple);
	return completions;
}
void Wbijam::setCompleterListCallback(const CompleterReadyCallback &callback)
{
	m_completerListCallback = callback;
	if (m_completerListCallback)
	{
		if (m_animeTupleList.empty() && !m_animeListReply)
		{
			m_animeListReply = start(g_inneUrl);
			connect(m_animeListReply.data(), &NetworkReply::finished, this, &Wbijam::gotAnimeList);
		}
		else if (!m_animeTupleList.empty())
		{
			m_completerListCallback();
			m_completerListCallback = nullptr;
		}
	}
}

QAction *Wbijam::getAction() const
{
	return nullptr;
}

bool Wbijam::convertAddress(const QString &prefix, const QString &url, const QString &param, QString *streamUrl, QString *name, QIcon *icon, QString *extension, IOController<> *ioCtrl)
{
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
			const QString replyData = netReply->readAll();

			bool hasName = false;
			QString error;

			const auto getStreamUrl = [&](const QString &animeUrl)->bool {
				return YouTubeDL::fixUrl(animeUrl, *streamUrl, ioCtrl, hasName ? nullptr : name, extension, &error);
			};

			maybeFetchConfiguration(netReply);

			int maxPriority = 0;
			for (auto it = m_serverPriorities.constBegin(), itEnd = m_serverPriorities.constEnd(); it != itEnd; ++it)
				maxPriority = qMax(maxPriority, it.value());

			std::vector<std::vector<std::tuple<QString, bool>>> videoPriorityUrls(maxPriority + 1);

			const auto extractUrl = [&](const QString section) {
				int idx1 = section.indexOf("a href=\"");
				if (idx1 < 0)
				{
					const int priority = m_serverPriorities.isEmpty() ? 0 : m_serverPriorities.value("vk", -1);
					if (priority == -1)
						return;
					int idxRel = section.indexOf("rel=\"");
					int idxId  = section.indexOf("id=\"");
					if (idxRel > -1 && idxId > -1)
					{
						idxRel += 5;
						idxId  += 4;
						int idxRelEnd = section.indexOf('"', idxRel);
						int idxIdEnd  = section.indexOf('"', idxId);
						if (idxRelEnd > -1 && idxIdEnd > -1)
							videoPriorityUrls[priority].emplace_back(QString("https://vk.com/video%1_%2").arg(section.mid(idxRel, idxRelEnd - idxRel), section.mid(idxId, idxIdEnd - idxId)), true);
					}
				}
				else
				{
					idx1 += 8;

					int idx2 = section.indexOf('"', idx1);
					if (idx2 > -1)
					{
						const QString urlSection = section.mid(idx1, idx2 - idx1);
						const QString urlDest = url.left(url.indexOf("wbijam.pl/") + 10) + urlSection;
						if (m_serverPriorities.isEmpty())
						{
							videoPriorityUrls[0].emplace_back(urlDest, false);
						}
						else for (auto it = m_serverPriorities.constBegin(), itEnd = m_serverPriorities.constEnd(); it != itEnd; ++it)
						{
							if (urlSection.contains(it.key()))
							{
								videoPriorityUrls[it.value()].emplace_back(urlDest, false);
							}
						}
					}
				}
			};

			bool hasStreamUrl = false;

			const auto processUrls = [&] {
				for (const auto &videoUrls : videoPriorityUrls)
				{
					for (const auto &tuple : videoUrls)
					{
						QString animeUrl = std::get<0>(tuple);
						bool ok = std::get<1>(tuple);

						if (!ok) // iframe
						{
							if (net.startAndWait(netReply, animeUrl))
							{
								const QString replyData = netReply->readAll();

								QRegExp videoRx("<iframe.+src=\"(.*)\"");
								videoRx.setMinimal(true);
								int pos = 0;
								while ((pos = videoRx.indexIn(replyData, pos)) != -1)
								{
									QString url = videoRx.cap(1);
									if (url.startsWith("//"))
										url.prepend("http:");
									if (!url.contains("facebook.com/"))
									{
										animeUrl = url;
										ok = true;
										break;
									}
									pos += videoRx.matchedLength();
								}
							}
						}

						if (ok && getStreamUrl(animeUrl))
						{
							hasStreamUrl = true;
							return;
						}
					}
				}
			};

			if (param.isEmpty()) // Polecane
			{
				if (name)
				{
					QRegExp nameRx("<p class=\"pod_naglowek\">(.*)</p>");
					nameRx.setMinimal(true);
					if (nameRx.indexIn(replyData) != -1)
					{
						QString tmpName = nameRx.cap(1).simplified();
						if (tmpName.endsWith(':'))
							tmpName.chop(1);
						if (!tmpName.isEmpty())
						{
							*name = tmpName;
							hasName = true;
						}
					}
				}

				QRegExp videoRx("<tr class=\"lista_hover\">(.*)</tr>");
				videoRx.setMinimal(true);

				int pos = 0;
				while ((pos = videoRx.indexIn(replyData, pos)) != -1)
				{
					extractUrl(videoRx.cap(1));
					pos += videoRx.matchedLength();
				}

				processUrls();
			}
			else // Inne
			{
				const QString epName = QByteArray::fromBase64(param.toLatin1());
				if (name)
				{
					*name = epName;
					hasName = true;
				}

				int idx1 = replyData.indexOf(epName);
				if (idx1 > -1)
				{
					idx1 += epName.length();

					int idx2 = replyData.indexOf("</tr>", idx1);
					if (idx2 > -1)
					{
						QRegExp videoRx("<td.*>(.*)</td>");
						videoRx.setMinimal(true);

						const QString section = replyData.mid(idx1, idx2 - idx1);

						int pos = 0;
						while ((pos = videoRx.indexIn(section, pos)) != -1)
						{
							extractUrl(videoRx.cap(1));
							pos += videoRx.matchedLength();
						}

						processUrls();
					}
				}
			}

			if (extension && !ioCtrl->isAborted() && extension->isEmpty())
				*extension = ".mp4"; // Probably all videos here have MP4 file format

			if (!hasStreamUrl && !error.isEmpty() && !ioCtrl->isAborted())
				emit QMPlay2Core.sendMessage(error, m_name, 3, 0);

			ioCtrl->reset();
		}
	}
	return true;
}

void Wbijam::maybeFetchConfiguration(IOController<NetworkReply> &netReply)
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
			if (!name.isEmpty() && priority >= 0)
				m_serverPriorities[name] = qMin(priority, 9);
		}
		netReply.reset();
	}

	if (m_serverPriorities.isEmpty())
		qCWarning(wbijam) << "Can't fetch server priorities";
}

void Wbijam::gotAnimeList()
{
	NetworkReply *animeListReply = qobject_cast<NetworkReply *>(sender());
	if (animeListReply && m_animeListReply == animeListReply)
	{
		if (!m_animeListReply->hasError())
		{
			m_animeTupleList = parseAnimeList(m_animeListReply->readAll(), g_inneUrl, false);
			if (m_completerListCallback)
				m_completerListCallback();
		}
		m_completerListCallback = nullptr;
		m_animeListReply->deleteLater();
	}
}
