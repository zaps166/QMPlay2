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

#include <TrayNotify.hpp>
#include <Module.hpp>
#include <QMPlay2Core.hpp>

#include <QApplication>
#include <QImage>
#include <QSystemTrayIcon>

TrayNotify::TrayNotify(qint32 timeout) :
	Notify(timeout)
{}

bool TrayNotify::showMessage(const QString &summary, const QString &message, const QString &, const QImage &)
{
	QSystemTrayIcon *tray = QMPlay2Core.systemTray;
	if (!tray || !QSystemTrayIcon::supportsMessages())
		return false;

	if (m_timeout > 0)
		tray->showMessage(summary, message, QSystemTrayIcon::Information, m_timeout);
	else
		tray->showMessage(summary, message);
	return true;
}
