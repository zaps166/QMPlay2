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
