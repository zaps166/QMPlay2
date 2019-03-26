/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

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

#include <MediaBrowser/MyFreeMp3.hpp>

#include <QMPlay2Extensions.hpp>
#include <NetworkAccess.hpp>
#include <Functions.hpp>

#include <QLoggingCategory>
#include <QJsonDocument>
#include <QTextDocument>
#include <QJsonObject>
#include <QHeaderView>
#include <QTreeWidget>
#include <QJsonArray>
#include <QAction>

Q_LOGGING_CATEGORY(myfreemp3, "MyFreeMp3")

constexpr const char *g_streamHostContainer = "https://raw.githubusercontent.com/zaps166/QMPlay2OnlineContents/master/MyFreeMp3";
constexpr const char *g_url = "https://my-free-mp3s.com/api";

/**/

MyFreeMP3::MyFreeMP3(NetworkAccess &net) :
    MediaBrowserCommon(net, "MyFreeMP3", ":/applications-multimedia.svgz")
{}
MyFreeMP3::~MyFreeMP3()
{}

void MyFreeMP3::prepareWidget(QTreeWidget *treeW)
{
    MediaBrowserCommon::prepareWidget(treeW);

    treeW->sortByColumn(0, Qt::AscendingOrder);

    treeW->headerItem()->setText(0, tr("Title"));
    treeW->headerItem()->setText(1, tr("Artist"));
    treeW->headerItem()->setText(2, tr("Length"));

    treeW->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
}
void MyFreeMP3::finalize()
{
    for (const QString &url : asConst(m_urlNames))
        QMPlay2Core.addNameForUrl(url, QString());
    m_urlNames.clear();
}

QString MyFreeMP3::getQMPlay2Url(const QString &text) const
{
    return QString("%1://{%2}").arg(m_name, text);
}

NetworkReply *MyFreeMP3::getSearchReply(const QString &text, const qint32 page)
{
    return m_net.start(QString("%1/search.php").arg(g_url), QString("q=%1&page=%2").arg(QString(text.toUtf8().toPercentEncoding())).arg(page - 1).toUtf8(), NetworkAccess::UrlEncoded);
}
MediaBrowserCommon::Description MyFreeMP3::addSearchResults(const QByteArray &reply, QTreeWidget *treeW)
{
    const QIcon MyFreeMP3Icon = icon();

    const int replyOffset = (reply.startsWith('(') ? 1 : 0);
    const int replyLength = reply.size() - replyOffset - (reply.endsWith(");") ? 2 : 0);
    const QByteArray replyJson = QByteArray::fromRawData(reply.constData() + replyOffset, replyLength);

    const QJsonArray jsonArray = QJsonDocument::fromJson(replyJson).object()["response"].toArray();

    for (int i = 0; i < jsonArray.count(); ++i)
    {
        const QJsonObject entry = jsonArray[i].toObject();
        if (entry.isEmpty())
            continue;

        const QString id = encode(entry["owner_id"].toInt()) + ":" + encode(entry["id"].toInt());

        const QString title = entry["title"].toString();
        const QString artist = entry["artist"].toString();
        const QString fullName = artist + " - " + title;

        QTreeWidgetItem *tWI = new QTreeWidgetItem(treeW);
        tWI->setData(0, Qt::UserRole + 1, fullName);
        tWI->setData(0, Qt::UserRole, id);
        tWI->setIcon(0, MyFreeMP3Icon);

        tWI->setText(0, title);
        tWI->setToolTip(0, tWI->text(0));

        tWI->setText(1, artist);
        tWI->setToolTip(1, tWI->text(1));

        tWI->setText(2, Functions::timeToStr(entry["duration"].toInt()));

        QMPlay2Core.addNameForUrl(getQMPlay2Url(id), fullName, false);
        m_urlNames.append(id);
    }

    return {};
}

MediaBrowserCommon::PagesMode MyFreeMP3::pagesMode() const
{
    return PagesMode::Multi;
}

bool MyFreeMP3::hasWebpage() const
{
    return false;
}
QString MyFreeMP3::getWebpageUrl(const QString &text) const
{
    Q_UNUSED(text)
    return QString();
}

MediaBrowserCommon::CompleterMode MyFreeMP3::completerMode() const
{
    return CompleterMode::Continuous;
}
NetworkReply *MyFreeMP3::getCompleterReply(const QString &text)
{
    return m_net.start(QString("%1/autocomplete.php").arg(g_url), "query=" + text.toUtf8().toPercentEncoding(), NetworkAccess::UrlEncoded);
}
QStringList MyFreeMP3::getCompletions(const QByteArray &reply)
{
    const QJsonArray jsonArray = QJsonDocument::fromJson(reply).array();
    QStringList completions;
    for (int i = 0; i < jsonArray.count(); ++i)
    {
        const QString name = jsonArray[i].toObject()["name"].toString();
        if (!name.isEmpty())
            completions += name;
    }
    return completions;
}

QAction *MyFreeMP3::getAction() const
{
    QAction *act = new QAction(tr("Search on MyFreeMP3"), nullptr);
    act->setIcon(icon());
    return act;
}

bool MyFreeMP3::convertAddress(const QString &prefix, const QString &url, const QString &param, QString *streamUrl, QString *name, QIcon *icon, QString *extension, IOController<> *ioCtrl)
{
    Q_UNUSED(param)
    Q_UNUSED(name)

    if (prefix != m_name)
        return false;

    if (streamUrl || icon)
    {
#if 0 // Icon needed
        if (icon)
            *icon = m_icon;
#endif

        if (extension)
            *extension = ".mp3";

        if (ioCtrl && streamUrl)
        {
            if (m_streamHost.isEmpty())
            {
                auto &netReply = ioCtrl->toRef<NetworkReply>();
                NetworkAccess net;
                if (net.startAndWait(netReply, g_streamHostContainer))
                {
                    m_streamHost = netReply->readAll().trimmed();
                    netReply.reset();
                }
                else
                {
                    qCWarning(myfreemp3) << "Can't obtain host stream url";
                }
            }
            if (!m_streamHost.isEmpty())
                *streamUrl = "https://" + m_streamHost + "/stream/" + url;
        }
    }

    return true;
}

QString MyFreeMP3::encode(int input)
{
    const QString map = "ABCDEFGHJKMNPQRSTUVWXYZabcdefghjkmnpqrstuvxyz123";

    if (input == 0)
        return map.mid(0, 1);

    const int length = map.length();
    QString encoded;

    if (input < 0)
    {
        input *= - 1;
        encoded += "-";
    };

    while (input > 0)
    {
        const int idx = (input % length);
        input = (input / length);
        encoded += map[idx];
    }

    return encoded;
}
