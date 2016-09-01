#ifndef HTTP_HPP
#define HTTP_HPP

#include <QObject>

class HttpReplyPriv;
struct AVIOContext;

class HttpReply : public QObject
{
	Q_OBJECT
	friend class HttpReplyPriv;
	friend class Http;

public:
	enum HTTP_ERROR
	{
		NO_HTTP_ERROR = 0,
		UNSUPPORTED_SCHEME_ERROR,
		CONNECTION_ERROR,
		CONNECTION_ERROR_400,
		CONNECTION_ERROR_401,
		CONNECTION_ERROR_403,
		CONNECTION_ERROR_404,
		CONNECTION_ERROR_4XX,
		CONNECTION_ERROR_5XX,
		FILE_TOO_LARGE_ERROR,
		DOWNLOAD_ERROR,
		ABORTED
	};

	QByteArray url() const;

	void abort();

	HTTP_ERROR error() const;

	int size() const;
	QByteArray readAll();

signals:
	void downloadProgress(int pos, int total);
	void finished();

private:
	HttpReply(const QByteArray &url, const QByteArray &postData, const QByteArray &rawHeaders, const QByteArray &userAgent);
	~HttpReply();

	void run();

	HttpReplyPriv *m_priv;
};

/**/

class Http : public QObject
{
	Q_OBJECT
public:
	Http(QObject *parent = NULL);
	~Http();

	inline void setCustomUserAgent(const QString &customUserAgent)
	{
		m_customUserAgent = customUserAgent.toUtf8();
	}

	HttpReply *start(const QString &url, const QByteArray &postData = QByteArray(), const QString &rawHeaders = QString());

signals:
	void finished(HttpReply *reply);

private slots:
	void httpFinished();

private:
	QByteArray m_customUserAgent;
};

#endif // HTTP_HPP
