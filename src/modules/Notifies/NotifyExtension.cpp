/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

#ifdef Q_OS_LINUX
	#include <X11Notify.hpp>
#endif
#include <TrayNotify.hpp>

#include <QCoreApplication>

NotifyService::NotifyService(Notify *notify, Settings &settings) :
	m_notify(notify)
{
	Q_ASSERT(notify);
	if (settings.getBool("ShowTitle"))
		connect(&QMPlay2Core, SIGNAL(updatePlaying(bool, const QString &, const QString &, const QString &, int, bool, const QString &)), this, SLOT(updatePlaying(bool, const QString &, const QString &, const QString &, int, bool, const QString &)));
	if (settings.getBool("ShowPlayState"))
		connect(&QMPlay2Core, SIGNAL(playStateChanged(const QString &)), this, SLOT(playStateChanged(const QString &)));
	if (settings.getBool("ShowVolume"))
		connect(&QMPlay2Core, SIGNAL(volumeChanged(double)), this, SLOT(volumeChanged(double)));

	if (settings.getBool("CustomMsg"))
	{
		m_summaryFormat = settings.getString("CustomSummary");
		m_bodyFormat = settings.getString("CustomBody");
	}
}

NotifyService::~NotifyService()
{
	delete m_notify;
}

void NotifyService::volumeChanged(double v)
{
	m_notify->showMessage(tr("Volume Changed"), tr("Volume: %1%").arg((int)(100 * v)));
}

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

	m_notify->showMessage(summary, body);
}

void NotifyService::playStateChanged(const QString &playState)
{
	if (playState != "Playing")
		m_notify->showMessage(QCoreApplication::applicationName(), playState);
}

/**/

NotifyExtension::NotifyExtension(Module &module) : m_notify(NULL)
{
	SetModule(module);
}

NotifyExtension::~NotifyExtension()
{
	delete m_notify;
}

bool NotifyExtension::set()
{
	if (sets().getBool("TypeDisabled"))
		delete m_notify;
	else
	{
		const int timeout = sets().getInt("timeout");
		Notify *notify = NULL;
		if (sets().getBool("TypeTray"))
			notify = new TrayNotify(timeout);
#ifdef Q_OS_LINUX
		else if (sets().getBool("TypeNative"))
			notify = new X11Notify(timeout);
#endif
		if (notify)
		{
			delete m_notify;
			m_notify = new NotifyService(notify, sets());
		}
	}

	return true;
}
