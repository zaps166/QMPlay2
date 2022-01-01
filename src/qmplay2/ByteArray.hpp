/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <cstdint>

class ByteArray
{
public:
    inline ByteArray(const char *data, const uint32_t len, const bool data_bigendian = false) :
        data((const uint8_t *)data),
        dataStart((const uint8_t *)data),
        dataEnd((const uint8_t *)data+len),
        data_bigendian(data_bigendian)
    {}
    inline ByteArray(const uint8_t *data, const uint32_t len, const bool data_bigendian = false) :
        data(data),
        dataStart(data),
        dataEnd(data+len),
        data_bigendian(data_bigendian)
    {}

    inline bool atEnd() const
    {
        return data == dataEnd;
    }

    inline uint32_t pos() const
    {
        return data - dataStart;
    }
    inline uint32_t remaining() const
    {
        return dataEnd - data;
    }

    inline uint8_t getBYTE()
    {
        if (atEnd())
            return 0;
        return *(data++);
    }
    inline uint16_t getWORD()
    {
        if (data + 2 > dataEnd)
        {
            data = dataEnd;
            return 0;
        }
        uint16_t ret;
        if (data_bigendian)
            ret = data[1] | data[0] << 8;
        else
            ret = data[0] | data[1] << 8;
        data += sizeof ret;
        return ret;
    }
    inline uint32_t get24bAs32b()
    {
        if (data + 3 > dataEnd)
        {
            data = dataEnd;
            return 0;
        }
        uint32_t ret;
        if (data_bigendian)
            ret = data[2] << 8 | data[1] << 16 | data[0] << 24;
        else
            ret = data[0] << 8 | data[1] << 16 | data[2] << 24;
        data += 3;
        return ret;
    }
    inline uint32_t getDWORD()
    {
        if (data + 4 > dataEnd)
        {
            data = dataEnd;
            return 0;
        }
        uint32_t ret;
        if (data_bigendian)
            ret = data[3] | data[2] << 8 | data[1] << 16 | data[0] << 24;
        else
            ret = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
        data += sizeof ret;
        return ret;
    }
    inline float getFloat()
    {
        if (data + 4 > dataEnd)
        {
            data = dataEnd;
            return 0;
        }
        union
        {
            uint32_t u32;
            float flt;
        } ret;
        if (data_bigendian)
            ret.u32 = data[3] | data[2] << 8 | data[1] << 16 | data[0] << 24;
        else
            ret.u32 = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
        data += sizeof ret;
        return ret.flt;
    }

    inline ByteArray & operator =(const uint32_t pos)
    {
        if (dataStart + pos < dataEnd)
            data = dataStart + pos;
        else
            data = dataEnd;
        return *this;
    }
    inline ByteArray & operator +=(const uint32_t pos)
    {
        if (data + pos < dataEnd)
            data += pos;
        else
            data = dataEnd;
        return *this;
    }
    inline ByteArray & operator -=(const uint32_t pos)
    {
        if (data - pos > dataStart)
            data -= pos;
        else
            data = dataStart;
        return *this;
    }
    inline void operator ++()
    {
        if (!atEnd())
            ++data;
    }

    inline ByteArray operator +(const uint32_t bytes) const
    {
        return ByteArray(*this) += bytes;
    }
    inline ByteArray operator -(const uint32_t bytes) const
    {
        return ByteArray(*this) -= bytes;
    }

    inline uint8_t operator *() const
    {
        if (atEnd())
            return 0;
        return *data;
    }
    inline uint8_t operator [](const uint32_t idx) const
    {
        if (atEnd())
            return 0;
        return data[idx];
    }

    inline operator const void *() const
    {
        if (atEnd())
            return nullptr;
        return data;
    }
    inline operator const char *() const
    {
        if (atEnd())
            return nullptr;
        return (const char *)data;
    }
    inline operator const uint8_t *() const
    {
        if (atEnd())
            return nullptr;
        return data;
    }
private:
    const uint8_t *data, *const dataStart, *const dataEnd;
    const bool data_bigendian;
};

static inline uint32_t FourCC(const char str[5], const bool data_bigendian = false)
{
    if (data_bigendian)
        return str[3] | str[2] << 8 | str[1] << 16 | str[0] << 24;
    else
        return str[0] | str[1] << 8 | str[2] << 16 | str[3] << 24;
}
