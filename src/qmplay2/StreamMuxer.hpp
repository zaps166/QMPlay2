// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Lib.hpp>

#include <QList>

struct AVFormatContext;
class StreamInfo;
class Packet;

class QMPLAY2SHAREDLIB_EXPORT StreamMuxer
{
    struct Priv;

    StreamMuxer(const StreamMuxer &) = delete;
    StreamMuxer &operator =(const StreamMuxer &) = delete;

public:
    StreamMuxer(const QString &fileName, const QList<StreamInfo *> &streamsInfo, const QString &format, bool streamRecording = false);
    ~StreamMuxer();

    bool isOk() const;

    bool setFirstDts(const Packet &packet, const int idx);
    bool write(const Packet &packet, const int idx);

private:
    Priv &p;
};
