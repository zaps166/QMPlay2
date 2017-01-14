/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <QtGlobal>

struct AVBufferRef;

class Buffer
{
	AVBufferRef *m_bufferRef;
	qint32 m_size;
public:
	inline Buffer() :
		m_bufferRef(NULL),
		m_size(0)
	{}
	Buffer(const Buffer &other);
	~Buffer();

	inline bool isNull() const
	{
		return m_bufferRef == NULL;
	}
	inline bool isEmpty() const
	{
		return size() == 0;
	}

	inline qint32 size() const
	{
		return m_size;
	}
	qint32 capacity() const;

	bool isWritable() const;

	void resize(qint32 len);
	void remove(qint32 pos, qint32 len);
	void clear();

	const quint8 *data() const;
	inline const quint8 *constData() const
	{
		return data();
	}
	quint8 *data(); //Automatically detaches the buffer if necessary

	void assign(AVBufferRef *otherBufferRef, qint32 len = -1);
	void assign(const void *data, qint32 len, qint32 mem);
	inline void assign(const void *data, qint32 len)
	{
		assign(data, len, len);
	}

	AVBufferRef *toAvBufferRef();

#if 0
	void append(const void *data, qint32 len);
#endif

	Buffer &operator =(const Buffer &other);
};

#endif //BUFFER_HPP
