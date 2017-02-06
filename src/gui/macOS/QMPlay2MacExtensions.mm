#include "QMPlay2MacExtensions.hpp"

#import <AppKit/AppKit.h>

#include <QApplication>
#include <QPixmap>
#include <QStyle>
#include <QTimer>
#include <QtMac>

@interface QMPlay2NotificationItem : NSObject<NSUserNotificationCenterDelegate>
{}
- (void)dealloc;
- (BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification;
@end

@implementation QMPlay2NotificationItem
- (void)dealloc
{
	[super dealloc];
}
- (BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification
{
	Q_UNUSED(center)
	Q_UNUSED(notification)
	return YES;
}
@end

/**/

static QMPlay2NotificationItem *g_notificationItem;
static QList<NSUserNotification *> g_notifications;

static inline void releaseNotification(NSUserNotification *notification)
{
	[[NSUserNotificationCenter defaultUserNotificationCenter] removeDeliveredNotification: notification];
	[notification release];
}

/**/

void QMPlay2MacExtensions::init()
{
	if (!g_notificationItem)
	{
		g_notificationItem = [QMPlay2NotificationItem alloc];
		[[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:g_notificationItem];
	}
}
void QMPlay2MacExtensions::deinit()
{
	if (g_notificationItem)
	{
		[[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:nil];
		[g_notificationItem release];
		g_notificationItem = nil;
	}
	for (auto &&notification : g_notifications)
		releaseNotification(notification);
	g_notifications.clear();
}

void QMPlay2MacExtensions::setApplicationVisible(bool visible)
{
	static NSApplication *app = [NSApplication sharedApplication];
	if (visible)
		[app unhide:nil];
	else
		[app hide:nil];
}

void QMPlay2MacExtensions::notify(const QString &title, const QString &msg, const int iconId, const int ms)
{
	if (!g_notificationItem)
		return;

	NSUserNotification *notification = [[NSUserNotification alloc] init];

	notification.title = title.toNSString();
	notification.informativeText = msg.toNSString();

	if (iconId > 0)
	{
		const QIcon icon = QApplication::style()->standardIcon((QStyle::StandardPixmap)((int)QStyle::SP_MessageBoxInformation + iconId - 1));
		const QList<QSize> iconSizes = icon.availableSizes();
		if (!iconSizes.isEmpty())
			notification.contentImage = QtMac::toNSImage(icon.pixmap(iconSizes.last()));
	}

	[[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notification];

	if (ms < 1)
		[notification release];
	else
	{
		QTimer::singleShot(ms + 500 /* Add 500 ms for animation */, [notification] {
			const int idx = g_notifications.indexOf(notification);
			if (idx > -1)
			{
				releaseNotification(notification);
				g_notifications.removeAt(idx);
			}
		});
		g_notifications.append(notification);
	}
}
