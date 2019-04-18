#include <NotifiesMacOS.hpp>

#import <AppKit/AppKit.h>

#include <QPixmap>
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

static inline void releaseNotification(NSUserNotification *notification)
{
    [[NSUserNotificationCenter defaultUserNotificationCenter] removeDeliveredNotification: notification];
    [notification release];
}

/**/

NotifiesMacOS::NotifiesMacOS()
{
    QMPlay2NotificationItem *notificationItem = [QMPlay2NotificationItem alloc];
    [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:notificationItem];
    m_notificationItem = notificationItem;
}
NotifiesMacOS::~NotifiesMacOS()
{
    [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:nil];
    [(QMPlay2NotificationItem *)m_notificationItem release];
    for (auto &&notification : m_notifications)
        releaseNotification((NSUserNotification *)notification);
}

bool NotifiesMacOS::doNotify(const QString &title, const QString &message, const int ms, const QPixmap &pixmap, const int iconId)
{
    Q_UNUSED(iconId)

    if (!m_notificationItem)
        return false;

    NSUserNotification *notification = [[NSUserNotification alloc] init];

    notification.title = title.toNSString();
    notification.informativeText = message.toNSString();
    if (!pixmap.isNull())
        notification.contentImage = QtMac::toNSImage(pixmap);

    [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notification];

    if (ms < 1)
        [notification release];
    else
    {
        QTimer::singleShot(ms + 500 /* Add 500 ms for animation */, [this, notification] {
            const int idx = m_notifications.indexOf(notification);
            if (idx > -1)
            {
                releaseNotification(notification);
                m_notifications.removeAt(idx);
            }
        });
        m_notifications.append(notification);
    }

    return true;
}
bool NotifiesMacOS::doNotify(const QString &title, const QString &message, const int ms, const QImage &image, const int iconId)
{
    return doNotify(title, message, ms, QPixmap::fromImage(image), iconId);
}
