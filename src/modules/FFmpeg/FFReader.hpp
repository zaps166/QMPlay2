/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

#ifndef FFREADER_HPP
#define FFREADER_HPP

#include <Reader.hpp>

struct AVIOContext;

class FFReader : public Reader
{
public:
	FFReader();
private:
	bool readyRead() const;
	bool canSeek() const;

	bool seek(qint64);
	QByteArray read(qint64);
	void pause();
	bool atEnd() const;
	void abort();

	qint64 size() const;
	qint64 pos() const;
	QString name() const;

	bool open();

	/**/

	~FFReader();

	AVIOContext *avioCtx;
	bool aborted, paused, canRead;
};

#endif // FFREADER_HPP
