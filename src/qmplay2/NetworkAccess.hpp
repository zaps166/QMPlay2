// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
    static constexpr int s_defaultRetryInterval = 20;

public:
    static const char *const UrlEncoded;

    NetworkAccess(QObject *parent = nullptr);
    ~NetworkAccess();

    void setCustomUserAgent(const QString &customUserAgent);
    void setMaxDownloadSize(const int maxSize);
    void setRetries(const int retries, const int retryInterval = s_defaultRetryInterval); // retryInterval is in 1/10 sec units

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
