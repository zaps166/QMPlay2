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

#include <QList>

struct AVFormatContext;
class StreamInfo;
struct Packet;

class QMPLAY2SHAREDLIB_EXPORT MkvMuxer
{
	MkvMuxer(const MkvMuxer &) = delete;
	MkvMuxer &operator =(const MkvMuxer &) = delete;

public:
	MkvMuxer(const QString &fileName, const QList<StreamInfo *> &streamsInfo);
	~MkvMuxer();

	inline bool isOk() const;

	bool write(Packet &packet, const int idx);

private:
	AVFormatContext *m_ctx = nullptr;
	bool m_ok = false;
};

/* Inline implementation */

bool MkvMuxer::isOk() const
{
	return m_ok;
}
