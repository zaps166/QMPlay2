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
	quint8 *bufferRefData = data();
	if (bufferRefData)
	{
		if (pos + len > m_size)
			len = m_size - pos;
		memmove(bufferRefData + pos, bufferRefData + pos + len, len);
		m_size -= len;
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
	m_bufferRef = otherBufferRef;
	m_size = (len >= 0 && len <= m_bufferRef->size) ? len : m_bufferRef->size;
}
void Buffer::assign(const void *data, qint32 len, qint32 mem)
{
	if (mem < len)
		mem = len;

	if (!isWritable())
		av_buffer_unref(&m_bufferRef);
	av_buffer_realloc(&m_bufferRef, mem);
	memcpy(m_bufferRef->data, data, len);
	memset(m_bufferRef->data + len, 0, mem - len);

	m_size = len;
}

void Buffer::append(const void *data, qint32 len)
{
	av_buffer_realloc(&m_bufferRef, m_size + len);
	memcpy(m_bufferRef->data + m_size, data, len);
	m_size += len;
}

Buffer &Buffer::operator =(const Buffer &other)
{
	av_buffer_unref(&m_bufferRef);
	m_bufferRef = other.m_bufferRef ? av_buffer_ref(other.m_bufferRef) : NULL;
	m_size = other.m_size;
	return *this;
}
