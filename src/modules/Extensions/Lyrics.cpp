/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <Lyrics.hpp>

#include <algorithm>
#include <vector>
#include <tuple>

/**/

constexpr const char *g_tekstowoUrl = "http://www.tekstowo.pl/";

static inline QString getTekstowoSearchUrl(const QString &artist, const QString &title)
{
    return QString("%1szukaj,wykonawca,%2,tytul,%3.html").arg(g_tekstowoUrl, artist.toUtf8().toPercentEncoding(), title.toUtf8().toPercentEncoding());
}
static inline QString getMakeitpersonalUrl(const QString &artist, const QString &title)
{
    return QString("https://makeitpersonal.co/lyrics?artist=%1&title=%2").arg((QString)artist.toUtf8().toPercentEncoding()).arg((QString)title.toUtf8().toPercentEncoding());
}

static QString simplifyString(const QString &str)
{
    QString ret = str;
    for (int i = ret.length() - 1; i >= 0; --i)
    {
        const QChar chr = ret.at(i);
        if (chr == '-')
            ret[i] = ' ';
        else if (chr.isMark() || chr.isPunct() || chr.isSymbol())
            ret.remove(i, 1);
    }
    return ret.simplified().toLower();
}

/**/

Lyrics::Lyrics(Module &module) :
    m_visible(false), m_pending(false)
{
    SetModule(module);

    connect(&QMPlay2Core, &QMPlay2CoreClass::updatePlaying,
            this, &Lyrics::updatePlaying);
    connect(&m_net, SIGNAL(finished(NetworkReply *)), this, SLOT(finished(NetworkReply *)));

    m_dW = new DockWidget;
    connect(m_dW, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));
    m_dW->setWindowTitle(tr("Lyrics"));
    m_dW->setObjectName(LyricsName);
    m_dW->setWidget(this);

    setReadOnly(true);
}
Lyrics::~Lyrics()
{}

DockWidget *Lyrics::getDockWidget()
{
    return m_dW;
}

void Lyrics::visibilityChanged(bool v)
{
    m_visible = v;
    if (m_visible && m_pending)
        search();
}

void Lyrics::updatePlaying(bool play, const QString &title, const QString &artist, const QString &album, int length, bool needCover, const QString &fileName, const QString &lyrics)
{
    Q_UNUSED(album)
    Q_UNUSED(length)
    Q_UNUSED(needCover)
    Q_UNUSED(fileName)

    m_pending = false;
    if (m_tekstowoSearchReply)
        m_tekstowoSearchReply->deleteLater();
    if (m_tekstowoLyricsReply)
        m_tekstowoLyricsReply->deleteLater();
    m_realTitle.clear();
    m_realArtist.clear();
    m_title.clear();
    m_artist.clear();
    m_name.clear();
    clear();

    if (play)
    {
        if (!lyrics.isEmpty())
        {
            QString html = "<center>";
            if (!title.isEmpty() && !artist.isEmpty())
                html += "<b>" + title + " - " + artist + "</b><br/><br/>";
            html += QString(lyrics).replace("\n", "<br/>") + "</center>";
            setHtml(html);
            return;
        }

        m_realTitle = title;
        m_realArtist = artist;
        m_title  = simplifyString(title);
        m_artist = simplifyString(artist);
        search();
    }
}

