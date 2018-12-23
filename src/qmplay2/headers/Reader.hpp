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

#include <ModuleCommon.hpp>
#include <ModuleParams.hpp>
#include <IOController.hpp>

class QMPLAY2SHAREDLIB_EXPORT Reader : protected ModuleCommon, public ModuleParams, public BasicIO
{
public:
	static bool create(const QString &url, IOController<Reader> &reader, const QString &plugName = QString());

	virtual ~Reader() = default;

	inline QString getUrl() const
	{
		return _url;
	}

	virtual bool readyRead() const = 0;
	virtual bool canSeek() const = 0;

	virtual bool seek(qint64) = 0;
	virtual QByteArray read(qint64) = 0;
	virtual bool atEnd() const = 0;

	virtual qint64 size() const = 0;
	virtual qint64 pos() const = 0;
	virtual QString name() const = 0;

private:
	virtual bool open() = 0;

	QString _url;
};
