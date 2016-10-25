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

#include <Buffer.hpp>

extern "C"
{
	#include <libavutil/buffer.h>
	#include <libavutil/mem.h>
}

Buffer::Buffer(const Buffer &other) :
	m_bufferRef(other.m_bufferRef ? av_buffer_ref(other.m_bufferRef) : NULL),
	m_size(other.m_size)
{}
Buffer::~Buffer()
{
	av_buffer_unref(&m_bufferRef);
}

qint32 Buffer::capacity() const
{
	if (isNull())
		return 0;
	return m_bufferRef->size;
}

bool Buffer::isWritable() const
{
	return !isNull() && av_buffer_is_writable(m_bufferRef);
}

void Buffer::resize(qint32 len)
{
	if (capacity() < len)
		av_buffer_realloc(&m_bufferRef, len);
	m_size = len;
}

void Buffer::remove(qint32 pos, qint32 len)
{
	if (pos < 0 || pos >= m_size || len < 0)
		return;
	quint8 *bufferData = data();
	if (bufferData)
	{
		if (pos + len > m_size)
			len = m_size - pos;
		m_size -= len;
		memmove(bufferData + pos, bufferData + pos + len, m_size - pos);
	}
}

void Buffer::clear()
{
	av_buffer_unref(&m_bufferRef);
	m_size = 0;
}

const quint8 *Buffer::data() const
{
	return !isNull() ? m_bufferRef->data : NULL;
}
quint8 *Buffer::data()
{
	if (isNull())
		return NULL;
	av_buffer_make_writable(&m_bufferRef);
	return m_bufferRef->data;
}

void Buffer::assign(AVBufferRef *otherBufferRef, qint32 len)
{
	av_buffer_unref(&m_bufferRef);
	m_bufferRef = otherBufferRef;
	m_size = (len >= 0 && len <= m_bufferRef->size) ? len : m_bufferRef->size;
}
void Buffer::assign(const void *data, qint32 len, qint32 mem)
{
	if (mem < len)
		mem = len;
	if (!isWritable() || capacity() < mem)
	{
		av_buffer_unref(&m_bufferRef);
		av_buffer_realloc(&m_bufferRef, mem);
	}
	memcpy(m_bufferRef->data, data, len);
	memset(m_bufferRef->data + len, 0, mem - len);
	m_size = len;
}

AVBufferRef *Buffer::toAvBufferRef()
{
	return av_buffer_ref(m_bufferRef);
}

#if 0
void Buffer::append(const void *data, qint32 len)
{
	av_buffer_realloc(&m_bufferRef, m_size + len);
	memcpy(m_bufferRef->data + m_size, data, len);
	m_size += len;
}
#endif

Buffer &Buffer::operator =(const Buffer &other)
{
	av_buffer_unref(&m_bufferRef);
	if (other.m_bufferRef)
		m_bufferRef = av_buffer_ref(other.m_bufferRef);
	m_size = other.m_size;
	return *this;
}
