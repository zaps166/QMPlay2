#include <Http.hpp>
#include <Version.hpp>

extern "C"
{
	#include <libavformat/avformat.h>
	#include <libavformat/avio.h>
	#include <libavutil/dict.h>
}

#include <QThread>
#include <QMutex>

static int interruptCB(bool &aborted)
{
	return aborted;
}

/**/

class HttpReplyPriv : public QThread
{
public:
	HttpReplyPriv(HttpReply *httpReply, const QByteArray &url, const QByteArray &postData, const QByteArray &rawHeaders, const QByteArray &userAgent) :
		m_httpReply(httpReply),
		m_url(url),
		m_postData(postData),
		m_rawHeaders(rawHeaders),
		m_userAgent(userAgent),
		m_ctx(NULL),
		m_error(HttpReply::NO_HTTP_ERROR),
		m_aborted(false)
	{}

	HttpReply *m_httpReply;

	const QByteArray m_url;
	const QByteArray m_postData;
	const QByteArray m_rawHeaders;
	const QByteArray m_userAgent;

	AVIOContext *m_ctx;
	QByteArray m_data;
	HttpReply::HTTP_ERROR m_error;

	bool m_aborted;
	QMutex m_mutex;

private:
	void run()
	{
		if (!m_url.startsWith("http:") && !m_url.startsWith("https:"))
			m_error = HttpReply::UNSUPPORTED_SCHEME_ERROR;
		else
		{
			AVDictionary *options = NULL;
			av_dict_set(&options, "user-agent", m_userAgent, 0);
			av_dict_set(&options, "seekable", "0", 0);
#if LIBAVFORMAT_VERSION_MAJOR > 55
			av_dict_set(&options, "icy", "0", 0);
#endif
			if (!m_postData.isNull())
			{
				av_dict_set(&options, "method", "POST", 0);
				if (!m_postData.isEmpty())
					av_dict_set(&options, "post_data", m_postData.toHex(), 0);
			}
			if (!m_rawHeaders.isEmpty())
				av_dict_set(&options, "headers", m_rawHeaders, 0);

			AVIOInterruptCB interruptCB = {(int(*)(void*))::interruptCB, &m_aborted};

			const int ret = avio_open2(&m_ctx, m_url, AVIO_FLAG_READ | AVIO_FLAG_DIRECT, &interruptCB, &options);
			if (ret >= 0)
			{
				int64_t size = avio_size(m_ctx);
				if (size >= INT_MAX)
					m_error = HttpReply::FILE_TOO_LARGE_ERROR;
				else
				{
					if (size < -1)
						size = -1; //Unknown size

					const int chunkSize = qMax<int>(4096, size / 1000);
					int64_t pos = 0;

					for (;;)
					{
						const int dataPos = m_data.size();
						m_data.resize(dataPos + chunkSize);

						const int received = avio_read(m_ctx, (quint8 *)m_data.data() + dataPos, chunkSize);

						if (received < 0) //Error
						{
							m_data.resize(dataPos);
							if (received == AVERROR_EOF)
								break; //EOF
							m_error = HttpReply::DOWNLOAD_ERROR;
							break;
						}

						if (received > 0)
						{
							pos += received;
							m_mutex.lock();
							if (m_httpReply)
								emit m_httpReply->downloadProgress(pos, size);
							m_mutex.unlock();
						}

						if (received < chunkSize) //EOF
						{
							m_data.resize(dataPos + received);
							break;
						}

						if (pos + chunkSize >= INT_MAX)
						{
							m_error = HttpReply::FILE_TOO_LARGE_ERROR;
							break;
						}
					}
				}
				avio_closep(&m_ctx);
			}
			else
			{
				switch (ret)
				{
					case AVERROR_HTTP_BAD_REQUEST:
						m_error = HttpReply::CONNECTION_ERROR_400;
						break;
					case AVERROR_HTTP_UNAUTHORIZED:
						m_error = HttpReply::CONNECTION_ERROR_401;
						break;
					case AVERROR_HTTP_FORBIDDEN:
						m_error = HttpReply::CONNECTION_ERROR_403;
						break;
					case AVERROR_HTTP_NOT_FOUND:
						m_error = HttpReply::CONNECTION_ERROR_404;
						break;
					case AVERROR_HTTP_OTHER_4XX:
						m_error = HttpReply::CONNECTION_ERROR_4XX;
						break;
					case AVERROR_HTTP_SERVER_ERROR:
						m_error = HttpReply::CONNECTION_ERROR_5XX;
						break;
					default:
						m_error = HttpReply::CONNECTION_ERROR;
						break;
				}
			}
		}

		if (m_aborted)
			m_error = HttpReply::ABORTED;

		m_mutex.lock();
		if (m_httpReply)
			emit m_httpReply->finished();
		m_mutex.unlock();
	}
};

/**/

QByteArray HttpReply::url() const
{
	return m_priv->m_url;
}

void HttpReply::abort()
{
	m_priv->m_aborted = true;
}

HttpReply::HTTP_ERROR HttpReply::error() const
{
	return m_priv->m_error;
}

int HttpReply::size() const
{
	return m_priv->m_data.size();
}

QByteArray HttpReply::readAll()
{
	const QByteArray ret = m_priv->m_data;
	m_priv->m_data.clear();
	return ret;
}

HttpReply::HttpReply(const QByteArray &url, const QByteArray &postData, const QByteArray &rawHeaders, const QByteArray &userAgent) :
	m_priv(new HttpReplyPriv(this, url, postData, rawHeaders, userAgent))
{}
HttpReply::~HttpReply()
{
	if (!m_priv->isRunning())
		delete m_priv;
	else
	{
		connect(m_priv, SIGNAL(finished()), m_priv, SLOT(deleteLater()));
		m_priv->m_mutex.lock();
		m_priv->m_httpReply = NULL;
		m_priv->m_mutex.unlock();
		abort();
	}
}

/**/

Http::Http(QObject *parent) :
	QObject(parent)
{}
Http::~Http()
{}

HttpReply *Http::start(const QString &url, const QByteArray &postData, const QString &rawHeaders)
{
	HttpReply *reply = new HttpReply(url.toUtf8(), postData, (rawHeaders.endsWith("\r\n") ? rawHeaders : rawHeaders + "\r\n").toUtf8(), m_customUserAgent.isNull() ? QMPlay2UserAgent : m_customUserAgent);
	connect(reply, SIGNAL(finished()), this, SLOT(httpFinished()));
	//Don't set parent to "m_priv" - this prevents waiting for thread at QMPlay2 exit
	reply->setParent(this);
	reply->m_priv->start();
	return reply;
}

void Http::httpFinished()
{
	if (HttpReply *reply = (HttpReply *)sender())
		emit finished(reply);
}
