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

#include <QMPlay2Lib.hpp>

class QSystemTrayIcon;
class QPixmap;
class QImage;

class QMPLAY2SHAREDLIB_EXPORT Notifies
{
public:
	static void initialize(QSystemTrayIcon *tray);
	static bool hasBoth();
	static void setNativeFirst(bool f);
	static void finalize();

	static bool notify(const QString &title, const QString &message, const int ms, const QPixmap &pixmap, const int iconId = 0);
	static bool notify(const QString &title, const QString &message, const int ms, const QImage &image, const int iconId = 0);
	static bool notify(const QString &title, const QString &message, const int ms, const int iconId = 0);

protected:
	virtual ~Notifies() = default;

	virtual bool doNotify(const QString &title, const QString &message, const int ms, const QPixmap &pixmap, const int iconId) = 0;
	virtual bool doNotify(const QString &title, const QString &message, const int ms, const QImage &image, const int iconId) = 0;
	virtual bool doNotify(const QString &title, const QString &message, const int ms, const int iconId);

private:
	static Notifies *s_notifies[2];
	static bool s_nativeFirst;
};
