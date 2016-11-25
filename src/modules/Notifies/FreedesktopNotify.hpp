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

#include <Notify.hpp>

#include <QDateTime>

class OrgFreedesktopNotificationsInterface;
class QDBusPendingCallWatcher;

class FreedesktopNotify : public Notify
{
	Q_OBJECT
	Q_DISABLE_COPY(FreedesktopNotify)
public:
	FreedesktopNotify(qint32 timeout);
	~FreedesktopNotify();

	bool showMessage(const QString &summary, const QString &message = QString(), const QString &icon = QString(), const QImage &image = QImage());
private slots:
	void CallFinished(QDBusPendingCallWatcher *watcher);
private:
	OrgFreedesktopNotificationsInterface *m_interface;
	uint m_notificationId;
	QDateTime m_lastNotificationTime;
};
