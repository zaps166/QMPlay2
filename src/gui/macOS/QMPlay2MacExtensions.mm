#include "QMPlay2MacExtensions.hpp"

#import <AppKit/AppKit.h>

void QMPlay2MacExtensions::setApplicationVisible(bool visible)
{
	static NSApplication *app = [NSApplication sharedApplication];
	if (visible)
		[app unhide:nil];
	else
		[app hide:nil];
}
