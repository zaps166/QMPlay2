// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QString>
#include <QVector>
#include <QPair>

struct AVIOInterruptCB;
struct AVIOContext;

class OggHelper
{
    Q_DISABLE_COPY(OggHelper)

public:
    using Chain = QPair<qint64, qint64>;
    using Chains = QVector<Chain>;

    OggHelper(const QString &url, int track, qint64 size, const AVIOInterruptCB &interruptCB);
    OggHelper(const QString &url, bool &isAborted);
    ~OggHelper();

    Chains getOggChains(bool &ok);

private:
    inline quint32 getSerial(const quint8 *header) const;
    inline bool getBeginOfStream(const quint8 *header) const;
    inline bool getEndOfStream(const quint8 *header) const;
    inline quint8 getPageSegmentsCount(const quint8 *header) const;

public:
    AVIOContext *io, *pb;
    bool *isAborted;
    qint64 size;
    int track;
};
