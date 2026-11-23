// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <FFReader.hpp>

#include <Functions.hpp>
#include <FFCommon.hpp>

static int interruptCB(bool &aborted)
{
    return aborted;
}

class OpenAvioThr : public OpenThr
{
    AVIOContext *m_avioCtx;

public:
    inline OpenAvioThr(const QByteArray &url, AVDictionary *options, std::shared_ptr<AbortContext> &abortCtx) :
        OpenThr(url, options, abortCtx),
        m_avioCtx(nullptr)
    {
        start();
    }

    inline AVIOContext *getAvioCtx() const
    {
        return waitForOpened() ? m_avioCtx : nullptr;
    }

private:
    void run() override
    {
        const AVIOInterruptCB interruptCB = {(int(*)(void*))::interruptCB, &m_abortCtx->isAborted};
        avio_open2(&m_avioCtx, m_url, AVIO_FLAG_READ, &interruptCB, &m_options);
        if (!wakeIfNotAborted() && m_avioCtx)
            avio_close(m_avioCtx);
    }
};

/**/

FFReader::FFReader() :
    avioCtx(nullptr),
    paused(false), canRead(false),
    abortCtx(new AbortContext)
{}

bool FFReader::readyRead() const
{
    return canRead && !abortCtx->isAborted;
}
bool FFReader::canSeek() const
{
    return avioCtx->seekable;
}

bool FFReader::seek(qint64 pos)
{
    return avio_seek(avioCtx, pos, SEEK_SET) >= 0;
}
QByteArray FFReader::read(qint64 size)
{
    QByteArray arr;
    arr.resize(size);
    if (paused)
    {
        avio_pause(avioCtx, false);
        paused = false;
    }
    int ret = avio_read(avioCtx, (quint8 *)arr.data(), arr.size());
    if (ret > 0)
    {
        if (arr.size() > ret)
            arr.resize(ret);
        return arr;
    }
    canRead = false;
    return QByteArray();
}
void FFReader::pause()
{
    avio_pause(avioCtx, true);
    paused = true;
}
bool FFReader::atEnd() const
{
    return avio_feof(avioCtx);
}
void FFReader::abort()
{
    abortCtx->abort();
}

qint64 FFReader::size() const
{
    return avio_size(avioCtx);
}
qint64 FFReader::pos() const
{
    return avio_tell(avioCtx);
}
QString FFReader::name() const
{
    return FFReaderName;
}

bool FFReader::open()
{
    AVDictionary *options = nullptr;
    const QString url = Functions::prepareFFmpegUrl(getUrl(), options, false);

    OpenAvioThr *openThr = new OpenAvioThr(url.toUtf8(), options, abortCtx);
    avioCtx = openThr->getAvioCtx();
    openThr->drop();

    if (avioCtx)
        canRead = true;

    return canRead;
}

FFReader::~FFReader()
{
    avio_close(avioCtx);
}
