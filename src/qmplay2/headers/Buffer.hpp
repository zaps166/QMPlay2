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

	void append(const void *data, qint32 len);

	Buffer &operator =(const Buffer &other);
};

#endif //BUFFER_HPP
