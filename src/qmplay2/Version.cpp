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

#include <Version.hpp>

#include <QMPlay2Core.hpp>

#include <QFile>

#ifndef QMPlay2GitHEAD
	#define QMPlay2GitHEAD
#endif
#define QMPlay2Version "18.03.02" QMPlay2GitHEAD

QByteArray Version::get()
{
	static const QByteArray ver = QMPlay2Version + (isPortable() ? "-portable" : QByteArray());
	return ver;
}
QByteArray Version::userAgent()
{
	static const QByteArray agent = "QMPlay2/" + get();
	return agent;
}
bool Version::isPortable()
{
	static bool portable = QFile::exists(QMPlay2Core.getShareDir() + "portable");
	return portable;
}
