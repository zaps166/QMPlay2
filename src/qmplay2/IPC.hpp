// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

class IPCSocketPriv;
class IPCServerPriv;
#ifdef Q_OS_WIN
    class PipeData;
#endif

class QMPLAY2SHAREDLIB_EXPORT IPCSocket final : public QIODevice
{
    Q_OBJECT
    friend class IPCServer;
public:
    IPCSocket(const QString &fileName, QObject *parent = nullptr);
    ~IPCSocket();

    bool isConnected() const;

    bool open(OpenMode mode) override;
    void close() override;

private slots:
    void socketReadActive();

private:
#ifdef Q_OS_WIN
    IPCSocket(const PipeData &pipeData, QObject *parent);
#else
    IPCSocket(int socket, QObject *parent);
#endif

    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;

    IPCSocketPriv *m_priv;
};

/**/

class QMPLAY2SHAREDLIB_EXPORT IPCServer final : public QObject
{
    Q_OBJECT
public:
    IPCServer(const QString &fileName, QObject *parent = nullptr);
    ~IPCServer();

    bool listen();
    void close();

signals:
    void newConnection(IPCSocket *socket);

private slots:
    void socketAcceptActive();

private:
    IPCServerPriv *m_priv;
};
