/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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
#include <QMPlay2Lib.hpp>

#include <QObject>

class NetworkReplyPriv;
struct NetworkAccessParams;

class QMPLAY2SHAREDLIB_EXPORT NetworkReply final : public QObject, public BasicIO
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
	Q_ENUM(Error)

	enum class Wait
	{
		Ok = 0,
		Timeout,
		Error
	};
	Q_ENUM(Wait)

	~NetworkReply();

	QString url() const;

	void abort() override;

	bool hasError() const;
	Error error() const;

	QByteArray getCookies() const;

	QByteArray readAll();

	Wait waitForFinished(int ms = -1);

signals:
	void downloadProgress(int pos, int total);
	void finished();

private:
	NetworkReply(const QString &url, const QByteArray &postData, const QByteArray &rawHeaders, const NetworkAccessParams &params);

	NetworkReplyPriv *m_priv;
};

/**/

class QMPLAY2SHAREDLIB_EXPORT NetworkAccess : public QObject
{
	Q_OBJECT

public:
	static const char *const UrlEncoded;

	NetworkAccess(QObject *parent = nullptr);
	~NetworkAccess();

	void setCustomUserAgent(const QString &customUserAgent);
	void setMaxDownloadSize(const int maxSize);
	void setRetries(const int retries);

	int getRetries() const;

	NetworkReply *start(const QString &url, const QByteArray &postData = QByteArray(), const QByteArray &rawHeaders = QByteArray());
	bool start(IOController<NetworkReply> &ioCtrl, const QString &url, const QByteArray &postData = QByteArray(), const QByteArray &rawHeaders = QByteArray());

	bool startAndWait(IOController<NetworkReply> &ioCtrl, const QString &url, const QByteArray &postData = QByteArray(), const QByteArray &rawHeaders = QByteArray(), const int retries = -1);

signals:
	void finished(NetworkReply *reply);

private slots:
	void networkFinished();

private:
	NetworkAccessParams *m_params;
};
