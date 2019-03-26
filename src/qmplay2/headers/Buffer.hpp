/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

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

struct AVBufferRef;

class QMPLAY2SHAREDLIB_EXPORT Buffer
{
    AVBufferRef *m_bufferRef = nullptr;
    qint32 m_size = 0;
    qint32 m_offset = 0;

public:
    Buffer() = default;
    inline Buffer(const Buffer &other);
    inline Buffer(Buffer &&other);
    ~Buffer();

    inline bool isNull() const;
    inline bool isEmpty() const;

    inline qint32 size() const;
    inline qint32 offset() const;
    qint32 capacity() const;

    bool isWritable() const;

    bool reserve(qint32 len);
    bool resize(qint32 len);
    bool remove(qint32 pos, qint32 len);
    void clear();

    const quint8 *data() const;
    inline const quint8 *constData() const;
    quint8 *data(); //Automatically detaches the buffer if necessary

    void assign(AVBufferRef *otherBufferRef, qint32 len = -1, qint32 offset = 0);
    void assign(const void *data, qint32 len, qint32 mem);
    inline void assign(const void *data, qint32 len);

    AVBufferRef *toAvBufferRef() const;

    inline Buffer &operator =(const Buffer &other);
    inline Buffer &operator =(Buffer &&other);

private:
    void copy(const Buffer &other);
    inline void move(Buffer &other);
};

/* Inline implementation */

Buffer::Buffer(const Buffer &other)
{
    copy(other);
}
Buffer::Buffer(Buffer &&other)
{
    move(other);
}

bool Buffer::isNull() const
{
    return m_bufferRef == nullptr;
}
bool Buffer::isEmpty() const
{
    return size() == 0;
}

qint32 Buffer::size() const
{
    return m_size;
}
qint32 Buffer::offset() const
{
    return m_offset;
}

const quint8 *Buffer::constData() const
{
    return data();
}

void Buffer::assign(const void *data, qint32 len)
{
    assign(data, len, len);
}

Buffer &Buffer::operator =(const Buffer &other)
{
    copy(other);
    return *this;
}
Buffer &Buffer::operator =(Buffer &&other)
{
    move(other);
    return *this;
}

void Buffer::move(Buffer &other)
{
    qSwap(m_bufferRef, other.m_bufferRef);
    qSwap(m_size, other.m_size);
    qSwap(m_offset, other.m_offset);
}
