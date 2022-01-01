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

#include <NetworkAccess.hpp>

#include <QMPlay2Core.hpp>
#include <Functions.hpp>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavformat/avio.h>
    #include <libavutil/dict.h>
    #include <libavutil/opt.h>
}

#include <QThread>
#include <QMutex>

static int interruptCB(bool *m_status)
{
    m_status[1] = true;
    return m_status[0];
}

/**/

static bool doSleep(bool &aborted)
{
    for (int i = 0; i < 20 && !aborted; ++i)
        Functions::s_wait(0.1);
    return !aborted;
}

/**/

struct NetworkAccessParams
{
    QByteArray customUserAgent;
    int maxSize = INT_MAX;
    int retries = 1;
};

/**/

class NetworkReplyPriv : public QThread
{
public:
    NetworkReplyPriv(NetworkReply *networkReply, const QString &url, const QByteArray &postData, const QByteArray &rawHeaders, const NetworkAccessParams &params) :
        m_networkReply(networkReply),
        m_url(url),
        m_postData(postData),
        m_rawHeaders(rawHeaders),
        m_customUserAgent(params.customUserAgent),
        m_maxSize(params.maxSize),
        m_retries(params.retries),
        m_ctx(nullptr),
        m_error(NetworkReply::Error::Ok),
        m_aborted(false),
        m_afterOpen(false)
    {}
    ~NetworkReplyPriv() = default;

    NetworkReply *m_networkReply;

    const QString    m_url;
    const QByteArray m_postData;
    const QByteArray m_rawHeaders;
    const QByteArray m_customUserAgent;

    const int m_maxSize;
    int m_retries;

    AVIOContext *m_ctx;
    QByteArray m_cookies;
    QByteArray m_data;
    NetworkReply::Error m_error;

    QMutex m_networkReplyMutex, m_dataMutex;

    union
    {
        struct
        {
            bool m_aborted;
            bool m_afterOpen;
        };
        bool m_status[2];
    };

private:
    void run() override
    {
        const QString scheme = Functions::getUrlScheme(m_url);
        if (scheme.isEmpty() || scheme == "file")
            m_error = NetworkReply::Error::UnsupportedScheme;
        else
        {
            AVDictionary *options = nullptr;
            const QByteArray url = Functions::prepareFFmpegUrl(m_url, options, true, m_rawHeaders.isEmpty(), m_rawHeaders.isEmpty(), false, m_customUserAgent).toUtf8();
            av_dict_set(&options, "seekable", "0", 0);
            if (!m_postData.isNull())
            {
                av_dict_set(&options, "method", "POST", 0);
                if (!m_postData.isEmpty())
                    av_dict_set(&options, "post_data", m_postData.toHex(), 0);
            }
            if (!m_rawHeaders.isEmpty())
                av_dict_set(&options, "headers", m_rawHeaders, 0);

            AVIOInterruptCB interruptCB = {(int(*)(void*))::interruptCB, m_status};

            do
            {
                const int ret = avio_open2(&m_ctx, url, AVIO_FLAG_READ | AVIO_FLAG_DIRECT, &interruptCB, &options);
                if (ret >= 0)
                {
                    m_error = NetworkReply::Error::Ok;
                    break;
                }
                switch (ret)
                {
                    case AVERROR_HTTP_BAD_REQUEST:
                        m_error = NetworkReply::Error::Connection400;
                        break;
                    case AVERROR_HTTP_UNAUTHORIZED:
                        m_error = NetworkReply::Error::Connection401;
                        break;
                    case AVERROR_HTTP_FORBIDDEN:
                        m_error = NetworkReply::Error::Connection403;
                        break;
                    case AVERROR_HTTP_NOT_FOUND:
                        m_error = NetworkReply::Error::Connection404;
                        break;
                    case AVERROR_HTTP_OTHER_4XX:
                        m_error = NetworkReply::Error::Connection4XX;
                        break;
                    case AVERROR_HTTP_SERVER_ERROR:
                        m_error = NetworkReply::Error::Connection5XX;
                        continue; // Continue if server error (e.g. Service Temporarily Unavailable)
                    default:
                        m_error = NetworkReply::Error::Connection;
                        continue; // Continue if connection error
                }
                break;
            } while (--m_retries > 0 && doSleep(m_aborted));

            if (m_error == NetworkReply::Error::Ok)
            {
                char *cookies = nullptr;
                if (av_opt_get(m_ctx, "cookies", AV_OPT_SEARCH_CHILDREN, (uint8_t **)&cookies) >= 0)
                {
                    for (const QByteArray &cookie : QByteArray(cookies).trimmed().split('\n'))
                    {
                        int idx = cookie.indexOf(';');
                        if (idx < 0)
                            idx = cookie.length();
                        if (idx > 0)
                            m_cookies += cookie.left(idx) + "; ";
                    }
                    m_cookies.chop(1);
                    av_free(cookies);
                }

                int64_t size = avio_size(m_ctx);
                if (size >= m_maxSize)
                    m_error = NetworkReply::Error::FileTooLarge;
                else
                {
                    if (size < -1)
                        size = -1; //Unknown size

                    const int chunkSize = qMax<int>(4096, size / 1000);
                    quint8 *data = new quint8[chunkSize];
                    int64_t pos = 0;

                    for (;;)
                    {
                        const int received = avio_read(m_ctx, data, chunkSize);

                        if (received < 0) //Error
                        {
                            if (received != AVERROR_EOF)
                                m_error = NetworkReply::Error::Download;
                            break;
                        }

                        if (received > 0)
                        {
                            pos += received;

                            m_dataMutex.lock();
                            const int dataPos = m_data.size();
                            m_data.resize(dataPos + received);
                            memcpy(m_data.data() + dataPos, data, received);
                            m_dataMutex.unlock();

                            m_networkReplyMutex.lock();
                            if (m_networkReply)
                                emit m_networkReply->downloadProgress(pos, size);
                            m_networkReplyMutex.unlock();
                        }

                        if (received < chunkSize) //EOF
                            break;

                        if (pos + chunkSize >= m_maxSize)
                        {
                            m_error = NetworkReply::Error::FileTooLarge;
                            break;
                        }
                    }

                    delete[] data;
                }

                avio_closep(&m_ctx);
            }
        }

        m_networkReplyMutex.lock();
        if (m_networkReply)
            emit m_networkReply->finished();
        m_networkReplyMutex.unlock();
    }
};