void Lyrics::finished(NetworkReply *reply)
{
    const QByteArray data = reply->readAll();
    reply->deleteLater();
    if (reply->hasError())
    {
        if (reply == m_tekstowoSearchReply || reply == m_tekstowoLyricsReply)
            m_makeitpersonalLyricsReply = m_net.start(getMakeitpersonalUrl(m_artist, m_title));
        return;
    }
    if (reply == m_tekstowoSearchReply)
    {
        int idx1 = data.indexOf("class=\"content\"");
        int idx2 = data.indexOf("class=\"t-artists\"", idx1);
        if (idx1 > -1 && (idx2 > idx1 || idx2 < 0))
        {
            const auto extractTag = [](const QString &data, const QString &tag) {
                int idx1 = data.indexOf(tag + "=\"");
                if (idx1 > -1)
                {
                    idx1 += tag.length() + 2;
                    int idx2 = data.indexOf("\"", idx1);
                    if (idx2 > -1)
                        return data.mid(idx1, idx2 - idx1);
                }
                return QString();
            };

            QStringList list = QString(data.mid(idx1, (idx2 < 0) ? -1 : (idx2 - idx1))).split("class=\"box-przeboje\"");
            list.removeFirst();

            using Guess = std::tuple<QString, QString, quint8>;
            std::vector<Guess> guesses;

            for (const QString &chunk : qAsConst(list))
            {
                const QString name = extractTag(chunk, "title");

                const QStringList artistTitle = name.split(" - ");
                if (artistTitle.count() != 2)
                    continue;
                const QString artist = simplifyString(artistTitle[0]);
                const QString title  = simplifyString(artistTitle[1]);

                quint8 artistScore = 0, titleScore = 0;

                if (artist == m_artist)
                    artistScore += 4;
                else if (m_artist.contains(artist))
                    artistScore += 3;
                else if (artist.contains(m_artist))
                    artistScore += 2;
                else
                {
                    // Compare all words separately - useful if artist words are in different order
                    const QStringList words = artist.split(' ');
                    if (words.count() > 1)
                    {
                        int matchWords = 0;
                        for (const QString &word : words)
                        {
                            if (m_artist.contains(word))
                                ++matchWords;
                        }
                        if (matchWords == words.count())
                            artistScore += 1;
                    }
                }

                if (title == m_title)
                    titleScore += 4;
                else if (m_title.contains(title))
                    titleScore += 3;
                else if (title.contains(m_title))
                    titleScore += 2;

                if (artistScore > 0 && titleScore > 0)
                {
                    guesses.emplace_back
                    (
                        name,
                        extractTag(chunk, "href").remove(0, 1).replace("piosenka", "drukuj"),
                        artistScore + titleScore
                    );
                }
            }

            if (guesses.empty())
            {
                m_makeitpersonalLyricsReply = m_net.start(getMakeitpersonalUrl(m_artist, m_title));
            }
            else
            {
                std::sort(guesses.begin(), guesses.end(), [](const Guess &a, const Guess &b)->bool {
                    return (std::get<2>(a) > std::get<2>(b));
                });

                m_tekstowoLyricsReply = m_net.start(g_tekstowoUrl + std::get<1>(guesses[0]));
                m_name = std::get<0>(guesses[0]);
            }
        }
    }
    else if (reply == m_tekstowoLyricsReply)
    {
        const auto getLyrics = [data](const QString &type)->QString {
            int idx1 = data.indexOf("class=\"" + type + "\"");
            if (idx1 > -1)
            {
                idx1 = data.indexOf("</h2>", idx1);
                if (idx1 > -1)
                {
                    int idx2 = data.indexOf("</div>", idx1);
                    if (idx2 > -1)
                        return data.mid(idx1, idx2 - idx1);
                }
            }
            return QString();
        };

        const QString lyrics = getLyrics("song-text");
        if (!lyrics.isEmpty())
        {
            const QString translated = getLyrics("tlumaczenie");
            QString html = "<center><b><big>" + m_name + "</big></b><br/>" + lyrics;
            if (!translated.isEmpty())
                html += "<br/><br/><b><big>" + tr("Translation") + "</big></b><br/>" + translated;
            html += "</center>";
            setHtml(html);
        }
        else
        {
            lyricsNotFound();
        }
    }
    else if (reply == m_makeitpersonalLyricsReply)
    {
        if (data != "Sorry, We don't have lyrics for this song yet.")
            setHtml("<center><b>" + m_realTitle + " - " + m_realArtist + "</b><br/>" + QByteArray(data).replace("\n", "<br/>") + "</center>");
        else
            lyricsNotFound();
    }
}

void Lyrics::search()
{
    if (m_title.isEmpty() || m_artist.isEmpty())
        return;

    if (!m_visible)
        m_pending = true;
    else
    {
        m_tekstowoSearchReply = m_net.start(getTekstowoSearchUrl(m_artist, m_title));
        m_pending = false;
    }
}

void Lyrics::lyricsNotFound()
{
    setHtml(QString("<center><i>%1</i></center>").arg(tr("Lyrics not found")));
}
