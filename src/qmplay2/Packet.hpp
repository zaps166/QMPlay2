// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Lib.hpp>
#include <Frame.hpp>

#include <optional>

extern "C" {
    #include <libavcodec/version.h>
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(58, 91, 100)
    #include <libavcodec/packet.h>
#else
    #include <libavcodec/avcodec.h>
#endif
    #include <libavutil/rational.h>
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

    bool hasFrame() const;
    const Frame &frame() const;
    void setFrame(Frame &&frame);

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
    std::optional<Frame> m_frame;
};
