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

#include <Writer.hpp>

#include <QSemaphore>

#include <SLES/OpenSLES.h>

class OpenSLESWriter : public Writer
{
public:
	OpenSLESWriter(Module &);
private:
	~OpenSLESWriter();

	bool set();

	bool readyWrite() const;

	bool processParams(bool *paramsCorrected);
	qint64 write(const QByteArray &);
	void pause();

	QString name() const;

	bool open();

	/**/

	void close();

	SLObjectItf engineObject;
	SLEngineItf engineEngine;
	SLObjectItf outputMixObject;
	SLObjectItf bqPlayerObject;
	SLPlayItf bqPlayerPlay;
	SLBufferQueueItf bqPlayerBufferQueue;

	QVector<QVector<qint16> > buffers;
	QVector<qint16> tmpBuffer;
	QSemaphore sem;
	int currBuffIdx;

	int sample_rate, channels;
	bool paused;
};

#define OpenSLESWriterName "OpenSL|ES"
