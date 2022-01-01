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

#include <OggHelper.hpp>

extern "C"
{
    #include <libavformat/avio.h>
}

static int readPacket(void *opaque, uint8_t *buf, int bufSize)
{
    OggHelper *oggHelper = (OggHelper *)opaque;
    const int64_t pos = avio_tell(oggHelper->io);
    return avio_read(oggHelper->io, buf, oggHelper->size > 0 ? qMin<int>(bufSize, oggHelper->size - pos) : bufSize);
}
static int64_t seekPacket(void *opaque, int64_t offset, int whence)
{
    OggHelper *oggHelper = (OggHelper *)opaque;
    switch (whence)
    {
        case SEEK_END:
            offset += avio_size(oggHelper->io);
            whence = SEEK_SET;
            break;
        case AVSEEK_SIZE:
            if (oggHelper->size > 0)
                return oggHelper->size;
            return avio_size(oggHelper->io);
    }
    return avio_seek(oggHelper->io, offset, whence);
}

static int interruptCB(bool &aborted)
{
    return aborted;
}

constexpr int g_minHeaderSize = 27;
constexpr int g_maxPageSize = 65307; //MinHeaderSize + 255 + 255 * 255

/**/

OggHelper::OggHelper(const QString &url, int track, qint64 size, const AVIOInterruptCB &interruptCB) :
    io(nullptr), pb(nullptr),
    isAborted(nullptr),
    size(size),
    track(track)
{
    if (avio_open2(&io, url.toUtf8(), AVIO_FLAG_READ, &interruptCB, nullptr) >= 0)
        pb = avio_alloc_context((quint8 *)av_malloc(4096), 4096, false, this, readPacket, nullptr, seekPacket);
}
OggHelper::OggHelper(const QString &url, bool &isAborted) :
    io(nullptr), pb(nullptr),
    isAborted(&isAborted),
    size(-1),
    track(-1)
{
    const AVIOInterruptCB avioInterruptCB = {(int(*)(void *))interruptCB, &isAborted};
    avio_open2(&io, url.toUtf8(), AVIO_FLAG_READ, &avioInterruptCB, nullptr);
}
OggHelper::~OggHelper()
{
    if (pb)
    {
        av_free(pb->buffer);
        av_free(pb);
    }
    if (io)
        avio_close(io);
}

OggHelper::Chains OggHelper::getOggChains(bool &ok)
{
    Chains chains;

    /* Read OGG page header at the end of the file */
    QByteArray endData(g_maxPageSize, Qt::Uninitialized);
    const qint64 fileSize = avio_size(io);
    if (avio_seek(io, qMax<qint64>(fileSize - g_maxPageSize, 0), SEEK_SET) < 0)
        return chains;
    const int tmpToRead = qMin<qint64>(g_maxPageSize, fileSize);
    if (avio_read(io, (uint8_t *)endData.data(), tmpToRead) != tmpToRead)
        return chains;
    const int lastPagePos = endData.lastIndexOf(QByteArray("OggS", 5)); //0 byte at the end of the string means OGG version 0
    if (lastPagePos < 0)
    {
        //Incorrect "Magic" value or unsupported OGG version
        return chains;
    }
    endData.remove(0, lastPagePos);
    if (endData.size() < g_minHeaderSize)
    {
        //Incorrect header size
        return chains;
    }
    if (avio_seek(io, 0, SEEK_SET) < 0)
        return chains;
    const quint32 endHeaderSerial = getSerial((const quint8 *)endData.constData());
    if (!endHeaderSerial) //Probably serial can't be 0
        return chains;

    /* Declare helper variables */
    QVector<quint32> firstStreams;
    bool checkEndHeader = true;

    int64_t eosCnt = 0, bosCnt = 0;
    quint8 header[g_minHeaderSize];
    quint8 pageSegments[255];

    /* Analyse OGG file */
    while (!*isAborted)
    {
        /* Read and check OGG page header */
        if (avio_read(io, header, g_minHeaderSize) != g_minHeaderSize)
        {
            //EOF or read error - break
            break;
        }
        if (memcmp(header, "OggS", 5)) //0 byte at the end of the string means OGG version 0
        {
            //Incorrect "Magic" value or unsupported OGG version - break
            break;
        }

        /* Get begin of stream and end of stream info */
        const bool bos = getBeginOfStream(header);
        const bool eos = getEndOfStream(header);

        /* Check if OGG file is chained.
         * Compare OGG page serial at the end of the file with one of first streams
        */
        if (checkEndHeader)
        {
            if (!bos && bosCnt)
            {
                // We've got all streams in this chain, let's check the stream on the last page in the file.
                bool isChained = true;
                for (int i = 0; i < firstStreams.size(); ++i)
                {
                    if (endHeaderSerial == firstStreams[i])
                    {
                        isChained = false;
                        break;
                    }
                }
                if (!isChained)
                    break;
                firstStreams.clear();
                checkEndHeader = false;
            }
            else
            {
                firstStreams.append(getSerial(header));
            }
        }

        if (bos && bosCnt == eosCnt)
        {
            //Create a new chain - get its start offset in the file
            chains.append(qMakePair<qint64, qint64>(avio_tell(io) - sizeof header, -1));
        }

        bosCnt += bos;
        eosCnt += eos;

        /* Skip the data */
        int seekOffset = 0;
        const quint8 pageSegmentsCount = getPageSegmentsCount(header);
        if (avio_read(io, pageSegments, pageSegmentsCount) != pageSegmentsCount)
        {
            //EOF or read error - break
            break;
        }
        for (uint32_t i = 0; i < pageSegmentsCount; ++i)
        {
            //Calculate the page data size
            seekOffset += pageSegments[i];
        }
        if (avio_seek(io, seekOffset, SEEK_CUR) < 0)
        {
            //EOF or seek error - break
            break;
        }

        /* End of the chain - we've got end of stream and all streams are ended */
        if (eos && bosCnt == eosCnt)
        {
            if (chains.empty() || chains.last().second != -1)
            {
                //Something went wrong - break
                break;
            }
            chains[chains.size() - 1].second = avio_tell(io);
        }
    }

    if (*isAborted || chains.size() == 1)
    {
        //Something went wrong - let fully handle this via FFmpeg
        chains.clear();
        if (*isAborted)
            ok = false;
    }
    else if (!chains.isEmpty())
    {
        // Always let FFmpeg handle last chain size
        chains.last().second = -1;
    }

    return chains;
}

inline quint32 OggHelper::getSerial(const quint8 *header) const
{
    return *(quint32 *)(header + 14);
}
inline bool OggHelper::getBeginOfStream(const quint8 *header) const
{
    return *(header + 5) & 0x02;
}
inline bool OggHelper::getEndOfStream(const quint8 *header) const
{
    return *(header + 5) & 0x04;
}
inline quint8 OggHelper::getPageSegmentsCount(const quint8 *header) const
{
    return *(header + 26);
}
