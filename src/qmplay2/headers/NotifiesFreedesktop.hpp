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

#pragma once

#include <Notifies.hpp>

#include <QDateTime>
#include <QObject>

class OrgFreedesktopNotificationsInterface;
class QDBusPendingCallWatcher;

class QMPLAY2SHAREDLIB_EXPORT NotifiesFreedesktop final : public QObject, public Notifies
{
	Q_OBJECT

public:
	NotifiesFreedesktop();
	~NotifiesFreedesktop();

private:
	bool doNotify(const QString &title, const QString &message, const int ms, const QPixmap &pixmap, const int iconId = 0) override;
	bool doNotify(const QString &title, const QString &message, const int ms, const QImage &image, const int iconId = 0) override;

private slots:
	void callFinished(QDBusPendingCallWatcher *watcher);

private:
	OrgFreedesktopNotificationsInterface *m_interface;
	QDateTime m_lastNotificationTime;
	quint32 m_notificationId;
	bool m_error;
};
