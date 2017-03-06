/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

#include <IOController.hpp>

class NetworkReplyPriv;

class NetworkReply : public QObject, public BasicIO
{
	Q_OBJECT

	friend class NetworkReplyPriv;
	friend class NetworkAccess;

public:
	enum class Error
	{
		Ok = 0,
		UnsupportedScheme,
		Connection,
		Connection400,
		Connection401,
		Connection403,
		Connection404,
		Connection4XX,
		Connection5XX,
		FileTooLarge,
		Download,
		Aborted
	};
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
	Q_ENUM(Error)
#endif

	enum class Wait
	{
		Ok = 0,
		Timeout,
		Error
	};
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
	Q_ENUM(Wait)
#endif

	~NetworkReply() final;

	QString url() const;

	void abort() override final;

	bool hasError() const;
	Error error() const;

	QByteArray getCookies() const;

	QByteArray readAll();

	Wait waitForFinished(int ms = -1);

signals:
	void downloadProgress(int pos, int total);
	void finished();

private:
	NetworkReply(const QString &url, const QByteArray &postData, const QByteArray &rawHeaders, const QByteArray &userAgent, const int maxSize);

	NetworkReplyPriv *m_priv;
};

/**/

class NetworkAccess : public QObject
{
	Q_OBJECT

public:
	static const char *const UrlEncoded;

	NetworkAccess(QObject *parent = nullptr);
	~NetworkAccess();

	void setCustomUserAgent(const QString &customUserAgent);
	void setMaxDownloadSize(const int maxSize);

	NetworkReply *start(const QString &url, const QByteArray &postData = QByteArray(), const QByteArray &rawHeaders = QByteArray());
	bool start(IOController<NetworkReply> &ioCtrl, const QString &url, const QByteArray &postData = QByteArray(), const QByteArray &rawHeaders = QByteArray());

	bool startAndWait(IOController<NetworkReply> &ioCtrl, const QString &url, const QByteArray &postData = QByteArray(), const QByteArray &rawHeaders = QByteArray());

signals:
	void finished(NetworkReply *reply);

private slots:
	void networkFinished();

private:
	QByteArray m_customUserAgent;
	qint32 m_maxSize;
};
