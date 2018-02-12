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

#include <IOController.hpp>

#include <QString>
#include <QList>

class Q_DECL_EXPORT Playlist
{
public:
	class Entry
	{
	public:
		enum Flags
		{
			Selected = 0x1,
			Skip = 0x2,
			StopAfter = 0x4,
			Locked = 0x8,
			AlwaysSync = 0x10,
		};

		Entry() = default;
		inline Entry(const QString &name, const QString &url) :
			name(name), url(url)
		{}

		QString name, url;
		double length = -1.0;
		qint32 flags = 0, queue = 0, GID = 0, parent = 0;
	};
	using Entries = QVector<Entry>;

	enum OpenMode
	{
		NoOpen,
		ReadOnly,
		WriteOnly
	};

	virtual ~Playlist() = default;

	static Entries read(const QString &url, QString *name = nullptr);
	static bool write(const Entries &list, const QString &url, QString *name = nullptr);
	static QStringList extensions();

	virtual Entries read() = 0;
	virtual bool write(const Entries &) = 0;

private:
	static Playlist *create(const QString &url, OpenMode openMode, QString *name = nullptr);

protected:
	static QString getPlaylistPath(const QString &url);

	QList<QByteArray> readLines();

	IOController<> ioCtrl;
};
