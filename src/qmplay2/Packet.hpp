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

#include <QMPlay2Lib.hpp>

extern "C" {
    #include <libavcodec/avcodec.h>
}

class QMPLAY2SHAREDLIB_EXPORT Packet
{
public:
    Packet();
    explicit Packet(AVPacket *packet, bool forceCopy = false);
    Packet(const Packet &other);
    Packet(Packet &&other);
    ~Packet();

    bool isEmpty() const;
    void clear();

    void setTimeBase(const AVRational &timeBase);
    void setOffsetTS(double offsetTS);

    void resize(int size);
    int size() const;

    AVBufferRef *getBufferRef() const;
    uint8_t *data() const;

    double duration() const;
    void setDuration(double duration);

    bool hasKeyFrame() const;

    bool isTsValid() const;
    void setTsInvalid();

    bool hasDts() const;
    double dts() const;
    void setDts(double dts);

    bool hasPts() const;
    double pts() const;
    void setPts(double pts);

    double ts() const;
    void setTS(double ts);

private:
    inline int64_t d2i(double value);

public: // Operators
    inline operator const AVPacket *() const
    {
        return m_packet;
    }

    Packet &operator =(const Packet &other);
    Packet &operator =(Packet &&other);

private:
    AVPacket *m_packet = nullptr;
    AVRational m_timeBase = {1, 10000};
};