/**/

QString NetworkReply::url() const
{
    return m_priv->m_url;
}

void NetworkReply::abort()
{
    m_priv->m_aborted = true;
}

bool NetworkReply::hasError() const
{
    return (error() != Error::Ok);
}
NetworkReply::Error NetworkReply::error() const
{
    return m_priv->m_aborted ? Error::Aborted : m_priv->m_error;
}

QByteArray NetworkReply::getCookies() const
{
    return m_priv->m_cookies;
}

QByteArray NetworkReply::readAll()
{
    QMutexLocker locker(&m_priv->m_dataMutex);
    const QByteArray ret = m_priv->m_data;
    m_priv->m_data.clear();
    return ret;
}

NetworkReply::Wait NetworkReply::waitForFinished(int ms)
{
    const bool inf = (ms < 0);
    bool ret = true;
    while (m_priv->isRunning() && (!m_priv->m_aborted || m_priv->m_afterOpen))
    {
        if (m_priv->m_afterOpen)
            ret = m_priv->wait(ms);
        else
        {
            // "avio_open2()" can block in DNS name resolution, so it doesn't call interrupt callback.
            const int timeout = inf ? 100 : qMin(ms, 100);
            ret = m_priv->wait(timeout);
            if (ret)
                break;
            if (!inf)
            {
                ms -= timeout;
                if (ms == 0)
                    break;
            }
        }
    }
    return ret ? (hasError() ? Wait::Error : Wait::Ok) : Wait::Timeout;
}

NetworkReply::NetworkReply(const QString &url, const QByteArray &postData, const QByteArray &rawHeaders, const NetworkAccessParams &params) :
    m_priv(new NetworkReplyPriv(this, url, postData, rawHeaders, params))
{}
NetworkReply::~NetworkReply()
{
    if (!m_priv->isRunning())
        delete m_priv;
    else
    {
        connect(m_priv, SIGNAL(finished()), m_priv, SLOT(deleteLater()));
        m_priv->m_networkReplyMutex.lock();
        m_priv->m_networkReply = nullptr;
        m_priv->m_networkReplyMutex.unlock();
        abort();
    }
}

/**/

const char *const NetworkAccess::UrlEncoded = "Content-Type: application/x-www-form-urlencoded; charset=utf-8\r\n";

NetworkAccess::NetworkAccess(QObject *parent) :
    QObject(parent),
    m_params(new NetworkAccessParams)
{}
NetworkAccess::~NetworkAccess()
{
    delete m_params;
}

void NetworkAccess::setCustomUserAgent(const QString &customUserAgent)
{
    m_params->customUserAgent = customUserAgent.toUtf8();
}
void NetworkAccess::setMaxDownloadSize(const int maxSize)
{
    m_params->maxSize = maxSize;
}
void NetworkAccess::setRetries(const int retries)
{
    if (retries > 0 && retries <= 10)
        m_params->retries = retries;
}

int NetworkAccess::getRetries() const
{
    return m_params->retries;
}

NetworkReply *NetworkAccess::start(const QString &url, const QByteArray &postData, const QByteArray &rawHeaders)
{
    const QByteArray rawHeadersData = (rawHeaders.isEmpty() || rawHeaders.endsWith("\r\n")) ? rawHeaders : rawHeaders + "\r\n";
    NetworkReply *reply = new NetworkReply(url, postData, rawHeadersData, *m_params);
    connect(reply, SIGNAL(finished()), this, SLOT(networkFinished()));
    // Don't set parent to "m_priv" - this prevents waiting for thread at QMPlay2 exit
    reply->setParent(this);
    reply->m_priv->start();
    return reply;
}
bool NetworkAccess::start(IOController<NetworkReply> &ioCtrl, const QString &url, const QByteArray &postData, const QByteArray &rawHeaders)
{
    return ioCtrl.assign(start(url, postData, rawHeaders));
}

bool NetworkAccess::startAndWait(IOController<NetworkReply> &ioCtrl, const QString &url, const QByteArray &postData, const QByteArray &rawHeaders, const int retries)
{
    const int oldRetries = m_params->retries;
    setRetries(retries);
    const bool ok = start(ioCtrl, url, postData, rawHeaders);
    m_params->retries = oldRetries;
    if (ok)
    {
        if (ioCtrl->waitForFinished() == NetworkReply::Wait::Ok)
            return true;
        ioCtrl.reset();
    }
    return false;
}

void NetworkAccess::networkFinished()
{
    if (NetworkReply *reply = (NetworkReply *)sender())
        emit finished(reply);
}
