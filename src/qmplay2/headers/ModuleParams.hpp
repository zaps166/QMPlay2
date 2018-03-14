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

#include <QHash>
#include <QString>
#include <QVariant>

class QMPLAY2SHAREDLIB_EXPORT ModuleParams
{
	Q_DISABLE_COPY(ModuleParams)
public:
	bool modParam(const QString &key, const QVariant &val);
	inline QVariant getParam(const QString &key) const
	{
		return paramList.value(key);
	}
	inline bool hasParam(const QString &key) const
	{
		return paramList.contains(key);
	}

	virtual bool processParams(bool *paramsCorrected = nullptr);
protected:
	ModuleParams() = default;
	~ModuleParams() = default;

	inline void addParam(const QString &key, const QVariant &val = QVariant())
	{
		paramList.insert(key, val);
	}
private:
	QHash<QString, QVariant> paramList;
};
