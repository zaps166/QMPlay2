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

#include <NotifyExtension.hpp>

#include <Notifies.hpp>

#include <QCoreApplication>
#include <QFile>

constexpr const char *g_playState[3] = {
    QT_TRANSLATE_NOOP("NotifyService", "Stopped"),
    QT_TRANSLATE_NOOP("NotifyService", "Playing"),
    QT_TRANSLATE_NOOP("NotifyService", "Paused")
};

NotifyService::NotifyService(Settings &settings) :
    m_lastPlayState(g_playState[0]),
    m_timeout(settings.getInt("Timeout"))
{
    if (settings.getBool("ShowTitle"))
    {
        connect(&QMPlay2Core, SIGNAL(updatePlaying(bool, QString, QString, QString, int, bool, QString)), this, SLOT(updatePlaying(bool, QString, QString, QString, int, bool, QString)));

        //"coverDataFromMediaFile()" and "coverFile()" are emited before "updatePlaying()"
        connect(&QMPlay2Core, SIGNAL(coverDataFromMediaFile(QByteArray)), this, SLOT(coverDataFromMediaFile(QByteArray)));
        connect(&QMPlay2Core, SIGNAL(coverFile(QString)), this, SLOT(coverFile(QString)));
    }
    if (settings.getBool("ShowPlayState"))
        connect(&QMPlay2Core, SIGNAL(playStateChanged(QString)), this, SLOT(playStateChanged(QString)));
    if (settings.getBool("ShowVolume"))
        connect(&QMPlay2Core, SIGNAL(volumeChanged(double)), this, SLOT(volumeChanged(double)));

    if (settings.getBool("CustomMsg"))
    {
        m_summaryFormat = settings.getString("CustomSummary");
        m_bodyFormat = settings.getString("CustomBody");
    }
}
NotifyService::~NotifyService()
{}

void NotifyService::updatePlaying(bool play, const QString &title, const QString &artist, const QString &album, int, bool, const QString &fileName)
{
    if (!play)
        return;

    QString summary = m_summaryFormat;
    if (summary.isEmpty())
    {
        summary = title;
        if (!title.isEmpty() && !artist.isEmpty())
            summary += " - ";
        summary += artist;
    }
    else
        summary.replace("%title%", title).replace("%artist%", artist).replace("%album%", album).replace("%filename%", fileName);

    if (summary.isEmpty())
        summary = fileName;

    QString body = m_bodyFormat;
    if (!body.isEmpty())
        body.replace("%title%", title).replace("%artist%", artist).replace("%album%", album).replace("%filename%", fileName);

    if (body.isEmpty())
        body = album;

    QImage coverImage;
    if (!m_cover.isEmpty())
    {
        coverImage = QImage::fromData(m_cover);
        m_cover.clear();
    }

    Notifies::notify(summary, body, m_timeout, coverImage, 1);
}
void NotifyService::coverDataFromMediaFile(const QByteArray &cover)
{
    m_cover = cover;
}
void NotifyService::coverFile(const QString &fileName)
{
    QFile f(fileName);
    if (f.open(QFile::ReadOnly))
        m_cover = f.readAll();
}

void NotifyService::playStateChanged(const QString &playState)
{
    /* In those cases we don't show notification:
     *  1. The last one is the same as the current one
     *  2. The current one is Playing and the last one wasn't Paused
    */
    if (playState != m_lastPlayState && (playState != g_playState[1] || m_lastPlayState == g_playState[2]))
        Notifies::notify(QCoreApplication::applicationName(), tr(playState.toUtf8()), m_timeout, 1);
    m_lastPlayState = playState;
}
void NotifyService::volumeChanged(double v)
{
    Notifies::notify(tr("Volume changed"), tr("Volume: %1%").arg((int)(100 * v)), m_timeout, 1);
}

/**/

NotifyExtension::NotifyExtension(Module &module)
{
    SetModule(module);
}
NotifyExtension::~NotifyExtension()
{}

bool NotifyExtension::set()
{
    if (sets().getBool("Enabled"))
        m_notifyService.reset(new NotifyService(sets()));
    else
        m_notifyService.reset();
    return true;
}
