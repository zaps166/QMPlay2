#include "QMPlay2MacExtensions.hpp"

#include <QAbstractNativeEventFilter>
#include <QCoreApplication>

#include <IOKit/hidsystem/ev_keymap.h>
#include <AppKit/NSApplication.h>
#include <AppKit/NSEvent.h>

class MediaKeysFilter : public QAbstractNativeEventFilter
{
public:
	MediaKeysFilter(const QMPlay2MacExtensions::MediaKeysCallback &cb) :
		m_mediaKeysCallback(cb)
	{}

private:
	bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override final
	{
		Q_UNUSED(result)
		if (eventType == "mac_generic_NSEvent")
		{
			NSEvent *event = static_cast<NSEvent *>(message);
#if defined(MAC_OS_X_VERSION_10_12) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12)
			if ([event type] == NSEventTypeSystemDefined)
#else
			if ([event type] == NSSystemDefined)
#endif
			{
				const int  keyCode   = ([event data1] & 0xFFFF0000) >> 16;
				const int  keyFlags  = ([event data1] & 0x0000FFFF);
				const int  keyState  = (((keyFlags & 0xFF00) >> 8) == 0xA);
				const bool keyRepeat = (keyFlags & 0x1);
				Q_UNUSED(keyRepeat)

				if (keyState == 1)
				{
					switch (keyCode)
					{
						case NX_KEYTYPE_PLAY:
							m_mediaKeysCallback("toggle");
							return true;
						case NX_KEYTYPE_NEXT:
						case NX_KEYTYPE_FAST:
							m_mediaKeysCallback("next");
							return true;
						case NX_KEYTYPE_PREVIOUS:
						case NX_KEYTYPE_REWIND:
							m_mediaKeysCallback("prev");
							return true;
					}
				}
			}
		}
		return false;
	}

	QMPlay2MacExtensions::MediaKeysCallback m_mediaKeysCallback;
} static *g_mediaKeysFilter;

/**/

void QMPlay2MacExtensions::setApplicationVisible(bool visible)
{
	static NSApplication *app = [NSApplication sharedApplication];
	if (visible)
		[app unhide:nil];
	else
		[app hide:nil];
}

void QMPlay2MacExtensions::registerMacOSMediaKeys(const MediaKeysCallback &cb)
{
	if (!g_mediaKeysFilter)
	{
		g_mediaKeysFilter = new MediaKeysFilter(cb);
		QCoreApplication::instance()->installNativeEventFilter(g_mediaKeysFilter);
	}
}
void QMPlay2MacExtensions::unregisterMacOSMediaKeys()
{
	if (g_mediaKeysFilter)
	{
		QCoreApplication::instance()->removeNativeEventFilter(g_mediaKeysFilter);
		delete g_mediaKeysFilter;
		g_mediaKeysFilter = nullptr;
	}
}

void QMPlay2MacExtensions::showSystemUi(bool visible)
{
	unsigned long flags;
	if (visible)
		flags = NSApplicationPresentationDefault;
	else
		flags = NSApplicationPresentationHideDock | NSApplicationPresentationAutoHideMenuBar;
	[NSApp setPresentationOptions:flags];
}
