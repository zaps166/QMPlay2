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

#include <Buffer.hpp>

extern "C"
{
	#include <libavutil/buffer.h>
	#include <libavutil/mem.h>
}

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

bool Buffer::resize(qint32 len)
{
	if (m_offset > 0)
		return false;
	if (capacity() < len)
		av_buffer_realloc(&m_bufferRef, len);
	m_size = len;
	return true;
}

bool Buffer::remove(qint32 pos, qint32 len)
{
	if (pos < 0 || pos >= m_size || len < 0 || m_offset > 0)
		return false;
	if (quint8 *bufferData = data())
	{
		if (pos + len > m_size)
			len = m_size - pos;
		m_size -= len;
		memmove(bufferData + pos, bufferData + pos + len, m_size - pos);
		return true;
	}
	return false;
}

void Buffer::clear()
{
	av_buffer_unref(&m_bufferRef);
	m_size = 0;
	m_offset = 0;
}

const quint8 *Buffer::data() const
{
	return !isNull() ? (m_bufferRef->data + m_offset) : nullptr;
}
quint8 *Buffer::data()
{
	if (isNull())
		return nullptr;
	av_buffer_make_writable(&m_bufferRef);
	return m_bufferRef->data + m_offset;
}

void Buffer::assign(AVBufferRef *otherBufferRef, qint32 len, qint32 offset)
{
	av_buffer_unref(&m_bufferRef);
	m_bufferRef = otherBufferRef;
	m_size = (len >= 0 && len <= m_bufferRef->size) ? len : m_bufferRef->size;
	m_offset = (offset > 0 && m_size + offset <= m_bufferRef->size) ? offset : 0;
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
	m_offset = 0;
}

AVBufferRef *Buffer::toAvBufferRef() const
{
	return av_buffer_ref(m_bufferRef);
}

void Buffer::copy(const Buffer &other)
{
	av_buffer_unref(&m_bufferRef);
	if (other.m_bufferRef)
		m_bufferRef = other.toAvBufferRef();
	m_size = other.m_size;
	m_offset = other.m_offset;
}
